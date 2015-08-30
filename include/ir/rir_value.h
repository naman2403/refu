#ifndef LFR_IR_RIR_VALUE_H
#define LFR_IR_RIR_VALUE_H

#include <String/rf_str_decl.h>
#include <ast/constants_decls.h>

struct rir;
struct rir_ctx;
struct rir_expression;

enum rir_valtype {
    RIR_VALUE_CONSTANT,
    RIR_VALUE_VARIABLE,
    RIR_VALUE_LABEL,
    RIR_VALUE_NIL
};

struct rir_value {
    enum rir_valtype type;
    struct RFstring id;
    union {
        struct ast_constant constant;
        struct rir_expression *expr;
        struct rir_block *label_dst;
    };
};

bool rir_value_init(struct rir_value *v, enum rir_valtype type, void *obj, struct rir_ctx *ctx);
bool rir_value_label_init_string(struct rir_value *v, struct rir_block *b, const struct RFstring *s, struct rir_ctx *ctx);

struct rir_value *rir_value_create(enum rir_valtype type, struct rir_expression *e, struct rir_ctx *ctx);
bool rir_value_constant_init(struct rir_value *v, const struct ast_constant *c);

void rir_value_deinit(struct rir_value *v);
void rir_value_destroy(struct rir_value *v);

bool rir_value_tostring(struct rir *r, const struct rir_value *v);
const struct RFstring *rir_value_string(const struct rir_value *v);
#endif
