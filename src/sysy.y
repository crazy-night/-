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

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
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

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN VOID CONST IF ELSE WHILE BREAK CONTINUE OR AND EQ NE LE GE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
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

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
Start
  : CompUnit {
    auto start = make_unique<StartAST>();
    start->compunit = unique_ptr<BaseAST>($1);
    ast = move(start);
  }
  ;

// CompUnit ::= [CompUnit] (Decl | FuncDef);
// []表示0个或者1个
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
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
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
//此处使用move，因为vecto的push_back是深拷贝，防止注销指针;
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
// 此处若采用书上做法当读完MatchedStmt时会产生移进/规约冲突,Bison默认选择移进后续的ELSE
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

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  std::string ss = ast->Dump();
  cerr << "error: " << ss << endl;
}