%{

// ignore warnings from G++
#if !__llvm__  // warnings not supported by clang++
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#else
#  pragma clang diagnostic ignored "-Wunreachable-code"
#endif
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-declarations"

#include <cmath>
#include <cstdint>

#include <limits>
#include <iostream>
using namespace std;

#include "compiler/bytecode.hh"
#include "compiler/parser.hh"
#include "compiler/lexer.hh"
#include "vm/exceptions.hh"

/* 
 * PROTOTYPES
 */
static void add_number(Bytecode& b, double num);
void yyerror(YYLTYPE* yylloc, void* scanner, Bytecode& b, const char *s) __attribute__((noreturn));

%}

/* make the perser reentrant */
%define api.pure full
%locations

/* since we are operating reentrant, we need to pass the scanner state around */
%param { void* scanner }

/* we need to pass a Bytecode object around to fill it */
%parse-param { Bytecode& b }

/* define debugging output */
%verbose
%error-verbose
%printer { fprintf(yyoutput, "%f", $$); } NUMBER;
%printer { fprintf(yyoutput, "%s", $$ ? "true" : "false"); } BOOLEAN;
%printer { fprintf(yyoutput, "'%s'", $$->c_str()); } STRING;
%printer { fprintf(yyoutput, "%s", $$->c_str()); } IDENTIFIER;

/* start parsing by this token */
%start code

/*
 * DATA STRUCTURE
 */
%union {
    double       number;
    bool         boolean;
    size_t       integer;
    uint8_t      u8;
    std::string* str;
    Label        label;
}

%token <number>  NUMBER
%token <boolean> BOOLEAN
%token <str>     STRING IDENTIFIER
%token NIL SEP ENV _MUT _PUB _DEL

%type <str> string strings
%type <integer> array_items table_items table_items_x     /* $$ is a counter */
%type <u8> properties                                     /* $$ is a TableConfig instance */

%nonassoc '='
%precedence '['
%left '.'

%%

//
// CODE
//
code: optsep exps optsep
    | SEP
    ;

optsep: %empty
      | SEP
      ;

exps: exps SEP { b.Add(POP); } exp
    |          { b.Add(POP); } exp
    ;


//
// EXPRESSIONS
//
exp: literal_exp
   | array_init
   | table_init
   | ENV                { b.Add(PENV); }
   | set_op
   | get_op
   | '{' { b.Add(PSHS); b.Add(PNIL); } code '}' { b.Add(POPS); }
   | '{' '}'
   ;


// 
// LITERAL EXPRESSIONS
//
literal_exp: NIL        { b.Add(PNIL); }
           | NUMBER     { add_number(b, $1); }
           | BOOLEAN    { b.Add($1 ? PBT : PBF); }
           | strings    { b.Add(PSTR, *$1); delete $1; }
           ;

string: STRING
      ;

strings: string             { $$ = new string(*$1); delete $1; }
       | string strings     { $$ = new string(*$1 + *$2); delete $1; delete $2; }
       ;

// 
// ARRAY INITIALIZATION
//
array_init: '[' array_items opt_comma ']' { b.Add(PARY, static_cast<uint16_t>($2)); }
          ;

opt_comma: %empty
         | ','
         ;

array_items: %empty              { $$ = 0; }
           | exp                 { $$ = 1; }
           | array_items ',' exp { ++$$; }
           ;

//
// TABLE INITIALIZATION
//
table_init: '%' opt_identifier '{' table_items opt_comma '}' 
              { b.Add(PTBL, static_cast<uint16_t>($table_items)); }
          | '&' opt_identifier '{' table_items_x opt_comma '}'
              { b.Add(PTBX, static_cast<uint16_t>($table_items_x)); }
          ;

opt_identifier: %empty     { b.Add(PNIL); }
              | '[' exp ']'
              ;

table_items: %empty                      { $$ = 0; }
           | table_item                  { $$ = 1; }
           | table_items ',' table_item  { ++$$; }
           ;

table_item: properties IDENTIFIER { b.Add(PSTR, *$2); delete $2; b.Add(PN8, $1); } ':' exp
          | properties '[' exp ']' { b.Add(PN8, $1); } ':' exp  
          ;

properties: %empty          { $$ = 0; }
          | _MUT properties { $$ = MUT | $2; }
          | _PUB properties { $$ = PUB | $2; }
          ;

table_items_x: %empty                          { $$ = 0; }
             | table_item_x                    { $$ = 1; }
             | table_items_x ',' table_item_x  { ++$$; }
             ;

table_item_x: IDENTIFIER { b.Add(PSTR, *$1); delete $1; } ':' exp
            | '[' exp ']' ':' exp  
            ;

// 
// TABLE EXPRESSION
//
table_exp: exp '.' IDENTIFIER { b.Add(PSTR, *$3); delete $3; }
         | exp '[' exp ']'
         ;

// 
// SET OPERATOR
//
set_op: table_exp '=' exp { b.Add(SET, static_cast<uint8_t>(PUB|MUT)); }
      ;

// 
// GET OPERATOR
//
get_op: table_exp { b.Add(GET); }
      ;

%%

static void add_number(Bytecode& b, double num)
{
    double i;
    if(num >= 0 && num <= 255 && fabs(modf(num, &i)) < numeric_limits<double>::epsilon()) {
        b.Add(PN8, static_cast<uint8_t>(i));
    } else {
        b.Add(PNUM, num);
    }
}

int parse(Bytecode& b, string const& code)
{
    // parse code
    void *scanner;
    yylex_init_extra(&b, &scanner);
    yy_scan_bytes(code.c_str(), code.size(), scanner);
    int r = yyparse(scanner, b);
    yylex_destroy(scanner);
    return r;
}


void yyerror(YYLTYPE* yylloc, void* scanner, Bytecode& b, const char* s)
{
    yylex_destroy(scanner);     // gives us a clean exit
    throw zoe_syntax_error(s);
    //cerr << "error in " << yylloc->first_line << ":" << yylloc->first_column << ": " << s << "\n";
}

// vim: ts=4:sw=4:sts=4:expandtab:syntax=yacc
