%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>

// ��Ϊ Flex ���õ� Bison �й��� token �Ķ���
// ������Ҫ include Bison ���ɵ�ͷ�ļ�
#include "sysy.tab.hpp"

using namespace std;

%}

/* �հ׷���ע�� */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*$
MultiLineComment "/*"([^*]|\*+[^*/])*\*+"/"

/* ��ʶ�� */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* ���������� */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%

{WhiteSpace}    { /* ����, �����κβ��� */ }
{LineComment}   { /* ����, �����κβ��� */ }
{MultiLineComment}   { /* ����, �����κβ��� */ }

"int"           { return INT; }
"return"        { return RETURN; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

.               { return yytext[0]; }

%%