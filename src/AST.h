#ifndef AST_H
#define AST_H
// 所有 AST 的基类
#include<iostream>
#include<memory>
#include<string>
#include<cstdio>
#include<cassert>
#include<vector>
#define END "end"

struct Loop {
	std::string begin;
	std::string end;
	Loop(int, int);
};

struct Symbol
{
	std::string name;
	int type;
	int value;
	int depth;
	std::string dump();
};

struct SymbolTable {
	SymbolTable* pre;
	std::vector<Symbol> table;
};

Symbol find(std::string);
bool exist(std::string);


class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual std::string Dump() const = 0;
};

class SubBaseAST :public BaseAST {
public:
	virtual ~SubBaseAST() = default;
	virtual int Cal()const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
	// 用智能指针管理对象
	std::unique_ptr<BaseAST> func_def;
	std::string Dump() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
	std::unique_ptr<BaseAST> func_type;
	std::string ident;
	std::unique_ptr<BaseAST> block;
	std::string Dump() const override;
};

// ...
class FuncTypeAST :public BaseAST {
public:
	int func_typeid;
	std::string Dump()const override;
};

class BlockItemAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> decl;
	std::unique_ptr<BaseAST> stmt;
	std::string Dump() const override;
};

class BlockAST :public BaseAST {
public:
	std::unique_ptr<std::vector<std::unique_ptr<BlockItemAST>>> blockitem;
	std::string Dump() const override;
};

class DeclAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> constdecl;
	std::unique_ptr<BaseAST> vardecl;
	std::string Dump() const override;
};

class ConstDefAST :public BaseAST {
public:
	std::string ident;
	std::unique_ptr<SubBaseAST> constinitval;
	std::string Dump() const override;
};

class ConstDeclAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> btype;
	std::unique_ptr<std::vector<std::unique_ptr<ConstDefAST>>>constdef;
	std::string Dump() const override;
};

//int
class BTypeAST :public BaseAST {
public:
	int b_typeid;
	std::string Dump() const override;
};

class ConstInitValAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> constexp;
	std::string Dump() const override;
	int Cal() const override;
};

class ConstExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> exp;
	std::string Dump() const override;
	int Cal() const override;
};

class VarDefAST :public BaseAST {
public:
	std::string ident;
	std::unique_ptr<SubBaseAST> initval;
	std::string Dump() const override;
};

class VarDeclAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> btype;
	std::unique_ptr<std::vector<std::unique_ptr<VarDefAST>>>vardef;
	std::string Dump() const override;
};

class InitValAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> exp;
	std::string Dump() const override;
	int Cal() const override;
};

class LValAST :public SubBaseAST {
public:
	std::string ident;
	std::string Dump() const override;
	int Cal() const override;
	std::string Assign() const;
};


class StmtAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> matchedstmt;
	std::unique_ptr<BaseAST> openstmt;
	std::string Dump() const override;
};

class MatchedStmtAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> matchedstmt_1;
	std::unique_ptr<BaseAST> matchedstmt_2;
	std::unique_ptr<BaseAST> stmt;
	std::unique_ptr<SubBaseAST> exp;
	std::unique_ptr<LValAST> lval;
	std::unique_ptr<BaseAST> block;
	//会默认为0
	int flag;
	std::string Dump() const override;
};

class OpenStmtAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> matchedstmt;
	std::unique_ptr<BaseAST> openstmt;
	std::unique_ptr<BaseAST> stmt;
	std::unique_ptr<SubBaseAST> exp;
	std::string Dump() const override;
};


class ExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> lorexp;
	std::string Dump() const override;
	int Cal() const override;
};

class LOrExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> landexp;
	std::unique_ptr<SubBaseAST> lorexp;
	std::string Dump() const override;
	int Cal() const override;
};

class LAndExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> eqexp;
	std::unique_ptr<SubBaseAST> landexp;
	std::string Dump() const override;
	int Cal() const override;
};

class EqExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> relexp;
	std::unique_ptr<SubBaseAST> eqexp;
	int op;
	std::string Dump() const override;
	int Cal() const override;
};

class RelExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> addexp;
	std::unique_ptr<SubBaseAST> relexp;
	int op;
	std::string Dump() const override;
	int Cal() const override;
};

class AddExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> mulexp;
	std::unique_ptr<SubBaseAST> addexp;
	int op;
	std::string Dump() const override;
	int Cal() const override;
};

class MulExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> unaryexp;
	std::unique_ptr<SubBaseAST> mulexp;
	int op;
	std::string Dump() const override;
	int Cal() const override;
};

class UnaryExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> primaryexp;
	std::unique_ptr<SubBaseAST> unaryop;
	std::unique_ptr<SubBaseAST> unaryexp;
	std::string Dump() const override;
	int Cal() const override;
};

class PrimaryExpAST :public SubBaseAST {
public:
	std::unique_ptr<SubBaseAST> exp;
	std::unique_ptr<LValAST> lval;
	int number;
	std::string Dump() const override;
	int Cal() const override;
};



class UnaryOpAST :public SubBaseAST {
public:
	int op_id;
	std::string Dump() const override;
	int Cal() const override;
};

#endif // !AST_H