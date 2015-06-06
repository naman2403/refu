#include <backend/llvm.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Transforms/Scalar.h>

#include <String/rf_str_core.h>
#include <System/rf_system.h>
#include <Persistent/buffers.h>

#include <info/info.h>
#include <analyzer/analyzer.h>
#include <compiler_args.h>
#include <front_ctx.h>

#include "llvm_ast.h"
#include "llvm_utils.h"


static inline void llvm_traversal_ctx_init(struct llvm_traversal_ctx *ctx,
                                           struct compiler_args *args)
{
    ctx->mod = NULL;
    ctx->current_st = NULL;
    ctx->current_function = NULL;
    ctx->args = args;
    ctx->builder = LLVMCreateBuilder();
    darray_init(ctx->params);
    darray_init(ctx->values);
    rir_types_map_init(&ctx->types_map);
}

static inline void llvm_traversal_ctx_deinit(struct llvm_traversal_ctx *ctx)
{
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->mod);
    LLVMDisposeTargetData(ctx->target_data);
}

static inline void llvm_traversal_ctx_set_singlepass(struct llvm_traversal_ctx *ctx,
                                                     struct analyzer *a)
{
    ctx->a = a;
    ctx->mod = NULL;
    ctx->current_st = NULL;
    ctx->current_function = NULL;;
    darray_init(ctx->params);
    darray_init(ctx->values);
    rir_types_map_init(&ctx->types_map);
}

static inline void llvm_traversal_ctx_reset_singlepass(struct llvm_traversal_ctx *ctx)
{
    ctx->a = NULL;
    rir_types_map_deinit(&ctx->types_map);
    darray_free(ctx->params);
    darray_free(ctx->values);
}

static bool bllvm_ir_generate(struct RFilist_head *fronts, struct compiler_args *args)
{
    struct llvm_traversal_ctx ctx;
    struct LLVMOpaqueModule *llvm_module;
    struct LLVMOpaqueModule *main_module;
    bool ret = false;
    char *error = NULL; // Used to retrieve messages from functions

    LLVMInitializeCore(LLVMGetGlobalPassRegistry());
    LLVMInitializeNativeTarget();

    struct front_ctx *front;
    bool index = 0;

    llvm_traversal_ctx_init(&ctx, args);
    rf_ilist_for_each(fronts, front, ln) {
        llvm_traversal_ctx_set_singlepass(&ctx, front->analyzer);
        llvm_module = blvm_create_module(front->analyzer->root, &ctx);
        if (!llvm_module) {
            ERROR("Failed to form the LLVM IR ast");
            llvm_traversal_ctx_reset_singlepass(&ctx);
            goto end;
        }
        llvm_traversal_ctx_reset_singlepass(&ctx);

        // verify module and create code
        if (!LLVMVerifyModule(llvm_module, LLVMAbortProcessAction, &error)) {
            bllvm_error("Could not verify LLVM module", error);
            goto end;
        }
        LLVMDisposeMessage(error);

        // link all other modules to the first one
        if (index == 0) {
            main_module = llvm_module;
        } else {
            if (!LLVMLinkModules(main_module, llvm_module, LLVMLinkerDestroySource, &error)) {
                bllvm_error("Could not link LLVM modules", error);
                goto end;
            }
            LLVMDisposeMessage(error);
        }

        ++index;
    }


    RFS_PUSH();
    struct RFstring *temp_s = RFS_NT_OR_DIE(
        RF_STR_PF_FMT".ll",
        RF_STR_PF_ARG(compiler_args_get_executable_name(args)));
    if (0 != LLVMPrintModuleToFile(main_module, rf_string_data(temp_s), &error)) {
        bllvm_error("Could not output LLVM module to file", error);
        goto end_pop_rfs;
    }
    LLVMDisposeMessage(error);
    llvm_traversal_ctx_deinit(&ctx);
    ret = true;

end_pop_rfs:
    RFS_POP();
end:
    LLVMShutdown();
    return ret;
}

static bool transformation_step_do(struct compiler_args *args,
                                   const char *executable,
                                   const char *insuff,
                                   const char *outsuff)
{
    int rc;
    FILE *proc;
    struct RFstring *inname;
    struct RFstring *cmd;
    const struct RFstring* output = compiler_args_get_executable_name(args);
    bool ret = true;
    RFS_PUSH();

    inname = RFS(RF_STR_PF_FMT".%s", RF_STR_PF_ARG(output), insuff);
    cmd = RFS(
        "%s "RF_STR_PF_FMT" -o "RF_STR_PF_FMT".%s",
        executable,
        RF_STR_PF_ARG(inname),
        RF_STR_PF_ARG(output),
        outsuff);
    proc = rf_popen(cmd, "r");

    if (!proc) {
        ret = false;
        goto end;
    }

    rc = rf_pclose(proc);
    if (0 != rc) {
        ERROR("%s failed with error code: %d", executable, rc);
        ret = false;
        goto end;
    }

    // delete no longer needed input name file
    rf_system_delete_file(inname);
    fflush(stdout);
end:
    RFS_POP();
    return ret;
}

static bool bllvm_ir_to_asm(struct compiler_args *args)
{
    return transformation_step_do(args, "llc", "ll", "s");
}

static bool backend_asm_to_exec(struct compiler_args *args)
{
    return transformation_step_do(args, "gcc", "s", "exe");
}

bool bllvm_generate(struct RFilist_head *fronts, struct compiler_args *args)
{

    if (!bllvm_ir_generate(fronts, args)) {
        return false;
    }

    if (!bllvm_ir_to_asm(args)) {
        ERROR("Failed to generate assembly from LLVM IR code");
        return false;
    }

    if (!backend_asm_to_exec(args)) {
        ERROR("Failed to generate executable from assembly machine code");
        return false;
    }

    return true;
}
