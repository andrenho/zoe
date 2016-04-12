#include <stdio.h>
#include <string.h>

#include "lib/bytecode.h"
#include "lib/opcode.h"
#include "lib/hash.h"
#include "lib/zoe.h"

// {{{ TEST FRAMEWORK

#define mu_assert(message, test) \
{ \
    bool _v = (test); \
    do { \
        printf("  \033[2;3%dm[%s]\033[0m %s\n", (!_v ? 1 : 2), (!_v ? "err" : "ok"), message); \
        if(!_v) { \
            return message; \
        } \
    } while (0); \
}
#define mu_run_test(test) printf("\033[1;34m[%s]\033[0m\n", #test); \
    do { char *message = test(); tests_run++; \
    if (message) return message; } while (0)
extern int tests_run;

static double number_expr(char* expr)
{
    Zoe* Z = zoe_createvm(NULL);

    zoe_eval(Z, expr);
    zoe_call(Z, 0);
    double r = zoe_popnumber(Z);

    zoe_free(Z);

    return r;
}

static bool boolean_expr(char* expr)
{
    Zoe* Z = zoe_createvm(NULL);

    zoe_eval(Z, expr);
    zoe_call(Z, 0);
    bool r = zoe_popboolean(Z);

    zoe_free(Z);

    return r;
}

static char* string_expr(char* expr)
{
    Zoe* Z = zoe_createvm(NULL);

    zoe_eval(Z, expr);
    zoe_call(Z, 0);
    char* s = zoe_popstring(Z);

    zoe_free(Z);

    return s;
}

static char* inspect_expr(char* expr)
{
    Zoe* Z = zoe_createvm(NULL);

    zoe_eval(Z, expr);
    zoe_call(Z, 0);
    zoe_inspect(Z, -1);
    char* s = zoe_popstring(Z);
    
    zoe_free(Z);
    return s;
}

#define mu_assert_nexpr(expr, r) mu_assert(expr, number_expr(expr) == r);
#define mu_assert_bexpr(expr, r) mu_assert(expr, boolean_expr(expr) == r);
#define mu_assert_sexpr(expr, r) { char* s = string_expr(expr); mu_assert(expr, strcmp(s, r) == 0); free(s); }
#define mu_assert_inspect(expr, r) { char* s = inspect_expr(expr); mu_assert(expr, strcmp(s, r) == 0); free(s); }

#pragma GCC diagnostic ignored "-Wfloat-equal"

// }}}

// {{{ BYTECODE

static uint8_t expected[] = {
    0x90, 0x6F, 0x65, 0x20, 0xEB, 0x00, 0x01, 0x00,     // header
    0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // code_pos
    0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // code_sz
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // data_pos
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // data_sz
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // debug_pos
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // debug_sz
    POP, PUSH_N, 0xA7, 0xE8, 0x48, 0x2E, 0xFF, 0x21, 0x09, 0x40,  // PUSH_N 3.1416
    END,                                                // EOF
};

static char* test_bytecode_gen(void) 
{
    Bytecode* bc = bytecode_new(NULL);

    bytecode_addcode(bc, POP);
    bytecode_addcode(bc, PUSH_N);
    bytecode_addcodef64(bc, 3.1416);
    bytecode_addcode(bc, END);

    uint8_t* found;
    size_t sz = bytecode_generatezb(bc, &found);

    mu_assert("BZ size", sz == sizeof expected);
    for(size_t i=0; i<sz; ++i) {
        static char buf[100];
        sprintf(buf, "%zuth byte (expected: 0x%02X, found: 0x%02X)", i, expected[i], found[i]);
        mu_assert(buf, expected[i] == found[i]);
    }
    free(found);

    bytecode_free(bc);
    
    return 0;
}


static char* test_bytecode_import(void)
{
    Bytecode* bc = bytecode_newfromzb(expected, sizeof expected, NULL);

    mu_assert("version minor", bc->version_minor == 0x1);
    mu_assert("code_sz", bc->code_sz == 11);
    mu_assert("code[0]", bc->code[1] == PUSH_N);
    mu_assert("code[1]", bc->code[2] == 0xA7);
    mu_assert("code[8]", bc->code[9] == 0x40);

    bytecode_free(bc);

    return 0;
}


static char* test_bytecode_simplecode(void)
{
    Bytecode* bc = bytecode_newfromcode("3.1416", NULL);

    uint8_t* found;
    size_t sz = bytecode_generatezb(bc, &found);

    mu_assert("BZ size", sz == sizeof expected);
    for(size_t i=0; i<sz; ++i) {
        static char buf[100];
        sprintf(buf, "%zuth byte", i);
        mu_assert(buf, expected[i] == found[i]);
    }
    free(found);

    bytecode_free(bc);

    return 0;
}

// }}}

// {{{ LOW-LEVEL STACK

bool error_found = false;
static void test_error(const char* s) {
    (void) s;
    error_found = true;
}

extern ZValue* zoe_stack_pushexisting(Zoe* Z, ZValue* existing);
extern ZValue* zoe_stack_pushnew(Zoe* Z, ZType type);
extern void zoe_stack_pop(Zoe* Z);
extern ZValue* zoe_stack_get(Zoe* Z, STPOS pos);

static char* test_stack(void) 
{
    Zoe* Z = zoe_createvm(test_error);

    mu_assert("stack size == 1", zoe_stacksize(Z) == 1);

    zoe_stack_pushnew(Z, NIL);
    mu_assert("stack size == 2 (after push)", zoe_stacksize(Z) == 2);
    mu_assert("stack abs 0 = 0", zoe_absindex(Z, 0) == 0);
    mu_assert("stack abs -1 = 1", zoe_absindex(Z, -1) == 1);

    mu_assert("push & peek", zoe_stack_get(Z, -1)->type == NIL);

    zoe_stack_pop(Z);
    mu_assert("stack size == 1 (after push/pop)", zoe_stacksize(Z) == 1);
    
    mu_assert("no errors so far", !error_found);
    zoe_stack_pop(Z);
    zoe_stack_pop(Z);
    mu_assert("stack underflow", error_found);

    zoe_stack_pushnew(Z, NIL); // for testing memory leaks
    zoe_stack_pushnew(Z, NIL);
    zoe_stack_pushnew(Z, NIL);

    zoe_free(Z); // if there's a memory leak, it will happen here

    return 0;
}

// }}}

// {{{ HIGH-LEVEL STACK

static char* test_zoe_stack(void) 
{
    Zoe* Z = zoe_createvm(NULL);

    mu_assert("stack size == 1", zoe_stacksize(Z) == 1);

    double f = 3.24;
    zoe_pushnumber(Z, f);
    mu_assert("stack size == 2 (after push)", zoe_stacksize(Z) == 2);
    mu_assert("peek", zoe_peeknumber(Z) == f);
    mu_assert("pop", zoe_popnumber(Z) == f);
    mu_assert("stack size == 1 (after push/pop)", zoe_stacksize(Z) == 1);

    zoe_free(Z);
    return 0;
}


static char* test_zoe_stack_order(void)
{
    Zoe* Z = zoe_createvm(NULL);

    zoe_pushnumber(Z, 1);
    zoe_pushnumber(Z, 2);
    zoe_pushnumber(Z, 3);
    mu_assert("stack size == 4", zoe_stacksize(Z) == 4);
    mu_assert("pop == 3", zoe_popnumber(Z) == 3);
    mu_assert("pop == 3", zoe_popnumber(Z) == 2);
    mu_assert("pop == 3", zoe_popnumber(Z) == 1);
    zoe_popnil(Z);
    mu_assert("stack size == 0", zoe_stacksize(Z) == 0);

    zoe_free(Z);
    return 0;
}


static char* test_zoe_string(void)
{
    Zoe* Z = zoe_createvm(NULL);
    
    zoe_pushstring(Z, "hello world");
    mu_assert("stack size == 2 (after push)", zoe_stacksize(Z) == 2);
    mu_assert("peek", strcmp(zoe_peekstring(Z), "hello world") == 0);

    char* buf = zoe_popstring(Z);
    mu_assert("pop", strcmp(buf, "hello world") == 0);
    free(buf);

    mu_assert("stack size == 1 (after push/pop)", zoe_stacksize(Z) == 1);
    zoe_free(Z);

    return 0;
}


// }}}

// {{{ ZOE EXECUTION

static char* test_execution(void) 
{
    Zoe* Z = zoe_createvm(NULL);

    // load code
    zoe_eval(Z, "42");
    mu_assert("eval pushed into stack", zoe_stacksize(Z) == 2);
    mu_assert("type == function", zoe_peektype(Z) == FUNCTION);

    // execute code
    zoe_call(Z, 0);
    mu_assert("result pushed into stack", zoe_stacksize(Z) == 1);
    mu_assert("type == function", zoe_peektype(Z) == NUMBER);
    mu_assert("return 42", zoe_popnumber(Z) == 42);

    zoe_free(Z);

    return 0;
}

static char* test_inspect(void)
{
    Zoe* Z = zoe_createvm(NULL);

    zoe_eval(Z, "42");
    zoe_call(Z, 0);
    zoe_inspect(Z, -1);

    char* r = zoe_popstring(Z);
    mu_assert("-> 42", strcmp(r, "42") == 0);
    free(r);

    zoe_free(Z);
    return 0;
}

// }}}

// {{{ ZOE EXPRESSIONS

static char* test_math_expressions(void)
{
    mu_assert_nexpr("2 + 3", 5);
    mu_assert_nexpr("2 * 3", 6);
    mu_assert_nexpr("2 - 3", -1);
    mu_assert_nexpr("3 / 2", 1.5);
    mu_assert_nexpr("1 + 2 * 3", 7);
    mu_assert_nexpr("(1 + 2) * 3", 9);
    mu_assert_nexpr("3 %/ 2", 1);
    mu_assert_nexpr("3 % 2", 1);
    mu_assert_nexpr("-3 + 2", -1);
    mu_assert_nexpr("2 ** 3", 8);
    mu_assert_nexpr("0b11 & 0b10", 2);
    mu_assert_nexpr("0b11 | 0b10", 3);
    mu_assert_nexpr("0b11 ^ 0b10", 1);
    mu_assert_nexpr("0b1000 >> 2", 2);
    mu_assert_nexpr("0b1000 << 2", 32);
    mu_assert_nexpr("(~0b1010) & 0b1111", 5);
    mu_assert_bexpr("2 > 3", false);
    mu_assert_bexpr("2 < 3", true);
    mu_assert_bexpr("2 == 3", false);
    mu_assert_bexpr("2 != 3", true);
    mu_assert_bexpr("?3", false);
    mu_assert_bexpr("?nil", true);
    mu_assert_bexpr("!true", false);
    mu_assert_bexpr("!false", true);

    return 0;
}


static char* test_shortcircuit_expressions(void)
{
    // &&
    mu_assert_bexpr("true && true", true);
    mu_assert_bexpr("true && false", false);
    mu_assert_bexpr("false && true", false);
    mu_assert_bexpr("false && false", false);
    mu_assert_bexpr("true && true && true", true);
    mu_assert_bexpr("true && true && false", false);

    // ||
    mu_assert_bexpr("true || true", true);
    mu_assert_bexpr("true || false", true);
    mu_assert_bexpr("false || true", true);
    mu_assert_bexpr("false || false", false);
    mu_assert_bexpr("true || true || true", true);
    mu_assert_bexpr("true || true || false", true);
    mu_assert_bexpr("false || false || false", false);

    // ternary
    mu_assert_nexpr("2 < 3 ? 4 : 5", 4);
    mu_assert_nexpr("2 >= 3 ? 4 : 5", 5);

    return 0;
}

// }}}

// {{{ ZOE STRINGS

static char* test_strings(void)
{
    mu_assert_sexpr("'abc'", "abc");
    mu_assert_sexpr("'a\\nb'", "a\nb");
    mu_assert_sexpr("'a\\x41b'", "aAb");
    mu_assert_sexpr("'a\\x41b'", "aAb");
    mu_assert_sexpr("'ab'..'cd'", "abcd");
    mu_assert_sexpr("'ab'..'cd'..'ef'", "abcdef");
    mu_assert_sexpr("'a\nf'", "a\nf");  // multiline string
    mu_assert_sexpr("'a' 'b' 'cd'", "abcd");
    mu_assert_sexpr("'ab$e'", "ab$e");
    mu_assert_sexpr("'ab$'", "ab$");
    mu_assert_sexpr("'ab${'cd'}ef'", "abcdef");
    mu_assert_sexpr("'ab${'cd'}ef'\n", "abcdef");
    mu_assert_sexpr("'ab${'cd'..'xx'}ef'", "abcdxxef");
    mu_assert_nexpr("#'abcd'", 4);
    mu_assert_nexpr("#''", 0);
    mu_assert_nexpr("#'ab${'cd'..'xx'}ef'", 8);
    return 0;
}

static char* test_zoe_string_subscripts(void)
{
    mu_assert_sexpr("'abcd'[1]", "b");
    mu_assert_sexpr("'abcd'[-1]", "d");
    mu_assert_sexpr("'abcd'[1:2]", "b");
    mu_assert_sexpr("'abcd'[1:3]", "bc");
    mu_assert_sexpr("'abcd'[1:]", "bcd");
    mu_assert_sexpr("'abcd'[:3]", "abc");
    mu_assert_sexpr("'abcd'[-3:-1]", "bc");
    return 0;
}

// }}}

// {{{ ZOE COMMENTS

static char* test_comments(void)
{
    mu_assert_nexpr("2 + 3 /* test */", 5);
    mu_assert_nexpr("/* test */ 2 + 3", 5);
    mu_assert_nexpr("2 /* test */ + 3", 5);
    mu_assert_nexpr("2 /* t\ne\nst */ + 3", 5);
    mu_assert_nexpr("// test\n2 + 3", 5);
    mu_assert_nexpr("2 + 3//test\n", 5);
    mu_assert_nexpr("2 /* a /* b */ */", 2);  // nested comments
    mu_assert_nexpr("2 /* /* / */ */", 2);  // nested comments
    return 0;
}

// }}}

// {{{ ZOE ARRAYS

static char* test_array(void)
{
    mu_assert_inspect("[]", "[]");
    mu_assert_inspect("[2,3]", "[2, 3]");
    mu_assert_inspect("[2,3,]", "[2, 3]");
    mu_assert_inspect("[[1]]", "[[1]]");
    mu_assert_inspect("[2, 3, []]", "[2, 3, []]");
    mu_assert_inspect("[2, 3, ['abc', true, [nil]]]", "[2, 3, ['abc', true, [nil]]]");

    return 0;
}

static char* test_array_equality(void)
{
    mu_assert_bexpr("[]==[]", true);
    mu_assert_bexpr("[2,3,]==[2,3]", true);
    mu_assert_bexpr("[2,3,[4,'abc'],nil] == [ 2, 3, [ 4, 'abc' ], nil ]", true);
    mu_assert_bexpr("[2,3,4]==[2,3]", false);
    mu_assert_bexpr("[2,3,4]==[2,3,5]", false);
    mu_assert_bexpr("[2,3,4]!=[2,3,5]", true);

    return 0;
}

static char* test_array_access(void)
{
    mu_assert_nexpr("[2,3,4][0]", 2);
    mu_assert_nexpr("[2,3,4][1]", 3);
    mu_assert_nexpr("[2,3,4][-1]", 4);
    mu_assert_nexpr("[2,3,4][-2]", 3);
    mu_assert_sexpr("[2,3,'hello'][2]", "hello");

    // TODO - test key error

    return 0;
}

static char* test_array_slices(void)
{
    mu_assert_inspect("[2,3,4][0:1]", "[2]");
    mu_assert_inspect("[2,3,4][0:2]", "[2, 3]");
    mu_assert_inspect("[2,3,4][1:3]", "[3, 4]");
    mu_assert_inspect("[2,3,4][1:]", "[3, 4]");
    mu_assert_inspect("[2,3,4][:2]", "[2, 3]");
    mu_assert_inspect("[2,3,4][:]", "[2, 3, 4]");
    mu_assert_inspect("[2,3,4][-1:]", "[4]");
    mu_assert_inspect("[2,3,4][-2:-1]", "[3]");
    mu_assert_inspect("[2,3,4][:-2]", "[2]");

    return 0;
}

static char* test_array_operators(void)
{
    mu_assert_nexpr("#[2, 3, 4]", 3);
    mu_assert_nexpr("#[]", 0);
    mu_assert_inspect("[2,3]..[4,5,6]", "[2, 3, 4, 5, 6]");
    mu_assert_inspect("[2,3] * 3", "[2, 3, 2, 3, 2, 3]");

    return 0;
}

// }}}

// {{{ HASHES

static char* test_hash(void)
{
    Zoe* Z = zoe_createvm(NULL);
    zoe_pushstring(Z, "hello");
    zoe_pushstring(Z, "world");
    zoe_pushnumber(Z, 13);
    zoe_pushnumber(Z, 24);

    Hash* h = hash_new(Z);

    hash_set(h, zoe_stack_get(Z, -1), zoe_stack_get(Z, -2));  // 24 = 13
    hash_set(h, zoe_stack_get(Z, -3), zoe_stack_get(Z, -4));  // world = hello
    hash_set(h, zoe_stack_get(Z, -4), zoe_stack_get(Z, -2));  // hello = 13

    mu_assert("fetch #1", hash_get(h, zoe_stack_get(Z, -1)) == zoe_stack_get(Z, -2));
    mu_assert("fetch #2", hash_get(h, zoe_stack_get(Z, -3)) == zoe_stack_get(Z, -4));
    mu_assert("fetch #3", hash_get(h, zoe_stack_get(Z, -4)) == zoe_stack_get(Z, -2));

    mu_assert("fetch #4 - key error", hash_get(h, zoe_stack_get(Z, -2)) == NULL);

    hash_del(h, zoe_stack_get(Z, -1));
    mu_assert("fetch #5 - key error after delete", hash_get(h, zoe_stack_get(Z, -1)) == NULL);

    hash_free(h);
    zoe_free(Z);
    return 0;
}

static char* test_hash_update(void)
{
    Zoe* Z = zoe_createvm(NULL);
    zoe_pushnumber(Z, 13);
    zoe_pushnumber(Z, 24);

    Hash* h = hash_new(Z);

    hash_set(h, zoe_stack_get(Z, -1), zoe_stack_get(Z, -2));  // 24 = 13
    hash_set(h, zoe_stack_get(Z, -1), zoe_stack_get(Z, -1));  // 24 = 24
    mu_assert("verify updated", hash_get(h, zoe_stack_get(Z, -1)) == zoe_stack_get(Z, -1));
    
    hash_free(h);
    zoe_free(Z);
    return 0;
}

static char* test_hash_grow_shrink(void) 
{
    Zoe* Z = zoe_createvm(NULL);
    Hash* h = hash_new(Z);

    for(int i=30; i>=0; --i) {
        zoe_pushnumber(Z, i);
        hash_set(h, zoe_stack_get(Z, -1), zoe_stack_get(Z, -1));
        zoe_pop(Z, 1);
    }
    zoe_pushnumber(Z, 28);
    ZValue* v = hash_get(h, zoe_stack_get(Z, -1));
    mu_assert("sanity check #1", v);
    mu_assert("sanity check #2", v->type == NUMBER);
    mu_assert("sanity check #3", v->number == 28);
    zoe_pop(Z, 1);

    mu_assert("hash should have tripled by now", hash_buckets(h) == 64);
    
    hash_free(h);
    zoe_free(Z);
    return 0;
}

static char* test_hash_stress(void)
{
    Zoe* Z = zoe_createvm(NULL);
    Hash* h = hash_new(Z);

    for(int i=30000; i>=0; --i) {
        zoe_pushnumber(Z, i);
        hash_set(h, zoe_stack_get(Z, -1), zoe_stack_get(Z, -1));
        zoe_pop(Z, 1);
    }
    zoe_pushnumber(Z, 23456);
    ZValue* v = hash_get(h, zoe_stack_get(Z, -1));
    mu_assert("sanity check #1", v);
    mu_assert("sanity check #2", v->type == NUMBER);
    mu_assert("sanity check #3", v->number == 23456);
    zoe_pop(Z, 1);

    mu_assert("expected hash size = 65536", hash_buckets(h) == 65536);
    
    hash_free(h);
    zoe_free(Z);
    return 0;
}

// }}}

// {{{ TABLES

static char* test_table(void)
{
    mu_assert_inspect("%{}", "%{}");
    mu_assert_inspect("%{hello: 'world'}", "%{hello: 'world'}");
    mu_assert_inspect("%{hello: 'world',}", "%{hello: 'world'}");
    mu_assert_inspect("%{b: %{a:1}}", "%{b: %{a: 1}}");
    mu_assert_inspect("%{hello: []}", "%{hello: []}");
    mu_assert_inspect("%{[2]: 3, abc: %{d: 3}}", "%{[2]: 3, abc: %{d: 3}}"); 

    return 0;
}

static char* test_table_access(void)
{
    mu_assert_sexpr("%{hello: 'world', a: 42}['hello']", "world");
    mu_assert_sexpr("%{hello: 'world', a: 42}.hello", "world");
    mu_assert_nexpr("%{hello: 'world', a: 42}.a", 42);
    mu_assert_nexpr("%{hello: %{world: 42}}.hello.world", 42);
    mu_assert_nexpr("%{hello: %{world: 42}}['hello']['world']", 42);

    Zoe* Z = zoe_createvm(test_error);
    error_found = false;
    zoe_eval(Z, "%{hello: 'world'}.a");
    zoe_call(Z, 0);
    mu_assert("key error", error_found);
    zoe_free(Z);

    return 0;
}

static char* test_table_equality(void)
{
    mu_assert_bexpr("%{}==%{}", true);
    mu_assert_bexpr("%{hello: 'world'}==%{hello: 'world'}", true);
    mu_assert_bexpr("%{b: %{a:1}}==%{b: %{a:1}}", true);
    mu_assert_bexpr("%{[2]: 3, abc: %{d: %{e: 42}} }==%{[2]: 3, abc: %{d: %{e: 42} }}", true);
    mu_assert_bexpr("%{a: 1, b: 2} == %{b: 2, a: 1}", true);
    mu_assert_bexpr("%{}==%{hello: 'world'}", false);
    mu_assert_bexpr("%{hello: 'world'}==%{}", false);
    mu_assert_bexpr("%{b: %{a:1}}==%{b: 1}", false);
    mu_assert_bexpr("%{b: %{a:1}}==%{b: %{a:2}}", false);
    mu_assert_bexpr("%{b: %{a:1}}==%{b: %{c:1}}", false);

    return 0;
}

// }}}

// {{{ LOCAL VARIABLES

static char* test_local_vars(void)
{
    mu_assert_nexpr("let a = 4", 4);
    mu_assert_nexpr("let a = 4; a", 4);
    mu_assert_nexpr("let a = 4; let b = 25; a", 4);
    mu_assert_nexpr("let a = 4; let b = 25; b", 25);
    mu_assert_nexpr("let a = let b = 25; a", 25);
    mu_assert_nexpr("let a = let b = 25; b", 25);
    mu_assert_nexpr("let a = 25; let b = a; b", 25);
    mu_assert_nexpr("let a = 25; let b = a; let c = b; c", 25);
    mu_assert_nexpr("let a = 25, b = 13; a", 25);
    mu_assert_nexpr("let a = 25, b = 13, c = 48; b", 13);
    mu_assert_nexpr("let a = 25, b = a, c = b; b", 25);

    return 0;
}

static char* test_multiple_assignment(void)
{
    mu_assert_nexpr("let [a, b] = [3, 4]; a", 3);
    mu_assert_nexpr("let [a, b, c] = [3, 4, 5]; b", 4);
    mu_assert_nexpr("let x = 8, [a, b, c] = [3, x, 5]; b", 8);
    mu_assert_nexpr("let [a, b, c] = [3, 4, 5], x = b; x", 4);
    
    // TODO - test errors

    return 0;
}

// }}}

// {{{ SCOPES

static char* test_scopes(void)
{
    mu_assert_nexpr("{ 4 }", 4);
    mu_assert_nexpr("{ 4; 5 }", 5);
    mu_assert_nexpr("{ 4; 5; }", 5);
    mu_assert_nexpr("{ 4\n 5 }", 5);
    mu_assert_nexpr("{ 4; { 5; } }", 5);
    mu_assert_nexpr("{ 4; { 5; { 6; 7 } } }", 7);
    mu_assert_nexpr("{ 4; { 5; { 6; 7 } } }", 7);
    mu_assert_nexpr("{ 4; { 5; { 6; 7 } } }", 7);
    // mu_assert_nexpr("{ 4\n { 5; { 6; 7;; } } } 8", 8);
    //mu_assert_nexpr("{ 4 } 5", 5);
    mu_assert_nexpr("{ 4 }; 5", 5);
    mu_assert_nexpr("{ \n 4 \n }\n 5", 5);

    return 0;
}

static char* test_scope_vars(void)
{
    mu_assert_nexpr("let a = 4; { 4 }; a", 4);

    return 0;
}

// }}}

static char* all_tests(void)
{
    /*
    mu_run_test(test_bytecode_gen);
    mu_run_test(test_bytecode_import);
    mu_run_test(test_bytecode_simplecode);
    */
    mu_run_test(test_stack);
    mu_run_test(test_zoe_stack);
    mu_run_test(test_zoe_stack_order);
    mu_run_test(test_zoe_string);
    mu_run_test(test_zoe_string_subscripts);
    mu_run_test(test_execution);
    mu_run_test(test_inspect);
    mu_run_test(test_math_expressions);
    mu_run_test(test_shortcircuit_expressions);
    mu_run_test(test_strings);
    mu_run_test(test_comments);
    mu_run_test(test_array);
    mu_run_test(test_array_equality);
    mu_run_test(test_array_access);
    mu_run_test(test_array_slices);
    mu_run_test(test_array_operators);
    mu_run_test(test_hash);
    mu_run_test(test_hash_update);
    mu_run_test(test_hash_grow_shrink);
    mu_run_test(test_hash_stress);
    mu_run_test(test_table);
    mu_run_test(test_table_access);
    mu_run_test(test_table_equality);
    mu_run_test(test_local_vars);
    mu_run_test(test_multiple_assignment);
    mu_run_test(test_scopes);
    mu_run_test(test_scope_vars);
    return 0;
}

// {{{ TEST MANAGEMENT

int tests_run = 0;

int main(void) 
{
    char *result = all_tests();
    if(result != 0) {
        printf("\033[1;31merror: %s\033[0m\n", result);
    } else {
        printf("\033[1;32m.-----------------------.\033[0m\n");
        printf("\033[1;32m|   ALL TESTS PASSED!   |\033[0m\n");
        printf("\033[1;32m'-----------------------'\033[0m\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}

// }}}

// vim: ts=4:sw=4:sts=4:expandtab:foldmethod=marker:syntax=c