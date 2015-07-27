#ifndef LFR_IR_RIR_EXPRESSION_H
#define LFR_IR_RIR_EXPRESSION_H

#include <RFintrusive_list.h>

struct ast_node;
struct rir_ctx;


struct rir_fncall {
    const struct RFstring *name;
    // TODO: define the fncall members
};

enum rir_expression_type {
    RIR_EXPRESSION_FNCALL,
    RIR_EXPRESSION_ALLOC,
    RIR_EXPRESSION_CONSTRUCT,
    RIR_EXPRESSION_ADD,
    RIR_EXPRESSION_SUB,
    RIR_EXPRESSION_MUL,
    RIR_EXPRESSION_DIV,
};

struct rir_expression {
    enum rir_expression_type type;
    union {
        struct rir_fncall fncall;
    };
    // Control to be added to expression list of a rir block
    struct RFilist_node ln;
};

struct rir_expression *rir_expression_create(struct ast_node *n, struct rir_ctx *ctx);
void rir_expression_destroy(struct rir_expression *expr);

#endif
