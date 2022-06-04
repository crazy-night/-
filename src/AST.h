#ifndef AST_H
#define AST_H
// 所有 AST 的基类
#include<iostream>
#include<memory>
#include<string>
#include<cstdio>
#include<cassert>


class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual std::string Dump() const = 0;
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
	std::string type[1] = { "i32" };
	std::string Dump()const override;
};

class BlockAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> stmt;
	std::string Dump() const override;
};

class StmtAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> exp;
	std::string Dump() const override;
};

class ExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> lorexp;
	std::string Dump() const override;
};

class LOrExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> landexp;
	std::unique_ptr<BaseAST> lorexp;
	std::string Dump() const override;
};

class LAndExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> eqexp;
	std::unique_ptr<BaseAST> landexp;
	std::string Dump() const override;
};

class EqExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> relexp;
	std::unique_ptr<BaseAST> eqexp;
	int op;
	std::string Dump() const override;
};

class RelExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> addexp;
	std::unique_ptr<BaseAST> relexp;
	int op;
	std::string Dump() const override;
};

class AddExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> mulexp;
	std::unique_ptr<BaseAST> addexp;
	int op;
	std::string Dump() const override;
};

class MulExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> unaryexp;
	std::unique_ptr<BaseAST> mulexp;
	int op;
	std::string Dump() const override;
};

class UnaryExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> primaryexp;
	std::unique_ptr<BaseAST> unaryop;
	std::unique_ptr<BaseAST> unaryexp;
	std::string Dump() const override;
};

class PrimaryExpAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> exp;
	int number;
	std::string Dump() const override;
};

class UnaryOpAST :public BaseAST {
public:
	int op_id;
	std::string Dump() const override;
};

#endif // !AST_H