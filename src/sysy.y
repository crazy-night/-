%code requires {
  #include <memory>
  #include <string>
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.h"

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
  SubBaseAST *sub_val;
  LValAST *l_val;
  TypeAST *t_val;
  FuncFParamAST *ffp_val;
  ConstDefAST *cd_val;
  VarDefAST *vd_val;
  std::vector<std::unique_ptr<BlockItemAST>> *bi_vals;
  std::vector<std::unique_ptr<ConstDefAST>> *cd_vals;
  std::vector<std::unique_ptr<VarDefAST>> *vd_vals;
  std::vector<std::unique_ptr<FuncFParamAST>> *ffp_vals;
  std::vector<std::unique_ptr<FuncRParamAST>> *frp_vals;
}

// lexer ���ص����� token ���������
// ע�� IDENT �� INT_CONST �᷵�� token ��ֵ, �ֱ��Ӧ str_val �� int_val
%token INT RETURN VOID CONST IF ELSE WHILE BREAK CONTINUE OR AND EQ NE LE GE
%token <str_val> IDENT
%token <int_val> INT_CONST

// ���ս�������Ͷ���
%type <ast_val> FuncDef Block Decl ConstDecl Stmt VarDecl MatchedStmt OpenStmt CompUnit
%type <sub_val> ConstInitVal InitVal ConstExp Exp PrimaryExp AddExp MulExp UnaryExp UnaryOp LOrExp LAndExp EqExp RelExp
%type <l_val> LVal
%type <t_val> Type
%type <int_val>Number
%type <cd_vals> ConstDefs
%type <cd_val> ConstDef
%type <bi_vals> BlockItem
%type <vd_vals> VarDefs
%type <vd_val> VarDef
%type <ffp_vals> FuncFParams
%type <ffp_val> FuncFParam
%type <frp_vals> FuncRParam

%%

// ��ʼ��, CompUnit ::= FuncDef, �����ź������˽�����ɺ� parser Ҫ��������
// ֮ǰ���Ƕ����� FuncDef �᷵��һ�� str_val, Ҳ�����ַ���ָ��
// �� parser һ�������� CompUnit, ��˵�����е� token ����������, ������������
// ��ʱ����Ӧ�ð� FuncDef ���صĽ���ռ�����, ��Ϊ AST �������� parser �ĺ���
// $1 ָ���������һ�����ŵķ���ֵ, Ҳ���� FuncDef �ķ���ֵ
Start
  : CompUnit {
    auto start = make_unique<StartAST>();
    start->compunit = unique_ptr<BaseAST>($1);
    ast = move(start);
  }
  ;

// CompUnit ::= [CompUnit] (Decl | FuncDef);
// []��ʾ0������1��
CompUnit
  : FuncDef {
    auto ast = new CompUnitAST();
    ast->funcdef = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | CompUnit FuncDef {
    auto ast = new CompUnitAST();
    ast->compunit = unique_ptr<BaseAST>($1);
    ast->funcdef = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | CompUnit Decl {
    auto ast = new CompUnitAST();
    ast->compunit = unique_ptr<BaseAST>($1);
    ast->decl = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT "(" [FuncFParams] ")" Block;
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
  : Type IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->type = unique_ptr<TypeAST>($1);
    ast->ident = *unique_ptr<string>($2);
    auto v = new vector<unique_ptr<FuncFParamAST>>();
    ast->funcfparam = unique_ptr<vector<unique_ptr<FuncFParamAST>>>(v);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | Type IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->type = unique_ptr<TypeAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->funcfparam = unique_ptr<vector<unique_ptr<FuncFParamAST>>>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

// FuncFParams ::= FuncFParam {"," FuncFParam};
//�˴�ʹ��move����Ϊvecto��push_back���������ֹע��ָ��;
FuncFParams
  : FuncFParam {
    auto v = new vector<unique_ptr<FuncFParamAST>>();
    auto ast = unique_ptr<FuncFParamAST>($1);
    v->push_back(move(ast));
    $$ = v;
  }
  | FuncFParams ',' FuncFParam {
    auto ast = unique_ptr<FuncFParamAST>($3);
    $1->push_back(move(ast));
    $$ = $1;
  }
  ;

// FuncFParam ::= BType IDENT;
FuncFParam
  : Type IDENT { 
    auto ast = new FuncFParamAST();
    ast->type = unique_ptr<TypeAST>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

// FuncType ::= "void" | "int";
// BType ::= "int";
Type
  : INT {
    auto ast = new TypeAST();
    ast->type_id = 0;
    $$ = ast;
  }
  | VOID {
    auto ast = new TypeAST();
    ast->type_id = -1;
    $$ = ast;
  }
  ;

// Block ::= "{" {BlockItem} "}";
Block
  : '{' '}' {
    auto ast = new BlockAST();
    auto v = new vector<unique_ptr<BlockItemAST>>();
    ast->blockitem = unique_ptr<vector<unique_ptr<BlockItemAST>>>(v);
    $$ = ast;
  }
  | '{' BlockItem '}' {
    auto ast = new BlockAST();
    ast->blockitem = unique_ptr<vector<unique_ptr<BlockItemAST>>>($2);
    $$ = ast;
  }
  ;

// BlockItem ::= Decl | Stmt;
BlockItem
  : BlockItem Decl {
    unique_ptr<BlockItemAST> ast(new BlockItemAST());
    ast->decl = unique_ptr<BaseAST>($2);
    $1->push_back(move(ast));
    $$ = $1;
  }
  | BlockItem Stmt {
    unique_ptr<BlockItemAST> ast(new BlockItemAST());
    ast->stmt = unique_ptr<BaseAST>($2);
    $1->push_back(move(ast));
    $$ = $1;
  }
  | Decl {
    auto v = new vector<unique_ptr<BlockItemAST>>();
    unique_ptr<BlockItemAST> ast(new BlockItemAST());
    ast->decl = unique_ptr<BaseAST>($1);
    v->push_back(move(ast));
    $$ = v;
  }
  | Stmt {
    auto v = new vector<unique_ptr<BlockItemAST>>();
    unique_ptr<BlockItemAST> ast(new BlockItemAST());
    ast->stmt = unique_ptr<BaseAST>($1);
    v->push_back(move(ast));
    $$ = v;
  }
  ;

// Decl ::= ConstDecl | VarDecl;
Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->constdecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->vardecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// ConstDecl ::= "const" BType ConstDefs ";";
ConstDecl
  : CONST Type ConstDefs ';' {
    auto ast = new ConstDeclAST();   
    ast->type = unique_ptr<TypeAST>($2);
    ast->constdef = unique_ptr<vector<unique_ptr<ConstDefAST>>>($3);
    $$ = ast;
  }
  ;

// ConstDefs ::= ConstDef | ConstDefs "," ConstDef;
ConstDefs
  : ConstDef {
    auto v = new vector<unique_ptr<ConstDefAST>>();
    auto ast = unique_ptr<ConstDefAST>($1);
    v->push_back(move(ast));
    $$ = v;
    }
    | ConstDefs ',' ConstDef {
    auto ast =unique_ptr<ConstDefAST>($3);
    $1->push_back(move(ast));
    $$ = $1;
    }
    ;

// ConstDef ::= IDENT ["[" ConstExp "]"] "=" ConstInitVal;
ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();   
    ast->ident = *unique_ptr<string>($1);
    ast->constinitval = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;

// ConstInitVal ::= ConstExp;
ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();   
    ast->constexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  ;

// ConstExp ::= Exp;
ConstExp
  : Exp {
    auto ast = new ConstExpAST();   
    ast->exp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  ;

// VarDecl ::= BType VarDefs ";";
VarDecl
  : Type VarDefs ';' {
    auto ast = new VarDeclAST();   
    ast->type = unique_ptr<TypeAST>($1);
    ast->vardef = unique_ptr<vector<unique_ptr<VarDefAST>>>($2);
    $$ = ast;
  }
  ;

// VarDefs ::= VarDef | VarDefs "," VarDef
VarDefs
  : VarDef {
    auto v = new vector<unique_ptr<VarDefAST>>();
    auto ast = unique_ptr<VarDefAST>($1);
    v->push_back(move(ast));
    $$ = v;
    }
    | VarDefs "," VarDef {
    auto ast =unique_ptr<VarDefAST>($3);
    $1->push_back(move(ast));
    $$ = $1;
    }
    ;



// VarDef ::= IDENT | IDENT "=" InitVal;
VarDef
  : IDENT {
    auto ast = new VarDefAST();   
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal{
    auto ast =new VarDefAST();   
    ast->ident = *unique_ptr<string>($1);
    ast->initval = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;


// InitVal ::= Exp;
InitVal
  : Exp {
    auto ast = new InitValAST();   
    ast->exp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  ;

// Stmt ::= MatchedStmt | OpenStmt;
// �˴���������������������MatchedStmtʱ������ƽ�/��Լ��ͻ,BisonĬ��ѡ���ƽ�������ELSE
Stmt
  : MatchedStmt {
    auto ast = new StmtAST();
    ast->matchedstmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | OpenStmt {
    auto ast = new StmtAST();
    ast->openstmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

/* MatchedStmt ::= "if" "(" Exp ")" MatchedStmt "else" MatchedStmt
                |  LVal "=" Exp ";"
                | [Exp] ";"
                | Block
                | "if" "(" Exp ")" Stmt ["else" Stmt]
                | "while" "(" Exp ")" Stmt
                | "break" ";"
                | "continue" ";"
                | "return" [Exp] ";";*/ 

MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {    
    auto ast = new MatchedStmtAST();
    ast->exp = unique_ptr<SubBaseAST>($3);
    ast->matchedstmt_1 = unique_ptr<BaseAST>($5);
    ast->matchedstmt_2 = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new MatchedStmtAST();
    ast->lval = unique_ptr<LValAST>($1);
    ast->exp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | ';'{
    auto ast = new MatchedStmtAST();
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new MatchedStmtAST();
    ast->exp = unique_ptr<SubBaseAST>($1);
    ast->flag = 0;
    $$ = ast;
  }
  | Block { 
    auto ast = new MatchedStmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new MatchedStmtAST();
    ast->exp = unique_ptr<SubBaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';'{
    auto ast = new MatchedStmtAST();
    ast->flag = 3;
    $$ = ast;
  }
  | CONTINUE ';' { 
    auto ast = new MatchedStmtAST();
    ast->flag = 2;
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new MatchedStmtAST();
    ast->flag = 1;
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new MatchedStmtAST();
    ast->exp = unique_ptr<SubBaseAST>($2);
    ast->flag = 1;
    $$ = ast;
  }
  ;


// OpenStmt ::= "if" "(" Exp ') Stmt | "if" "(" Exp ') MatchedStmt "else" OpenStmt 
OpenStmt
  : IF '(' Exp ')' Stmt {    
    auto ast = new OpenStmtAST();
    ast->exp = unique_ptr<SubBaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {    
    auto ast = new OpenStmtAST();
    ast->exp = unique_ptr<SubBaseAST>($3);
    ast->matchedstmt = unique_ptr<BaseAST>($5);
    ast->openstmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;

// Exp ::= LOrExp;
Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lorexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  ;

// LOrExp ::= LAndExp | LOrExp "||" LAndExp;
LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->landexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  | LOrExp OR LAndExp {
    auto ast = new LOrExpAST();
    ast->lorexp = unique_ptr<SubBaseAST>($1);
    ast->landexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;

// LAndExp ::= EqExp | LAndExp "&&" EqExp;
LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eqexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  | LAndExp AND EqExp {
    auto ast = new LAndExpAST();
    ast->landexp = unique_ptr<SubBaseAST>($1);
    ast->eqexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;

// EqExp ::= RelExp | EqExp ("==" | "!=") RelExp;
EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->relexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  | EqExp EQ RelExp {
    auto ast = new EqExpAST();
    ast->eqexp = unique_ptr<SubBaseAST>($1);
    ast->op = 0;
    ast->relexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | EqExp NE RelExp {
    auto ast = new EqExpAST();
    ast->eqexp = unique_ptr<SubBaseAST>($1);
    ast->op = 1;
    ast->relexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;

// RelExp ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->addexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<SubBaseAST>($1);
    ast->op = 0;
    ast->addexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<SubBaseAST>($1);
    ast->op = 1;
    ast->addexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | RelExp LE AddExp {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<SubBaseAST>($1);
    ast->op = 2;
    ast->addexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<SubBaseAST>($1);
    ast->op = 3;
    ast->addexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;

// AddExp ::= MulExp | AddExp ("+" | "-") MulExp;
AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mulexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->addexp = unique_ptr<SubBaseAST>($1);
    ast->op = 0;
    ast->mulexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->addexp = unique_ptr<SubBaseAST>($1);
    ast->op = 1;
    ast->mulexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;

// MulExp ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unaryexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->mulexp = unique_ptr<SubBaseAST>($1);
    ast->op = 0;
    ast->unaryexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->mulexp = unique_ptr<SubBaseAST>($1);
    ast->op = 1;
    ast->unaryexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->mulexp = unique_ptr<SubBaseAST>($1);
    ast->op = 2;
    ast->unaryexp = unique_ptr<SubBaseAST>($3);
    $$ = ast;
  }
  ;

// UnaryExp ::= PrimaryExp | IDENT "(" [FuncRParams] ")" | UnaryOp UnaryExp;
UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primaryexp = unique_ptr<SubBaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->unaryop = unique_ptr<SubBaseAST>($1);
    ast->unaryexp = unique_ptr<SubBaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    auto v = new vector<unique_ptr<FuncRParamAST>>();
    ast->funcrparam = unique_ptr<vector<unique_ptr<FuncRParamAST>>>(v);
    $$ = ast;
  }
  | IDENT '(' FuncRParam ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->funcrparam = unique_ptr<vector<unique_ptr<FuncRParamAST>>>($3);
    $$ = ast;
  }
  ;

// FuncRParams ::= Exp {"," Exp};
FuncRParam
  : Exp { 
    auto v = new vector<unique_ptr<FuncRParamAST>>();
    unique_ptr<FuncRParamAST> ast(new FuncRParamAST());
    ast->exp = unique_ptr<SubBaseAST>($1);
    v->push_back(move(ast));
    $$ = v;
  }
  | FuncRParam ',' Exp {
    unique_ptr<FuncRParamAST> ast(new FuncRParamAST());
    ast->exp = unique_ptr<SubBaseAST>($3);
    $1->push_back(move(ast));
    $$ = $1;
  }
  ;

// PrimaryExp ::= "(" Exp ")" | LVal | Number;
PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<SubBaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number= $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->lval= unique_ptr<LValAST>($1);
    $$ = ast;
  }
  ;

// LVal ::= IDENT;
LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

// UnaryOp ::= "+" | "-" | "!";
UnaryOp
  : '+'{
  auto ast = new UnaryOpAST();
  ast->op_id = 0;
  $$ = ast;
  }
  | '-'{
  auto ast = new UnaryOpAST();
  ast->op_id = 1;
  $$ = ast;
  }
  | '!'{
  auto ast = new UnaryOpAST();
  ast->op_id = 2;
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
  std::string ss = ast->Dump();
  cerr << "error: " << ss << endl;
}