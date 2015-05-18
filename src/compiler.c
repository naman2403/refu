#include "compiler.h"

#include <refu.h>
#include <Utils/memory.h>

#include <info/info.h>
#include <types/type_comparisons.h>
#include <compiler_args.h>
#include <ast/ast.h>
#include <front_ctx.h>
#include <ir/rir.h>
#include <serializer/serializer.h>
#include <backend/llvm.h>

struct rir_module;

bool compiler_init(struct compiler *c)
{
    RF_STRUCT_ZERO(c);

    // initialize Refu library
    rf_init(LOG_TARGET_STDOUT,
            NULL,
            LOG_WARNING,
            RF_DEFAULT_TS_MBUFF_INITIAL_SIZE,
            RF_DEFAULT_TS_SBUFF_INITIAL_SIZE
    );

    // initialize an error buffer string
    if (!rf_stringx_init_buff(&c->err_buff, 1024, "")) {
        return false;
    }
    // initialize the type comparison thread local context
    if (!typecmp_ctx_init()) {
        return false;
    }

    if (!(c->args = compiler_args_create())) {
        return false;
    }

    if (!(c->serializer = serializer_create(c->args))) {
        return false;
    }

    return true;
}

void compiler_deinit(struct compiler *c)
{
    if (c->front) {
        front_ctx_destroy(c->front);
    }

    if (c->ir) {
        rir_destroy(c->ir);
    }

    serializer_destroy(c->serializer);
    compiler_args_destroy(c->args);
    typecmp_ctx_deinit();
    rf_stringx_deinit(&c->err_buff);
    rf_deinit();
}

bool compiler_pass_args(struct compiler *c, int argc, char **argv)
{
    if (!compiler_args_parse(c->args, argc, argv)) {
        return false;
    }

    // do not proceed any further if we got request for help
    if (compiler_args_help_is_requested(c->args)) {
        return true;
    }

    if (!(c->front = front_ctx_create(c->args))) {
        RF_ERROR("Failure at frontend context initialization");
        return false;
    }

    return true;
}

bool compiler_init_with_args(struct compiler *c, int argc, char **argv)
{
    if (!compiler_init(c)) {
        return false;
    }

    return compiler_pass_args(c, argc, argv);
}

bool compiler_process(struct compiler *c)
{
    struct analyzer *analyzer = front_ctx_process(c->front);
    if (!analyzer) {
        RF_ERROR("Failure to parse and analyze the input");

        // for now temporarily just dump all messages in the info context
        // TODO: fix
        if (!info_ctx_get_messages_fmt(c->front->info, MESSAGE_ANY, &c->err_buff)) {
            RF_ERROR("Could not retrieve messages from the info context");
            return false;
        }
        printf(RF_STR_PF_FMT, RF_STR_PF_ARG(&c->err_buff));
        return false;
    }

    // for now at least, if an empty file is given quit here with a message
    if (ast_node_get_children_number(analyzer->root) == 0) {
        ERROR("Provided an empty file for compilation");
        return false;
    }

    // create the intermediate representation from the analyzer and free analyzer
    c->ir = rir_create(analyzer);
    if (!c->ir) {
        RF_ERROR("Could not create the intermediate representation");
        return false;
    }

    enum serializer_rc rc = serializer_process(c->serializer, c->ir->root, c->front->file);
    if (rc == SERC_SUCCESS_EXIT || rc == SERC_SUCCESS_EXIT) {
        return rc;
    }

    if (!bllvm_generate(c->ir, c->args)) {
        RF_ERROR("Failed to create the LLVM IR from the Refu IR");
        return false;
    }

    return true;
}

bool compiler_help_requested(struct compiler *c)
{
    return compiler_args_check_and_display_help(c->args);
}
