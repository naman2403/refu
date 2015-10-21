#ifndef LFR_IR_RIR
#define LFR_IR_RIR

#include <RFintrusive_list.h>
#include <Data_Structures/darray.h>
#include <ir/rir_strmap.h>
#include <String/rf_str_decl.h>

struct module;
struct compiler;
struct rir_types_list;
struct rir_expression;
struct RFstringx;
struct ast_node;
struct type;
struct rir_type;

struct rir_arr {darray(struct rir*);};
struct rir {
    //! Set of all types of the file, moved here from struct module.
    struct rf_objset_type *types_set;
    //! Map of all global string literals of the module
    struct rirobj_strmap global_literals;
    //! Pointers to values that don't belong to any rir object. Will be destroyed at the end
    struct {darray(struct rir_value*);} free_values;
    //! List of function declarations/definitions
    struct RFilist_head functions;
    //! List of type definitions
    struct RFilist_head typedefs;
    //! List of all rir objects
    struct RFilist_head objects;
    //! A dynamic array of all other rir modules this rir module depends on
    struct rir_arr dependencies;
    //! Name of the module this RIR object represents.
    struct RFstring name;
    //! Buffer string to hold the string representation when asked. Can be NULL.
    struct RFstringx *buff;
    //! Map from strings to rir objects.
    struct rirobj_strmap map;
};

struct RFstring *rir_tostring(struct rir *r);

/**
 * Create a rir module
 *
 * @param m            If we are creating the RIR module directly after AST
 *                     parsing then this will point to the parsed module
 *                     from which we will read the types set and create the
 *                     rir types list. If not the rir_types list will be empty.
 * @return             The allocated rir module
 */
struct rir *rir_create(struct module *m);
void rir_destroy(struct rir* r);

bool compiler_create_rir();
bool rir_print(struct compiler *c);

struct rir_fndecl *rir_fndecl_byname(const struct rir *r, const struct RFstring *name);
struct rir_typedef *rir_typedef_frommap(const struct rir *r, const struct RFstring *name);
struct rir_typedef *rir_typedef_byname(const struct rir *r, const struct RFstring *name);
struct rir_ltype *rir_ltype_byname(const struct rir *r, const struct RFstring *name);
struct rir_object *rir_strlit_obj(const struct rir *r, const struct ast_node *lit);

void rir_freevalues_add(struct rir *r, struct rir_value *v);

struct rir_ctx {
    struct rir *rir;
    struct rir_fndef *current_fn;
    struct rir_block *current_block;
    struct rir_block *next_block;
    //! Stack of symbol table pointers, to remember current symbol table during rir formation
    struct {darray(struct symbol_table*);} st_stack;
    //! The current ast block object we are visiting
    const struct ast_node *current_ast_block;
    //! The value generated by the left hand side of the last assignment.
    //! Most of the times this is going to be same as the returned_obj but adding
    //! it here for explicitness
    struct rir_object *last_assign_obj;
    //! Used as the return value, if existing, of all process_XX() functions. Can be NULL
    struct rir_object *returned_obj;
    //! Used to enumerate numeric value to go to all expressions that need it. Is reset for each function.
    unsigned expression_idx;
    //! Used to enumerate numeric value for all labels. Is reset for each function.
    unsigned label_idx;
};

void rir_ctx_reset(struct rir_ctx *ctx);
struct rir_value *rir_ctx_lastval_get(const struct rir_ctx *c);
struct rir_value *rir_ctx_lastassignval_get(const struct rir_ctx *c);
void rir_ctx_push_st(struct rir_ctx *ctx, struct symbol_table *st);
struct symbol_table *rir_ctx_pop_st(struct rir_ctx *ctx);
struct symbol_table *rir_ctx_curr_st(struct rir_ctx *ctx);
bool rir_ctx_st_newobj(struct rir_ctx *ctx, const struct RFstring *id, struct type *t, struct rir_object *obj);
bool rir_ctx_st_setobj(struct rir_ctx *ctx, const struct RFstring *id, struct rir_object *obj);
bool rir_ctx_st_setrecobj(struct rir_ctx *ctx, const struct ast_node *desc, struct rir_object *obj);
struct rir_object *rir_ctx_st_getobj(struct rir_ctx *ctx, const struct RFstring *id);
void rir_ctx_st_create_allocas(struct rir_ctx *ctx);
void rir_ctx_st_add_allocas(struct rir_ctx *ctx);
void rir_ctx_st_create_and_add_allocas(struct rir_ctx *ctx);
void rir_strec_add_allocas(struct symbol_table_record *rec,
                           struct rir_ctx *ctx);
void rir_strec_create_allocas(struct symbol_table_record *rec,
                              struct rir_ctx *ctx);

#define RIRCTX_RETURN_EXPR(i_ctx_, i_result_, i_obj_)   \
    do {                                                \
        (i_ctx_)->returned_obj = i_obj_;                \
        return i_result_;                               \
    } while (0)


void rirctx_block_add(struct rir_ctx *ctx, struct rir_expression *expr);


struct rirtostr_ctx {
    struct rir *rir;
    struct {darray(const struct rir_block*);} visited_blocks;
};
void rirtostr_ctx_init(struct rirtostr_ctx *ctx, struct rir *r);
void rirtostr_ctx_reset(struct rirtostr_ctx *ctx);
void rirtostr_ctx_visit_block(struct rirtostr_ctx *ctx, const struct rir_block *b);
bool rirtostr_ctx_block_visited(struct rirtostr_ctx *ctx, const struct rir_block *b);

//! Represents one indentation level in the resulting string representation
#define RIRTOSTR_INDENT "    "
#endif
