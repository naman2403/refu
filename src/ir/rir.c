#include <ir/rir.h>
#include <ir/rir_function.h>
#include <ir/rir_block.h>
#include <ir/rir_type.h>
#include <ir/rir_object.h>
#include <ir/rir_expression.h>
#include <ir/rir_types_list.h>
#include <ir/rir_typedef.h>
#include <types/type.h>
#include <Utils/memory.h>
#include <String/rf_str_common.h>
#include <String/rf_str_corex.h>
#include <ast/ast.h>
#include <ast/ast_utils.h>
#include <module.h>
#include <compiler.h>

static inline void rir_ctx_init(struct rir_ctx *ctx, struct rir *r, struct module *m)
{
    RF_STRUCT_ZERO(ctx);
    ctx->rir = r;
    darray_init(ctx->st_stack);
    rir_ctx_push_st(ctx, &m->node->module.st);
    darray_init(ctx->visited_rir_types);
}

static inline void rir_ctx_deinit(struct rir_ctx *ctx)
{
    darray_free(ctx->visited_rir_types);
    darray_free(ctx->st_stack);
}

void rir_ctx_reset(struct rir_ctx *ctx)
{
    ctx->expression_idx = 0;
    ctx->label_idx = 0;
}

void rir_ctx_push_st(struct rir_ctx *ctx, struct symbol_table *st)
{
    darray_append(ctx->st_stack, st);
}

struct symbol_table *rir_ctx_pop_st(struct rir_ctx *ctx)
{
    RF_ASSERT(darray_size(ctx->st_stack) > 0, "Tried to pop from an empty stack");
    return darray_pop(ctx->st_stack);
}

struct symbol_table *rir_ctx_curr_st(struct rir_ctx *ctx)
{
    return darray_top(ctx->st_stack);
}

bool rir_ctx_st_setobj(struct rir_ctx *ctx, const struct RFstring *id, struct rir_object *obj)
{
    struct symbol_table_record *rec = symbol_table_lookup_record(rir_ctx_curr_st(ctx), id, NULL);
    if (!rec) {
        return false;
    }
    rec->rirobj = obj;
    return true;
}

bool rir_ctx_st_newobj(struct rir_ctx *ctx, const struct RFstring *id, struct type *t, struct rir_object *obj)
{

    struct symbol_table_record *rec = symbol_table_record_create_from_type(rir_ctx_curr_st(ctx), id, t);
    if (!rec) {
        return false;
    }
    rec->rirobj = obj;
    if (!symbol_table_add_record(rir_ctx_curr_st(ctx), rec)) {
        return false;
    }
    return true;
}

struct rir_object *rir_ctx_st_getobj(struct rir_ctx *ctx, const struct RFstring *id)
{
    struct symbol_table_record *rec = symbol_table_lookup_record(rir_ctx_curr_st(ctx), id, NULL);
    return rec ? rec->rirobj : NULL;
}

static void rir_symbol_table_create_allocas_do(struct symbol_table_record *rec,
                                               struct rir_ctx *ctx)
{
    struct rir_ltype *type = rir_ltype_create_from_type(symbol_table_record_type(rec), ctx);
    RF_ASSERT_OR_EXIT(type, "Could not create a rir_ltype during symbol table iteration");
    struct rir_object *alloca = rir_alloca_create_obj(type, 1, ctx);
    RF_ASSERT_OR_EXIT(alloca, "Could not create an alloca object during symbol table iteration");
    rec->rirobj = alloca;
}

static void rir_symbol_table_add_allocas_do(struct symbol_table_record *rec,
                                            struct rir_ctx *ctx)
{
    RF_ASSERT(rec->rirobj && rec->rirobj->category == RIR_OBJ_EXPRESSION,
              "Expected an expression rir object");
    rirctx_block_add(ctx, &rec->rirobj->expr);
}

static void rir_symbol_table_create_and_add_allocas_do(struct symbol_table_record *rec,
                                                       struct rir_ctx *ctx)
{
    rir_symbol_table_create_allocas_do(rec, ctx);
    rir_symbol_table_add_allocas_do(rec, ctx);
}

void rir_ctx_st_create_allocas(struct rir_ctx *ctx)
{
    symbol_table_iterate(rir_ctx_curr_st(ctx),
                         (htable_iter_cb)rir_symbol_table_create_allocas_do,
                         ctx);
}

void rir_ctx_st_add_allocas(struct rir_ctx *ctx)
{
    symbol_table_iterate(rir_ctx_curr_st(ctx),
                         (htable_iter_cb)rir_symbol_table_add_allocas_do,
                         ctx);
}

void rir_ctx_st_create_and_add_allocas(struct rir_ctx *ctx)
{
    symbol_table_iterate(rir_ctx_curr_st(ctx),
                         (htable_iter_cb)rir_symbol_table_create_and_add_allocas_do,
                         ctx);
}

void rir_ctx_visit_type(struct rir_ctx *ctx, const struct rir_type *t)
{
    darray_append(ctx->visited_rir_types, t);
}

bool rir_ctx_type_visited(struct rir_ctx *ctx, const struct rir_type *t)
{
    const struct rir_type **type;
    darray_foreach(type, ctx->visited_rir_types) {
        if (rir_type_equals(*type, t, RIR_TYPECMP_SIMPLE)) {
            return true;
        }
    }
    return false;
}


static struct rir_value *rir_ctx_lastobj_get(struct rir_object *obj)
{
    if (!obj) {
        return NULL;
    }
    return rir_object_value(obj);
}
struct rir_value *rir_ctx_lastval_get(const struct rir_ctx *ctx)
{
    return rir_ctx_lastobj_get(ctx->returned_obj);
}
struct rir_value *rir_ctx_lastassignval_get(const struct rir_ctx *ctx)
{
    return rir_ctx_lastobj_get(ctx->last_assign_obj);
}


static bool rir_init(struct rir *r, struct module *m)
{
    RF_STRUCT_ZERO(r);
    strmap_init(&r->map);
    rf_ilist_head_init(&r->functions);
    rf_ilist_head_init(&r->objects);
    rf_ilist_head_init(&r->typedefs);
    // create the rir types list from the types set for this module
    if (!(r->rir_types_list = rir_types_list_create(m->types_set))) {
        return false;
    }
    return true;
}

struct rir *rir_create(struct module *m)
{
    struct rir *ret;
    RF_MALLOC(ret, sizeof(*ret), return NULL);
    if (!rir_init(ret, m)) {
        free(ret);
        ret = NULL;
    }
    return ret;
}

static void rir_deinit(struct rir *r)
{
    struct rir_fndecl *fn;
    struct rir_fndecl *tmp;
    if (r->rir_types_list) {
        rir_types_list_destroy(r->rir_types_list);
    }
    rf_ilist_for_each_safe(&r->functions, fn, tmp, ln) {
        rir_fndecl_destroy(fn);
    }

    // TODO
    // all other rir objects are in the global rir object list so destroy them
    /* struct rir_object *obj; */
    /* struct rir_object *tmpobj; */
    /* rf_ilist_for_each_safe(&r->objects, obj, tmpobj, ln) { */
    /*     rir_object_destroy(obj); */
    /* } */
}

void rir_destroy(struct rir *r)
{
    rir_deinit(r);
    free(r);
}

/* -- functions for finalizing the ast and creating the RIR -- */

static bool rir_process_do(struct rir *r, struct module *m)
{
    bool ret = false;
    struct ast_node *child;
    struct rir_fndecl *fndecl;
    struct rir_ctx ctx;
    struct rir_type *t;
    rir_ctx_init(&ctx, r, m);
    // for each non elementary, non sum-type rir type create a typedef
    rir_types_list_for_each(r->rir_types_list, t) {
        // TODO: this check should go away ... is temporary due to rir_types_list_init()
        // actually putting two copies of a rir sum type in the list. Please fix!!
        if (t->category == COMPOSITE_RIR_DEFINED) {
            struct rir_object *checkdef = strmap_get(&ctx.rir->map, t->name);
            if (checkdef) {
                continue;
            }
        }
        if (!rir_type_is_elementary(t) && !rir_ctx_type_visited(&ctx, t) &&
            t->category != COMPOSITE_IMPLICATION_RIR_TYPE) {
            struct rir_typedef *def = rir_typedef_create(t, &ctx);
            if (!def) {
                RF_ERROR("Failed to create a RIR typedef");
                goto end;
            }
            rf_ilist_add_tail(&r->typedefs,  &def->ln);
        }
    }
    // for each function of the module, create a rir equivalent
    rf_ilist_for_each(&m->node->children, child, lh) {
        if (child->type == AST_FUNCTION_IMPLEMENTATION) {
            fndecl = rir_fndecl_create(child, &ctx);
            if (!fndecl) {
                RF_ERROR("Failed to create a RIR fndecl");
                goto end;
            }
            rf_ilist_add(&r->functions, &fndecl->ln);
        }
    }

    // success
    ret = true;
end:
    rir_ctx_deinit(&ctx);
    return ret;
}

bool rir_process(struct compiler *c)
{
    // for each module of the compiler do rir to string
    struct module *mod;
    rf_ilist_for_each(&c->sorted_modules, mod, ln) {
        if (!rir_process_do(mod->rir, mod)) {
            RF_ERROR("Failed to create the RIR for module \""RF_STR_PF_FMT"\"",
                     module_name(mod));
            return false;
        }
    }
    return true;
}

void rirtostr_ctx_reset(struct rirtostr_ctx *ctx)
{
    darray_clear(ctx->visited_blocks);
    darray_init(ctx->visited_blocks);
}

static inline void rirtostr_ctx_deinit(struct rirtostr_ctx *ctx)
{
    darray_clear(ctx->visited_blocks);
}

void rirtostr_ctx_init(struct rirtostr_ctx *ctx, struct rir *r)
{
    ctx->rir = r;
    darray_init(ctx->visited_blocks);
}

void rirtostr_ctx_visit_block(struct rirtostr_ctx *ctx, const struct rir_block *b)
{
    darray_append(ctx->visited_blocks, b);
}

bool rirtostr_ctx_block_visited(struct rirtostr_ctx *ctx, const struct rir_block *b)
{
    const struct rir_block **block;
    darray_foreach(block, ctx->visited_blocks) {
        if (*block == b) {
            return true;
        }
    }
    return false;
}

struct RFstring *rir_tostring(struct rir *r)
{
    if (r->buff) {
        return RF_STRX2STR(r->buff);
    }

    r->buff = rf_stringx_create_buff(1024, "");
    if (!r->buff) {
        RF_ERROR("Failed to create the string buffer for rir outputting");
        return NULL;
    }

    struct rirtostr_ctx ctx;
    rirtostr_ctx_init(&ctx, r);
    struct rir_typedef *def;
    rf_ilist_for_each(&r->typedefs, def, ln) {
        if (!rir_typedef_tostring(&ctx, def)) {
            RF_ERROR("Failed to turn a rir typedef to a string");
            goto fail_free_ctx;
        }
    }

    struct rir_fndecl *fn;
    rf_ilist_for_each(&r->functions, fn, ln) {
        if (!rir_fndecl_tostring(&ctx, fn)) {
            RF_ERROR("Failed to turn a rir function "RF_STR_PF_FMT" to a string",
                     RF_STR_PF_ARG(fn->name));
            goto fail_free_ctx;
        }
    }
    return RF_STRX2STR(r->buff);

fail_free_ctx:
    rirtostr_ctx_deinit(&ctx);
    return NULL;
}

bool rir_print(struct compiler *c)
{
    // for each module of the compiler do rir to string
    struct module *mod;
    struct RFstring *s;
    rf_ilist_for_each(&c->sorted_modules, mod, ln) {
        if (!(s = rir_tostring(mod->rir))) {
            return false;
        }
        printf(RF_STR_PF_FMT"\n", RF_STR_PF_ARG(s));
    }
    return true;
}

struct rir_typedef *rir_typedef_byname(const struct rir *r, const struct RFstring *name)
{
    struct rir_typedef *def;
    rf_ilist_for_each(&r->typedefs, def, ln) {
        if (rf_string_equal(name, def->name)) {
            return def;
        }
    }
    return NULL;
}

struct rir_ltype *rir_type_byname(const struct rir *r, const struct RFstring *name)
{
    struct rir_ltype *type = rir_ltype_elem_create_from_string(name, false);
    if (type) {
        return type;
    }
    // not elementary, search for typedef
    struct rir_typedef *def = rir_typedef_byname(r, name);
    if (!def) {
        return NULL;
    }
    return rir_ltype_comp_create(def, false);
}

void rirctx_block_add(struct rir_ctx *ctx, struct rir_expression *expr)
{
    rf_ilist_add_tail(&ctx->current_block->expressions, &expr->ln);
}

