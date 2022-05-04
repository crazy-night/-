#pragma once
// 所有 AST 的基类
#include<iostream>
#include<memory>
#include<string>
#include<cstdio>
class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(FILE *f) const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
	// 用智能指针管理对象
	std::unique_ptr<BaseAST> func_def;
	void Dump(FILE* f) const override {
		fprintf(f,"fun ");
		func_def->Dump(f);
	}
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
	std::unique_ptr<BaseAST> func_type;
	std::string ident;
	std::unique_ptr<BaseAST> block;
	void Dump(FILE* f) const override {
		fprintf(f,"@");
		fprintf(f, "%s", ident.c_str());
		fprintf(f, "(): ");
		func_type->Dump(f);
		block->Dump(f);
	}
};

// ...
class FuncTypeAST :public BaseAST {
public:
	int func_typeid;
	std::string type[1] = { "i32" };
	void Dump(FILE* f)const override {
		fprintf(f, "%s",type[func_typeid].c_str());
	}
};

class BlockAST :public BaseAST {
public:
	std::unique_ptr<BaseAST> stmt;
	void Dump(FILE* f) const override {
		fprintf(f, " {\n%%entry:\n");
		stmt->Dump(f);
		fprintf(f, "}\n");
	}
};

class StmtAST :public BaseAST {
public:
	int number;
	void Dump(FILE* f) const override {
		fprintf(f,"  ret ");
		fprintf(f, "%d", number);
		fprintf(f, "\n");
	}
};
