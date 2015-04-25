#include "llvm_functions.h"
#include "llvm_ast.h"

#include <llvm-c/Core.h>

#include <String/rf_str_common.h>
#include <String/rf_str_conversion.h>

#include <analyzer/symbol_table.h>
#include <ast/function.h>
#include <types/type.h>
#include <types/type_function.h>
#include <ir/elements.h>
#include <ir/rir_type.h>
#include <ir/rir.h>


#include "llvm_ast.h"
#include "llvm_utils.h"

struct ctor_args_to_value_cb_ctx {
    unsigned int index;
    unsigned int offset;
    struct llvm_traversal_ctx *llvm_ctx;
    LLVMValueRef alloca;
    LLVMTypeRef *params;
};

static void ctor_args_to_value_cb_ctx_init(struct ctor_args_to_value_cb_ctx *ctx,
                                           struct llvm_traversal_ctx *llvm_ctx,
                                           LLVMValueRef alloca,
                                           LLVMTypeRef *params)
{
    ctx->index = 0;
    ctx->offset = 0;
    ctx->llvm_ctx = llvm_ctx;
    ctx->alloca = alloca;
    ctx->params = params;
}

static bool ctor_args_to_value_cb(struct ast_node *n, struct ctor_args_to_value_cb_ctx *ctx)
{
    LLVMValueRef arg_value = bllvm_compile_expression(n, ctx->llvm_ctx, 0);
    LLVMValueRef indices[] = { LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), ctx->offset, 0) };
    LLVMValueRef gep = LLVMBuildGEP(ctx->llvm_ctx->builder, ctx->alloca, indices, 2, "");

    bllvm_store(arg_value, gep, ctx->llvm_ctx);
    ctx->offset += 1;
    ctx->index++;
    return true;
}


static LLVMValueRef bllvm_assign_params_to_defined_type(struct ast_node *fn_call,
                                                        LLVMTypeRef type,
                                                        LLVMTypeRef *params,
                                                        struct llvm_traversal_ctx *ctx)
{
    struct ctor_args_to_value_cb_ctx cb_ctx;
    LLVMValueRef allocation = LLVMBuildAlloca(ctx->builder, type, "");
    ctor_args_to_value_cb_ctx_init(&cb_ctx, ctx, allocation, params);
    ast_fncall_for_each_arg(fn_call, (fncall_args_cb)ctor_args_to_value_cb, &cb_ctx);
    return allocation;
}

static LLVMValueRef bllvm_simple_ctor_args_to_type(struct ast_node *fn_call,
                                                   const struct RFstring *type_name,
                                                   struct llvm_traversal_ctx *ctx)
{
    char *name;
    // alloca enough space in the stack for the type created by the constructor
    RFS_PUSH();
    name = rf_string_cstr_from_buff_or_die(type_name);
    LLVMTypeRef llvm_type = LLVMGetTypeByName(ctx->mod, name);
    RFS_POP();

    LLVMTypeRef *params;
    struct rir_type *defined_type = rir_types_list_get_defined(&ctx->rir->rir_types_list, type_name);
    RF_ASSERT(!rir_type_is_sumtype(defined_type), "Called with sum type");
    params = bllvm_simple_member_types(defined_type, ctx);
    return bllvm_assign_params_to_defined_type(fn_call, llvm_type, params, ctx);
}

static LLVMValueRef bllvm_sum_ctor_args_to_type(struct ast_node *fn_call,
                                                const struct RFstring *type_name,
                                                struct llvm_traversal_ctx *ctx)
{
    struct rir_type *defined_type = rir_types_list_get_defined(&ctx->rir->rir_types_list, type_name);
    RF_ASSERT(rir_type_is_sumtype(defined_type), "Called with non sum type");
    struct rir_type *params_type = rir_types_list_get_type(
        &ctx->rir->rir_types_list,
        ast_fncall_params_type(fn_call),
        NULL
    );
    // find out the index of the sum operand type in the defined type
    size_t child_index = 0;
    bool child_found = false;
    struct rir_type **subtype;
    darray_foreach(subtype, darray_item(defined_type->subtypes, 0)->subtypes) {
        // TODO: pointer comparison fails here. Investigate why. Thought only had unique rir types
        if (rir_type_equals(*subtype, params_type)) {
            child_found = true;
            break;
        }
        ++child_index;
    }
    RF_ASSERT(child_found, "LALALA");
    RF_ASSERT_OR_CRITICAL(child_found,
                          return NULL,
                          "Type should have been found as child of defined type");
    // get the LLVM struct type of the sum operand
    RFS_PUSH();
    LLVMTypeRef llvm_sum_type = LLVMGetTypeByName(
        ctx->mod,
        rf_string_cstr_from_buff_or_die(RFS_OR_DIE("internal_struct%u", rir_type_get_uid(params_type)))
    );
    RFS_POP();
    RF_ASSERT(llvm_sum_type, "Internal struct was not created for sum operand");
    // populate the sum operand's internal struct contents
    LLVMTypeRef *params = bllvm_type_to_subtype_array(params_type, ctx);
    LLVMValueRef populated_sum_type = bllvm_assign_params_to_defined_type(fn_call, llvm_sum_type, params, ctx);
    // now create the full struct and assign the contents and the selector
    RFS_PUSH();
    LLVMTypeRef llvm_type = LLVMGetTypeByName(ctx->mod,
                                              rf_string_cstr_from_buff_or_die(type_name));
    LLVMValueRef allocation = LLVMBuildAlloca(ctx->builder, llvm_type, "");
    RFS_POP();
    LLVMValueRef indices[] = { LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), 0, 0) };
    LLVMValueRef gep_to_main_contents = LLVMBuildGEP(ctx->builder, allocation, indices, 2, "");
    bllvm_assign_defined_types(populated_sum_type, gep_to_main_contents, ctx);
    // TODO: here also set the second value of the struct (the alloca) which should be the selector
    LLVMValueRef indices2[] = { LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), 1, 0) };
    LLVMValueRef gep_to_selector = LLVMBuildGEP(ctx->builder, allocation, indices2, 2, "");
    bllvm_store(LLVMConstInt(LLVMInt32Type(), 1, 0), gep_to_selector, ctx);    
    return allocation;
}

static LLVMValueRef bllvm_ctor_args_to_type(struct ast_node *fn_call,
                                            const struct RFstring *type_name,
                                            struct llvm_traversal_ctx *ctx)
{
    struct rir_type *defined_type = rir_types_list_get_defined(&ctx->rir->rir_types_list, type_name);
    if (rir_type_is_sumtype(defined_type)) {
        return bllvm_sum_ctor_args_to_type(fn_call, type_name, ctx);
    }
    return bllvm_simple_ctor_args_to_type(fn_call, type_name, ctx);
}

static bool fncall_args_to_value_cb(struct ast_node *n, struct llvm_traversal_ctx *ctx)
{
    LLVMValueRef arg_value = bllvm_compile_expression(n, ctx, 0);
    llvm_traversal_ctx_add_value(ctx, arg_value);
    return true;
}

LLVMValueRef bllvm_compile_functioncall(struct ast_node *n,
                                        struct llvm_traversal_ctx *ctx)
{
    // for now just deal with the built-in print() function
    const struct RFstring *fn_name = ast_fncall_name(n);
    const struct type *fn_type;
    struct ast_node *args = ast_fncall_args(n);
    fn_type = type_lookup_identifier_string(fn_name, ctx->current_st);
    if (type_is_function(fn_type)) {
        llvm_traversal_ctx_reset_values(ctx);
        ast_fncall_for_each_arg(n, (fncall_args_cb)fncall_args_to_value_cb, ctx);
        RFS_PUSH();
        char *fn_name_cstr = rf_string_cstr_from_buff_or_die(fn_name);
        LLVMValueRef llvm_fn = LLVMGetNamedFunction(ctx->mod, fn_name_cstr);
        RFS_POP();
        LLVMTypeRef llvm_fn_type = bllvm_function_type(llvm_fn);
        RF_ASSERT(LLVMCountParamTypes(llvm_fn_type) == llvm_traversal_ctx_get_values_count(ctx),
                  "Function \""RF_STR_PF_FMT"()\" receiving unexpected number of "
                  "arguments in backend code generation", RF_STR_PF_ARG(fn_name));
        return LLVMBuildCall(ctx->builder,
                             LLVMGetNamedFunction(ctx->mod, fn_name_cstr),
                             llvm_traversal_ctx_get_values(ctx),
                             llvm_traversal_ctx_get_values_count(ctx),
                             "");
    } else if (fn_type->category == TYPE_CATEGORY_DEFINED) {
        return bllvm_ctor_args_to_type(n, fn_name, ctx);
    } else {
        RF_ASSERT(type_is_explicitly_convertable_elementary(fn_type),
                  "At this point the only possible call should be explicit cast");
        return bllvm_compile_explicit_cast(fn_type, args, ctx);
    }

    RF_ASSERT_OR_EXIT(false, "should never get here");
    return NULL;
}

// return an array of arg types or NULL if our param type is nil
static LLVMTypeRef *bllvm_fn_arg_types(struct rir_type *type,
                                       struct llvm_traversal_ctx *ctx)
{
    bllvm_type_to_subtype_array(type, ctx);
    return darray_size(ctx->params) == 0 ? NULL : llvm_traversal_ctx_get_params(ctx);
}

LLVMValueRef bllvm_compile_function(struct rir_function *fn,
                                    struct llvm_traversal_ctx *ctx)
{
    char *fn_name;
    char *param_name;
    RFS_PUSH();
    fn_name = rf_string_cstr_from_buff_or_die(&fn->name);
    // evaluating types here since you are not guaranteed order of execution of
    // a function's arguments and this does have sideffects we read from
    // llvm_traversal_ctx_get_param_count()
    LLVMTypeRef * types = bllvm_fn_arg_types(fn->arg_type, ctx);
    ctx->current_function = LLVMAddFunction(
        ctx->mod, fn_name,
        LLVMFunctionType(bllvm_type(fn->ret_type, ctx),
                         types,
                         llvm_traversal_ctx_get_param_count(ctx),
                         false)); // never variadic for now
    RFS_POP();

    // now handle function body
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(ctx->current_function, "entry");
    bllvm_enter_block(ctx, entry);
    // place function's argument in the stack
    unsigned int i = 0;
    LLVMValueRef allocation;
    const struct RFstring *param_name_str;
    struct symbol_table_record *rec;

    if (fn->arg_type->category != ELEMENTARY_RIR_TYPE_NIL) {
        for (i = 0; i < LLVMCountParams(ctx->current_function); ++i) {

            // for each argument of the function allocate an LLVM variable
            // in the stack with alloca
            param_name_str = rir_type_get_nth_name_or_die(fn->arg_type, i);
            RFS_PUSH();
            param_name = rf_string_cstr_from_buff_or_die(param_name_str);
            allocation = LLVMBuildAlloca(
                ctx->builder,
                bllvm_type(rir_type_get_nth_type_or_die(fn->arg_type, i), ctx),
                param_name);
            RFS_POP();
            // and assign to it the argument value
            LLVMBuildStore(ctx->builder, LLVMGetParam(ctx->current_function, i) ,allocation);
            // also note the alloca in the symbol table
            rec = symbol_table_lookup_record(fn->symbols, param_name_str, NULL);
            RF_ASSERT_OR_CRITICAL(rec,
                                  return NULL,
                                  "Symbol table of rir_function did "
                                  "not contain expected parameter");
            symbol_table_record_set_backend_handle(rec, allocation);
        }
    }

    // if the function's got a return value alloc it here
    ctx->current_function_return = NULL;
    if (fn->ret_type->category != ELEMENTARY_RIR_TYPE_NIL) {
        ctx->current_function_return = LLVMBuildAlloca(
            ctx->builder,
            bllvm_type(fn->ret_type, ctx),
            "function_return_value");
    }

    // this block should always stay at the end of the function
    LLVMBasicBlockRef function_end = LLVMAppendBasicBlock(ctx->current_function, "function_end");
    // now compile all parts of the function
    bllvm_compile_basic_block(fn->entry, ctx);

    // finally build the function return. Jump from whichever the second last block
    // was to the return block and return
    bllvm_add_br(function_end, ctx);
    bllvm_enter_block(ctx, function_end);
    if (fn->ret_type->category == ELEMENTARY_RIR_TYPE_NIL) {
        LLVMBuildRetVoid(ctx->builder);
    } else { // if we got a return value
        // I suppose in some case no load would be needed. Need to abstract these
        // differentiations somehow
        LLVMValueRef ret = LLVMBuildLoad(ctx->builder, ctx->current_function_return, "");
        LLVMBuildRet(ctx->builder, ret);
    }
    return ctx->current_function;
}

LLVMTypeRef bllvm_function_type(LLVMValueRef fn)
{
    return LLVMGetElementType(LLVMTypeOf(fn));
}
