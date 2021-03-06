%{

#define YY_HEADER_EXPORT_START_CONDITIONS

#include <cstdlib>
#include <cstring>

#include "bytecode.h"
#include "parser.h"

static void str_addstr(String* str, char* s);
static char* remove_undersc(char* t);

%}

%option header-file="lib/lexer.h"
%option outfile="lib/lexer.cc"
%option noyywrap
%option reentrant
%option bison-bridge
%option extra-type="Bytecode*"
%option stack
%option warn

INTEGER      [0-9][0-9_]*
BINARY       0[Bb][01_]+
HEXDIGIT     [0-9A-Fa-f_]
HEX          0[Xx]{HEXDIGIT}+
OCTAL        0[Oo][0-7_]+

FLOAT        [0-9][0-9_]*\.[0-9_]+

IDENTIFIER   [[:alpha:]_][[:alnum:]_]*

SPECIALCHARS [\+\-\*\/%\(\)\^\~\&\|\<\>\:\?\[\]\,\#\!\{\}\.\=]

SEPARATOR    [\n\;]+

SPACES       [ \t]+

%x STR CMT
%s SB

%%

<CMT,INITIAL>"/*"  { yy_push_state(CMT, yyscanner); }
<CMT>[^\*\/]*      ;
<CMT>"*"+[^\/]     ;
<CMT>\/            ;
<CMT>"*/"          { yy_pop_state(yyscanner); }
"//"+.*\n          ;

{INTEGER}       { yylval->number = (double)(strtoll(remove_undersc(yytext), NULL, 10)); return NUMBER; }
{BINARY}        { yylval->number = (double)(strtoll(remove_undersc(&yytext[2]), NULL, 2)); return NUMBER; }
{HEX}           { yylval->number = (double)(strtoll(remove_undersc(&yytext[2]), NULL, 16)); return NUMBER; }
{OCTAL}         { yylval->number = (double)(strtoll(remove_undersc(&yytext[2]), NULL, 8)); return NUMBER; }

{FLOAT}         { yylval->number = strtod(remove_undersc(yytext), NULL); return NUMBER; }

true            { yylval->boolean = true; return BOOLEAN; }
false           { yylval->boolean = false; return BOOLEAN; }
nil             { return NIL; }
let             { return LET; }
mut             { return MUT; }

'                     { yy_push_state(STR, yyscanner); yylval->str = new string(); }
<STR>\\n              { yylval->str->append(1, '\n'); }
<STR>\\r              { yylval->str->append(1, '\r'); }
<STR>\\\\             { yylval->str->append(1, '\\'); }
<STR>\\x{HEXDIGIT}{2} { yylval->str->append(1, (char)strtol(&yytext[2], NULL, 16)); }
<STR>\\               { yylval->str->append(1, '\\'); }
<STR>[^'\\\$]*        { yylval->str->append(yytext); }
<STR>\$'              { yylval->str->append(1, '$');
                        yy_pop_state(yyscanner); 
                        return STRING; }
<STR>\$[^\{]          { yylval->str->append(yytext); }
<STR>\$\{             { yy_push_state(SB, yyscanner); return STRING; }
<STR>'                { yy_pop_state(yyscanner); return STRING; }

<SB>\}                { yy_pop_state(yyscanner);
                        yylval->str = new string(); }

{SPECIALCHARS}  { return yytext[0]; }

{IDENTIFIER}    { yylval->str = new string(yytext); return IDENTIFIER; }

\%\/            { return _IDIV;  }
\*\*            { return _POW;   }
\<\<            { return _SHL;   }
\>\>            { return _SHR;   }

\>\=            { return _GTE;   }
\<\=            { return _LTE;   }

\=\=            { return _EQ;    }
\!\=            { return _NEQ;   }

\&\&            { return CCAND;  }
\|\|            { return CCOR;   }

\.\.            { return CONCAT; }

{SEPARATOR}     { return SEP;    }

{SPACES}        ;
.               return 1;

%%


static char* remove_undersc(char* t)
{
    int c = 0;
    while(t[c]) {
        if(t[c] == '_') {
            memmove(t+c, t+c+1, strlen(t)-c);
            continue;
        }
        ++c;
    }
    return t;
}


// vim: ts=4:sw=4:sts=4:expandtab
