%code requires {
  #include <memory>
  #include <string>
  #include "AST.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"

// ���� lexer �����ʹ�������
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// ���� parser �����ʹ��������ĸ��Ӳ���
// ������Ҫ����һ���ַ�����Ϊ AST, �������ǰѸ��Ӳ���������ַ���������ָ��
// ������ɺ�, ����Ҫ�ֶ��޸��������, �������óɽ����õ����ַ���
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval �Ķ���, ���ǰ����������һ�������� (union)
// ��Ϊ token ��ֵ�е����ַ���ָ��, �е�������
// ֮ǰ������ lexer ���õ��� str_val �� int_val ���������ﱻ�����
// ����ΪʲôҪ���ַ���ָ�����ֱ���� string ���� unique_ptr<string>?
// ������ STFW �� union ��дһ��������������������ʲô���
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer ���ص����� token ���������
// ע�� IDENT �� INT_CONST �᷵�� token ��ֵ, �ֱ��Ӧ str_val �� int_val
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

// ���ս�������Ͷ���
%type <ast_val> FuncDef FuncType Block Stmt
%type <int_val> Number

%%

// ��ʼ��, CompUnit ::= FuncDef, �����ź������˽�����ɺ� parser Ҫ��������
// ֮ǰ���Ƕ����� FuncDef �᷵��һ�� str_val, Ҳ�����ַ���ָ��
// �� parser һ�������� CompUnit, ��˵�����е� token ����������, ������������
// ��ʱ����Ӧ�ð� FuncDef ���صĽ���ռ�����, ��Ϊ AST �������� parser �ĺ���
// $1 ָ���������һ�����ŵķ���ֵ, Ҳ���� FuncDef �ķ���ֵ
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// �����������ֱ��д '(' �� ')', ��Ϊ֮ǰ�� lexer ���Ѿ������˵����ַ������
// ������ɺ�, ����Щ���ŵĽ���ռ�����, Ȼ��ƴ��һ���µ��ַ���, ��Ϊ�������
// $$ ��ʾ���ս���ķ���ֵ, ���ǿ���ͨ����������Ÿ�ֵ�ķ��������ؽ��
// ����ܻ���, FuncType, IDENT ֮��Ľ���Ѿ����ַ���ָ����
// Ϊʲô��Ҫ�� unique_ptr ��ס����, Ȼ���ٽ�����, ������ƴ����һ���ַ���ָ����
// ��Ϊ���е��ַ���ָ�붼������ new ������, new �������ڴ�һ��Ҫ delete
// ����ᷢ���ڴ�й©, �� unique_ptr ��������ָ������Զ������� delete
// ��Ȼ�˴��㿴������ unique_ptr ���ֶ� delete ������, �������Ƕ����� AST ֮��
// ����д����ʡ�ºܶ��ڴ����ĸ���
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// ͬ��, ���ٽ���
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->func_typeid = 0;
    $$ = ast;
  }
  ;

Block
  : '{' Stmt '}' {
    auto ast = new BlockAST();
    ast->stmt = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Number ';' {
    auto ast = new StmtAST();
    ast->number = $2;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

%%

// �����������, ���еڶ��������Ǵ�����Ϣ
// parser ����������� (��������ĳ���������﷨����), �ͻ�����������
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}