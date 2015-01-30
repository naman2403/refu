#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <String/rf_str_core.h>
#include <lexer/lexer.h>

#include "../testsupport_front.h"
#include "testsupport_lexer.h"

#include CLIB_TEST_HELPERS

START_TEST(test_lexer_scan_tokens_1) {
    struct front_ctx *front;
    struct inpfile *f;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "asd { }");
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    f = front->file;
    lex = front->lexer;
    struct token expected[] = {
        TESTLEX_IDENTIFIER_INIT(d, 0, 0, 0, 2, "asd"),
        {
            .type=TOKEN_SM_OCBRACE,
            .location=LOC_INIT(f, 0, 4, 0, 4)
        },
        {
            .type=TOKEN_SM_CCBRACE,
            .location=LOC_INIT(f, 0, 6, 0, 6)
        }
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);

} END_TEST

START_TEST(test_lexer_scan_tokens_2) {
    struct front_ctx *front;
    struct inpfile *f;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "type foo { a:i32, b:string | c:f32 }\n"
        "fn foo(a:int) -> int\n"
        "+\n"
        "-\n"
        "*\n"
        "/\n"
        "==\n"
        "!=\n"
        ">\n"
        ">=\n"
        "<\n"
        "<=\n"
        "&&\n"
        "||\n"
        "return\n"
        "^\n"
        ".\n"
    );
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    f = front->file;
    lex = front->lexer;
    struct token expected[] = {
        {
            .type=TOKEN_KW_TYPE,
            .location=LOC_INIT(f, 0, 0, 0, 3)
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 5, 0, 7, "foo"),
        {
            .type=TOKEN_SM_OCBRACE,
            .location=LOC_INIT(f, 0, 9, 0, 9)
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 11, 0, 11, "a"),
        {
            .type=TOKEN_SM_COLON,
            .location=LOC_INIT(f, 0, 12, 0, 12)
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 13, 0, 15, "i32"),
        {
            .type=TOKEN_OP_COMMA,
            .location=LOC_INIT(f, 0, 16, 0, 16)
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 18, 0, 18, "b"),
        {
            .type=TOKEN_SM_COLON,
            .location=LOC_INIT(f, 0, 19, 0, 19)
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 20, 0, 25,  "string"),
        {
            .type=TOKEN_OP_TYPESUM,
            .location=LOC_INIT(f, 0, 27, 0, 27)
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 29, 0, 29, "c"),
        {
            .type=TOKEN_SM_COLON,
            .location=LOC_INIT(f, 0, 30, 0, 30)
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 31, 0, 33, "f32"),
        {
            .type=TOKEN_SM_CCBRACE,
            .location=LOC_INIT(f, 0, 35, 0, 35)
        },
        /* 2nd line */
        {
            .type=TOKEN_KW_FUNCTION,
            .location=LOC_INIT(f, 1, 0, 1, 1)
        },
        TESTLEX_IDENTIFIER_INIT(d, 1, 3, 1, 5, "foo"),
        {
            .type=TOKEN_SM_OPAREN,
            .location=LOC_INIT(f, 1, 6, 1, 6)
        },
        TESTLEX_IDENTIFIER_INIT(d, 1, 7, 1, 7, "a"),
        {
            .type=TOKEN_SM_COLON,
            .location=LOC_INIT(f, 1, 8, 1, 8)
        },
        TESTLEX_IDENTIFIER_INIT(d, 1, 9, 1, 11, "int"),
        {
            .type=TOKEN_SM_CPAREN,
            .location=LOC_INIT(f, 1, 12, 1, 12)
        },
        {
            .type=TOKEN_OP_IMPL,
            .location=LOC_INIT(f, 1, 14, 1, 15)
        },
        TESTLEX_IDENTIFIER_INIT(d, 1, 17, 1, 19, "int"),
        /* 3rd line */
        {
            .type=TOKEN_OP_PLUS,
            .location=LOC_INIT(f, 2, 0, 2, 0)
        },
        /* 4th line */
        {
            .type=TOKEN_OP_MINUS,
            .location=LOC_INIT(f, 3, 0, 3, 0)
        },
        /* 5th line */
        {
            .type=TOKEN_OP_MULTI,
            .location=LOC_INIT(f, 4, 0, 4, 0)
        },
        /* 6th line */
        {
            .type=TOKEN_OP_DIV,
            .location=LOC_INIT(f, 5, 0, 5, 0)
        },
        /* 7th line */
        {
            .type=TOKEN_OP_EQ,
            .location=LOC_INIT(f, 6, 0, 6, 1)
        },
        /* 8th line */
        {
            .type=TOKEN_OP_NEQ,
            .location=LOC_INIT(f, 7, 0, 7, 1)
        },
        /* 9th line */
        {
            .type=TOKEN_OP_GT,
            .location=LOC_INIT(f, 8, 0, 8, 0)
        },
        /* 10th line */
        {
            .type=TOKEN_OP_GTEQ,
            .location=LOC_INIT(f, 9, 0, 9, 1)
        },
        /* 11th line */
        {
            .type=TOKEN_OP_LT,
            .location=LOC_INIT(f, 10, 0, 10, 0)
        },
        /* 12th line */
        {
            .type=TOKEN_OP_LTEQ,
            .location=LOC_INIT(f, 11, 0, 11, 1)
        },
        /* 13th line */
        {
            .type=TOKEN_OP_LOGIC_AND,
            .location=LOC_INIT(f, 12, 0, 12, 1)
        },
        /* 14th line */
        {
            .type=TOKEN_OP_LOGIC_OR,
            .location=LOC_INIT(f, 13, 0, 13, 1)
        },
        /* 15th line */
        {
            .type=TOKEN_KW_RETURN,
            .location=LOC_INIT(f, 14, 0, 14, 5)
        },
        /* 16th line */
        {
            .type=TOKEN_OP_BITWISE_XOR,
            .location=LOC_INIT(f, 15, 0, 15, 0)
        },
        /* 17th line */
        {
            .type=TOKEN_OP_MEMBER_ACCESS,
            .location=LOC_INIT(f, 16, 0, 16, 0)
        },
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);

} END_TEST

START_TEST(test_lexer_scan_tokens_crammed) {
    struct front_ctx *front;
    struct inpfile *f;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "food<>||");
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    f = front->file;
    lex = front->lexer;
    struct token expected[] = {
        TESTLEX_IDENTIFIER_INIT(d, 0, 0, 0, 3, "food"),
        {
            .type=TOKEN_OP_LT,
            .location=LOC_INIT(f, 0, 4, 0, 4)
        },
        {
            .type=TOKEN_OP_GT,
            .location=LOC_INIT(f, 0, 5, 0, 5)
        },
        {
            .type=TOKEN_OP_LOGIC_OR,
            .location=LOC_INIT(f, 0, 6, 0, 7)
        }
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);

} END_TEST

START_TEST(test_lexer_scan_constant_numbers) {
    struct front_ctx *front;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "42\n"
        "3.14\n"
        "0b11100\n"
        "0xFEFfE\n"
        "03452623\n"
        "1.0e-10\n"
        "3.9265E+2\n"
    );
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    lex = front->lexer;
    struct token expected[] = {
        TESTLEX_INTEGER_INIT(d, 0, 0, 0, 1, 42),
        TESTLEX_FLOAT_INIT(d, 1, 0, 1, 3, 3.14),
        TESTLEX_INTEGER_INIT(d, 2, 0, 2, 6, 28),
        TESTLEX_INTEGER_INIT(d, 3, 0, 3, 6, 1044478),
        TESTLEX_INTEGER_INIT(d, 4, 0, 4, 7, 939411),
        TESTLEX_FLOAT_INIT(d, 5, 0, 5, 6, 1.0e-10),
        TESTLEX_FLOAT_INIT(d, 6, 0, 6, 8, 3.9265e+2),
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);

} END_TEST

START_TEST(test_lexer_scan_string_literals) {
    struct front_ctx *front;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "\"Celka\"\n"
        "\"Containing escaped \\\"\\\" quotes\"\n"
        "\"Eleos そう思いながらも\"\n"
    );
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file");
    struct token expected[] = {
        TESTLEX_LITERAL_INIT(d, 0, 0, 0, 6, 0, 6,  "Celka"),
        TESTLEX_LITERAL_INIT(d, 1, 0, 1, 31, 0, 31,
                             "Containing escaped \\\"\\\" quotes"),
        TESTLEX_LITERAL_INIT(d, 2, 0, 2, 15, 0, 31, "Eleos そう思いながらも")
    };
    ck_assert_lexer_scan(d, "Scanning failed");
    check_lexer_tokens(d->front.lexer, expected);

} END_TEST

/* -- lexer scan edge cases tests -- */

START_TEST(test_lexer_scan_identifier_at_end) {
    struct front_ctx *front;
    struct inpfile *f;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT("<Type a bbb");
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    f = front->file;
    lex = front->lexer;
    struct token expected[] = {
        {
            .type=TOKEN_OP_LT,
            .location=LOC_INIT(f, 0, 0, 0, 0),
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 1, 0, 4, "Type"),
        TESTLEX_IDENTIFIER_INIT(d, 0, 6, 0, 6, "a"),
        TESTLEX_IDENTIFIER_INIT(d, 0, 8, 0, 10, "bbb"),
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);

} END_TEST

START_TEST(test_lexer_scan_problematic_typeclass) {
    struct front_ctx *front;
    struct inpfile *f;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "class pointers {\n"
        "fn dosth(\n"
        "}");
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    f = front->file;
    lex = front->lexer;
    struct token expected[] = {
        {
            .type=TOKEN_KW_TYPECLASS,
            .location=LOC_INIT(f, 0, 0, 0, 4),
        },
        TESTLEX_IDENTIFIER_INIT(d, 0, 6, 0, 13, "pointers"),
        {
            .type=TOKEN_SM_OCBRACE,
            .location=LOC_INIT(f, 0, 15, 0, 15),
        },
        {
            .type=TOKEN_KW_FUNCTION,
            .location=LOC_INIT(f, 1, 0, 1, 1),
        },
        TESTLEX_IDENTIFIER_INIT(d, 1, 3, 1, 7, "dosth"),
        {
            .type=TOKEN_SM_OPAREN,
            .location=LOC_INIT(f, 1, 8, 1, 8),
        },
        {
            .type=TOKEN_SM_CCBRACE,
            .location=LOC_INIT(f, 2, 0, 2, 0),
        },
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);

} END_TEST

START_TEST(test_lexer_scan_constant_int_at_end) {
    struct front_ctx *front;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT("13 2462");
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    lex = front->lexer;
    struct token expected[] = {
        TESTLEX_INTEGER_INIT(d, 0, 0, 0, 1, 13),
        TESTLEX_INTEGER_INIT(d, 0, 3, 0, 6, 2462),
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);
} END_TEST

START_TEST(test_lexer_scan_constant_float_at_end) {
    struct front_ctx *front;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT("13 0.142");
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    lex = front->lexer;
    struct token expected[] = {
        TESTLEX_INTEGER_INIT(d, 0, 0, 0, 1, 13),
        TESTLEX_FLOAT_INIT(d, 0, 3, 0, 7, 0.142),
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);
} END_TEST

START_TEST(test_lexer_scan_string_literal_at_end) {
    struct front_ctx *front;
    struct lexer *lex;
    static const struct RFstring s = RF_STRING_STATIC_INIT("21 \"Berlin\"");
    struct front_testdriver *d = get_front_testdriver();
    front = front_testdriver_assign(d, &s);
    ck_assert_msg(front, "Failed to assign string to file ");
    lex = front->lexer;
    struct token expected[] = {
        TESTLEX_INTEGER_INIT(d, 0, 0, 0, 1, 21),
        TESTLEX_LITERAL_INIT(d, 0, 3, 0, 10, 3, 10, "Berlin"),
    };
    ck_assert(lexer_scan(lex));
    check_lexer_tokens(lex, expected);
} END_TEST


Suite *lexer_suite_create(void)
{
    Suite *s = suite_create("lexer");

    TCase *scan = tcase_create("lexer_scan");
    tcase_add_checked_fixture(scan,
                              setup_front_tests,
                              teardown_front_tests);
    tcase_add_test(scan, test_lexer_scan_tokens_1);
    tcase_add_test(scan, test_lexer_scan_tokens_2);
    tcase_add_test(scan, test_lexer_scan_tokens_crammed);
    tcase_add_test(scan, test_lexer_scan_constant_numbers);
    tcase_add_test(scan, test_lexer_scan_string_literals);

    TCase *scan_edge = tcase_create("lexer_scan_edge_cases");
    tcase_add_checked_fixture(scan_edge,
                              setup_front_tests,
                              teardown_front_tests);
    tcase_add_test(scan_edge, test_lexer_scan_identifier_at_end);
    tcase_add_test(scan_edge, test_lexer_scan_problematic_typeclass);
    tcase_add_test(scan_edge, test_lexer_scan_constant_int_at_end);
    tcase_add_test(scan_edge, test_lexer_scan_constant_float_at_end);
    tcase_add_test(scan_edge, test_lexer_scan_string_literal_at_end);

    suite_add_tcase(s, scan);
    suite_add_tcase(s, scan_edge);
    return s;
}


