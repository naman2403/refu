#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <String/rf_str_core.h>
#include <parser/parser.h>
#include "../../src/parser/recursive_descent/ifexpr.h"
#include <ast/ifexpr.h>
#include <ast/function.h>
#include <ast/operators.h>
#include <ast/block.h>
#include <ast/type.h>
#include <ast/vardecl.h>
#include <ast/string_literal.h>
#include <ast/constant_num.h>
#include <lexer/lexer.h>
#include <info/msg.h>

#include "../testsupport_front.h"
#include "testsupport_parser.h"

#include CLIB_TEST_HELPERS

START_TEST(test_acc_ifexpr_1) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a == 42 {\n"
        "    do_sth()\n"
        "}"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = d->front.file;

    struct ast_node *id1 = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 3);
    testsupport_parser_constant_create(cnum, file,
                                       0, 8, 0, 9, integer, 42);
    testsupport_parser_node_create(cmp_exp, binaryop, file, 0, 3, 0, 9,
                                   BINARYOP_CMP_EQ, id1, cnum);


    testsupport_parser_block_create(bnode, file, 0, 11, 2, 0);
    struct ast_node *fn_name = testsupport_parser_identifier_create(
        file,
        1, 4, 1, 9);

    testsupport_parser_node_create(fc, fncall, file, 1, 4, 1, 11, fn_name, NULL, NULL);
    ast_node_add_child(bnode, fc);

    testsupport_parser_node_create(cond, condbranch, file, 0, 3, 2, 0,
                                   cmp_exp, bnode);

    testsupport_parser_node_create(ifx, ifexpr, file, 0, 0, 2, 0, cond, NULL);

    ck_test_parse_as(n, ifexpr, d, "if_expression", ifx);

    ast_node_destroy(n);
    ast_node_destroy(ifx);
}END_TEST

START_TEST(test_acc_ifexpr_2) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a == 42 {\n"
        "    do_sth()\n"
        "} else { \n"
        "    55 + 2.31\n"
        "}"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = d->front.file;

    struct ast_node *id1 = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 3);
    testsupport_parser_constant_create(cnum1, file,
                                       0, 8, 0, 9, integer, 42);
    testsupport_parser_node_create(cmp_exp, binaryop, file, 0, 3, 0, 9,
                                   BINARYOP_CMP_EQ, id1, cnum1);


    testsupport_parser_block_create(bnode1, file, 0, 11, 2, 0);
    struct ast_node *fn_name = testsupport_parser_identifier_create(
        file,
        1, 4, 1, 9);

    testsupport_parser_node_create(fc, fncall, file, 1, 4, 1, 11, fn_name, NULL, NULL);
    ast_node_add_child(bnode1, fc);

    testsupport_parser_node_create(cond1, condbranch, file, 0, 3, 2, 0,
                                   cmp_exp, bnode1);


    testsupport_parser_block_create(bnode2, file, 2, 7, 4, 0);
    testsupport_parser_constant_create(cnum2, file,
                                       3, 4, 3, 5, integer, 55);
    testsupport_parser_constant_create(cnum3, file,
                                       3, 9, 3, 12, float, 2.31);
    testsupport_parser_node_create(op1, binaryop, file, 3, 4, 3, 12,
                                   BINARYOP_ADD, cnum2, cnum3);
    ast_node_add_child(bnode2, op1);



    testsupport_parser_node_create(ifx, ifexpr, file, 0, 0, 4, 0, cond1, bnode2);

    ck_test_parse_as(n, ifexpr, d, "if_expression", ifx);

    ast_node_destroy(n);
    ast_node_destroy(ifx);
}END_TEST

START_TEST(test_acc_ifexpr_3) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a == 42 {\n"
        "    do_sth()\n"
        "} elif (a == 50 && is_good()) {\n"
        "    \"foo\"\n"
        "} else { \n"
        "    55 + 2.31\n"
        "}"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = d->front.file;

    struct ast_node *id1 = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 3);
    testsupport_parser_constant_create(cnum1, file,
                                       0, 8, 0, 9, integer, 42);
    testsupport_parser_node_create(cmp_exp, binaryop, file, 0, 3, 0, 9,
                                   BINARYOP_CMP_EQ, id1, cnum1);


    testsupport_parser_block_create(bnode1, file, 0, 11, 2, 0);
    struct ast_node *fn_name = testsupport_parser_identifier_create(
        file,
        1, 4, 1, 9);

    testsupport_parser_node_create(fc, fncall, file, 1, 4, 1, 11, fn_name, NULL, NULL);
    ast_node_add_child(bnode1, fc);

    testsupport_parser_node_create(cond1, condbranch, file, 0, 3, 2, 0,
                                   cmp_exp, bnode1);



    struct ast_node *id2 = testsupport_parser_identifier_create(file,
                                                                2, 8, 2, 8);
    testsupport_parser_constant_create(cnum2, file,
                                       2, 13, 2, 14, integer, 50);
    testsupport_parser_node_create(op1, binaryop, file, 2, 8, 2, 14,
                                   BINARYOP_CMP_EQ, id2, cnum2);
    struct ast_node *fn_name2 = testsupport_parser_identifier_create(
        file,
        2, 19, 2, 25);
    testsupport_parser_node_create(fc2, fncall, file, 2, 19, 2, 27,
                                   fn_name2, NULL, NULL);
    testsupport_parser_node_create(cmp_exp2, binaryop, file, 2, 8, 2, 27,
                                   BINARYOP_LOGIC_AND, op1, fc2);

    testsupport_parser_block_create(bnode2, file, 2, 30, 4, 0);
    testsupport_parser_string_literal_create(sliteral1, file,
                                             3, 4, 3, 8);
    ast_node_add_child(bnode2, sliteral1);
    testsupport_parser_node_create(cond2, condbranch, file, 2, 8, 4, 0,
                                   cmp_exp2, bnode2);


    testsupport_parser_block_create(bnode3, file, 4, 7, 6, 0);
    testsupport_parser_constant_create(cnum3, file,
                                       5, 4, 5, 5, integer, 55);
    testsupport_parser_constant_create(cnum4, file,
                                       5, 9, 5, 12, float, 2.31);
    testsupport_parser_node_create(op3, binaryop, file, 5, 4, 5, 12,
                                   BINARYOP_ADD, cnum3, cnum4);
    ast_node_add_child(bnode3, op3);



    testsupport_parser_node_create(ifx, ifexpr, file, 0, 0, 6, 0, cond1, NULL);
    ast_node_add_child(ifx, cond2);
    ast_ifexpr_add_fall_through_branch(ifx, bnode3); /* adding the else branch */

    ck_test_parse_as(n, ifexpr, d, "if_expression", ifx);

    ast_node_destroy(n);
    ast_node_destroy(ifx);
}END_TEST

START_TEST(test_acc_ifexpr_4) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a == 42 {\n"
        "    do_sth()\n"
        "} elif (a == 50 && is_good()) {\n"
        "    \"foo\"\n"
        "} elif (5 < something) {\n"
        "    283.23\n"
        "} else { \n"
        "    55 + 2.31\n"
        "}"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = d->front.file;

    struct ast_node *id1 = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 3);
    testsupport_parser_constant_create(cnum1, file,
                                       0, 8, 0, 9, integer, 42);
    testsupport_parser_node_create(cmp_exp, binaryop, file, 0, 3, 0, 9,
                                   BINARYOP_CMP_EQ, id1, cnum1);


    testsupport_parser_block_create(bnode1, file, 0, 11, 2, 0);
    struct ast_node *fn_name = testsupport_parser_identifier_create(
        file,
        1, 4, 1, 9);

    testsupport_parser_node_create(fc, fncall, file, 1, 4, 1, 11, fn_name, NULL, NULL);
    ast_node_add_child(bnode1, fc);

    testsupport_parser_node_create(cond1, condbranch, file, 0, 3, 2, 0,
                                   cmp_exp, bnode1);



    struct ast_node *id2 = testsupport_parser_identifier_create(file,
                                                                2, 8, 2, 8);
    testsupport_parser_constant_create(cnum2, file,
                                       2, 13, 2, 14, integer, 50);
    testsupport_parser_node_create(op1, binaryop, file, 2, 8, 2, 14,
                                   BINARYOP_CMP_EQ, id2, cnum2);
    struct ast_node *fn_name2 = testsupport_parser_identifier_create(
        file,
        2, 19, 2, 25);
    testsupport_parser_node_create(fc2, fncall, file, 2, 19, 2, 27,
                                   fn_name2, NULL, NULL);
    testsupport_parser_node_create(cmp_exp2, binaryop, file, 2, 8, 2, 27,
                                   BINARYOP_LOGIC_AND, op1, fc2);

    testsupport_parser_block_create(bnode2, file, 2, 30, 4, 0);
    testsupport_parser_string_literal_create(sliteral1, file,
                                             3, 4, 3, 8);
    ast_node_add_child(bnode2, sliteral1);
    testsupport_parser_node_create(cond2, condbranch, file, 2, 8, 4, 0,
                                   cmp_exp2, bnode2);


    testsupport_parser_constant_create(cnum3, file,
                                       4, 8, 4, 8, integer, 5);
    struct ast_node *id3 = testsupport_parser_identifier_create(file,
                                                                4, 12, 4, 20);
    testsupport_parser_node_create(cmp_exp3, binaryop, file, 4, 8, 4, 20,
                                   BINARYOP_CMP_LT, cnum3, id3);
    testsupport_parser_block_create(bnode3, file, 4, 23, 6, 0);
    testsupport_parser_constant_create(cnum4, file,
                                       5, 4, 5, 9, float, 283.23);
    ast_node_add_child(bnode3, cnum4);
    testsupport_parser_node_create(cond3, condbranch, file, 4, 8, 6, 0,
                                   cmp_exp3, bnode3);


    testsupport_parser_block_create(bnode4, file, 6, 7, 8, 0);
    testsupport_parser_constant_create(cnum5, file,
                                       7, 4, 7, 5, integer, 55);
    testsupport_parser_constant_create(cnum6, file,
                                       7, 9, 7, 12, float, 2.31);
    testsupport_parser_node_create(op3, binaryop, file, 7, 4, 7, 12,
                                   BINARYOP_ADD, cnum5, cnum6);
    ast_node_add_child(bnode4, op3);



    testsupport_parser_node_create(ifx, ifexpr, file, 0, 0, 8, 0, cond1, NULL);
    ast_node_add_child(ifx, cond2);
    ast_node_add_child(ifx, cond3);
    ast_ifexpr_add_fall_through_branch(ifx, bnode4); /* adding the else branch */

    ck_test_parse_as(n, ifexpr, d, "if_expression", ifx);

    ast_node_destroy(n);
    ast_node_destroy(ifx);
}END_TEST


START_TEST(test_acc_ifexpr_errors_1) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if {\n"
        " do_sth()\n"
        "}"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_ifexpr(d->front.parser);
    ck_assert_msg(n == NULL, "parsing if expression should fail");

    struct info_msg errors[] = {
        TESTSUPPORT_INFOMSG_INIT_START(
            d->front.file,
            MESSAGE_SYNTAX_ERROR,
            "Expected an expression after 'if'",
            0, 1),
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_ifexpr_errors_2) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a > 25 \n"
        " do_sth()\n"
        "}"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_ifexpr(d->front.parser);
    ck_assert_msg(n == NULL, "parsing if expression should fail");

    struct info_msg errors[] = {
        TESTSUPPORT_INFOMSG_INIT_START(
            d->front.file,
            MESSAGE_SYNTAX_ERROR,
            "Expected a block after \"if\"'s conditional expression",
            0, 8),
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_ifexpr_errors_3) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a > 25 {\n"
        " do_sth()\n"
        "} else {"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_ifexpr(d->front.parser);
    ck_assert_msg(n == NULL, "parsing if expression should fail");

    struct info_msg errors[] = {
        TESTSUPPORT_INFOMSG_INIT_START(
            d->front.file,
            MESSAGE_SYNTAX_ERROR,
            "Expected an expression or a '}' at block end",
            2, 7),
        TESTSUPPORT_INFOMSG_INIT_START(
            d->front.file,
            MESSAGE_SYNTAX_ERROR,
            "Expected a block after 'else'",
            2, 5),
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_ifexpr_errors_4) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a > 25 {\n"
        " do_sth()\n"
        "} elif {\n"
        "  10 + 231\n"
        "}\n"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_ifexpr(d->front.parser);
    ck_assert_msg(n == NULL, "parsing if expression should fail");

    struct info_msg errors[] = {
        TESTSUPPORT_INFOMSG_INIT_START(
            d->front.file,
            MESSAGE_SYNTAX_ERROR,
            "Expected an expression after 'elif'",
            2, 5),
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_ifexpr_errors_5) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "if a > 25 {\n"
        " do_sth()\n"
        "} elif 55 == foo_value \n"
        "  10 + 231\n"
        "} else {\n"
        "  else_action()\n"
        "}"
    );
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_ifexpr(d->front.parser);
    ck_assert_msg(n == NULL, "parsing if expression should fail");

    struct info_msg errors[] = {
        TESTSUPPORT_INFOMSG_INIT_START(
            d->front.file,
            MESSAGE_SYNTAX_ERROR,
            "Expected a block after \"elif\"'s conditional expression",
            2, 21),
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

Suite *parser_ifexpr_suite_create(void)
{
    Suite *s = suite_create("parser_if_expression");

    TCase *ifp = tcase_create("if_expression_parsing");
    tcase_add_checked_fixture(ifp, setup_front_tests, teardown_front_tests);
    tcase_add_test(ifp, test_acc_ifexpr_1);
    tcase_add_test(ifp, test_acc_ifexpr_2);
    tcase_add_test(ifp, test_acc_ifexpr_3);
    tcase_add_test(ifp, test_acc_ifexpr_4);

    TCase *iferr = tcase_create("if_expression_parsing_errors");
    tcase_add_checked_fixture(iferr, setup_front_tests, teardown_front_tests);
    tcase_add_test(iferr, test_acc_ifexpr_errors_1);
    tcase_add_test(iferr, test_acc_ifexpr_errors_2);
    tcase_add_test(iferr, test_acc_ifexpr_errors_3);
    tcase_add_test(iferr, test_acc_ifexpr_errors_4);
    tcase_add_test(iferr, test_acc_ifexpr_errors_5);

    suite_add_tcase(s, ifp);
    suite_add_tcase(s, iferr);

    return s;
}
