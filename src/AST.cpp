#include "AST.h"
using namespace std;
string code = "";
int reg = 0;


string Type[1] = { "i32" };
string b_type[1] = { "int" };

vector<Symbol>* cur;

string CompUnitAST::Dump() const {
	code += "fun ";
	func_def->Dump();
	return code;
}

string FuncDefAST::Dump() const {
	code += "@" + ident + "(): ";
	code += func_type->Dump();
	block->Dump();
	return "FuncDefAST";
}

// ...
string FuncTypeAST::Dump() const {
	return Type[func_typeid];
}

string BlockAST::Dump()const {
	code += " {\n%entry:\n";
	vector<Symbol> v;
	cur = &v;
	for (auto it = blockitem->begin();it != blockitem->end();++it)
		(*it)->Dump();
	code += "}\n";
	return "BlockAST";
}

string BlockItemAST::Dump()const {
	if (decl != NULL)
		decl->Dump();
	else
		stmt->Dump();
	return "BlockItemAST";
}

string DeclAST::Dump()const {
	if (constdecl != NULL)
		constdecl->Dump();
	else
		vardecl->Dump();
	return "DeclAST";
}

string ConstDeclAST::Dump()const {
	string t = btype->Dump();
	for (auto it = constdef->begin();it != constdef->end();++it)
		(*it)->Dump();
	return "ConstDeclAST";
}

string BTypeAST::Dump()const {
	return b_type[b_typeid];
}

string ConstDefAST::Dump()const {
	if(exist(ident))
		assert(false);
	Symbol s;
	s.name = ident;
	s.value = constinitval->Cal();
	s.type = 0;
	cur->push_back(s);
	return "ConstDef";
}

string ConstInitValAST::Dump()const {
	return"ConstInitValAST";
}

int ConstInitValAST::Cal()const {
	return constexp->Cal();
}

string ConstExpAST::Dump()const {
	return"ConstExpAST";
}

int ConstExpAST::Cal()const {
	return exp->Cal();
}

string VarDeclAST::Dump()const {
	string t = btype->Dump();
	string ans;
	for (auto it = vardef->begin();it != vardef->end();++it) {
		(*it)->Dump();
	}
	return "";
}

string VarDefAST::Dump()const {
	if (exist(ident))
		assert(false);
	Symbol s;
	s.name = ident;
	s.type = 1;
	cur->push_back(s);
	code += "  @" + ident + " = alloc " + Type[0] + "\n";
	if (initval != NULL) {
		string ans = initval->Dump();
		code += "  store " + ans + ", @" + ident + "\n";
	}
	return "  @" + ident;
}

string InitValAST::Dump()const {
	return exp->Dump();
}

int InitValAST::Cal()const {
	return exp->Cal();
}


string StmtAST::Dump()const {
	string value = exp->Dump();
	if (lval != NULL) {
		code += "  store " + value + ", @" + lval->Assign() + "\n";
	}
	else
		code += "  ret " + value + "\n";
	return "StmtAST";
}

string ExpAST::Dump() const {
	return lorexp->Dump();
}

int ExpAST::Cal()const {
	return lorexp->Cal();
}

string LOrExpAST::Dump()const {
	if (lorexp != NULL) {
		string value_1 = lorexp->Dump();
		string value_2 = " or ";
		string value_3 = landexp->Dump();
		code += "  %" + to_string(reg++) + " =" + value_2 + value_1 + ", " + value_3 + "\n";
		code += "  %" + to_string(reg) + " = ne 0, %" + to_string(reg - 1) + "\n";
		return "%" + to_string(reg++);
	}
	return landexp->Dump();
}

int LOrExpAST::Cal()const {
	if (lorexp != NULL) {
		return lorexp->Cal() || landexp->Cal();
	}
	return landexp->Cal();
}

string LAndExpAST::Dump()const {
	if (landexp != NULL) {
		string value_1 = landexp->Dump();
		string value_3 = eqexp->Dump();
		code += "  %" + to_string(reg++) + " = eq 0, " + value_1 + "\n";
		code += "  %" + to_string(reg++) + " = eq 0, " + value_3 + "\n";
		code += "  %" + to_string(reg) + " = or %" + to_string(reg - 1) + ", %" + to_string(reg - 2) + "\n";
		++reg;
		code += "  %" + to_string(reg) + " = eq 0, %" + to_string(reg - 1) + "\n";
		return "%" + to_string(reg++);
	}
	return eqexp->Dump();
}

int LAndExpAST::Cal()const {
	if (landexp != NULL) {
		return landexp->Cal() && eqexp->Cal();
	}
	return eqexp->Cal();
}

string EqExpAST::Dump()const {
	if (eqexp != NULL) {
		string value_1 = eqexp->Dump();
		string value_2 = "";
		switch (op) {
		case 0:value_2 += " eq ";break;
		case 1:value_2 += " ne ";break;
		default:assert(false);
		}
		string value_3 = relexp->Dump();
		code += "  %" + to_string(reg) + " =" + value_2 + value_1 + ", " + value_3 + "\n";
		return "%" + to_string(reg++);
	}
	return relexp->Dump();
}

int EqExpAST::Cal()const {
	if (eqexp != NULL) {
		switch (op) {
		case 0:return eqexp->Cal() == relexp->Cal();
		case 1:return eqexp->Cal() != relexp->Cal();
		default:assert(false);
		}
	}
	return relexp->Cal();
}

string RelExpAST::Dump()const {
	if (relexp != NULL) {
		string value_1 = relexp->Dump();
		string value_2 = "";
		switch (op) {
		case 0:value_2 += " lt ";break;
		case 1:value_2 += " gt ";break;
		case 2:value_2 += " le ";break;
		case 3:value_2 += " ge ";break;
		default:assert(false);
		}
		string value_3 = addexp->Dump();
		code += "  %" + to_string(reg) + " =" + value_2 + value_1 + ", " + value_3 + "\n";
		return "%" + to_string(reg++);
	}
	return addexp->Dump();
}

int RelExpAST::Cal()const {
	if (relexp != NULL) {
		switch (op) {
		case 0:return relexp->Cal() < addexp->Cal();
		case 1:return relexp->Cal() > addexp->Cal();
		case 2:return relexp->Cal() <= addexp->Cal();
		case 3:return relexp->Cal() >= addexp->Cal();
		default:assert(false);
		}
	}
	return addexp->Cal();
}

string AddExpAST::Dump()const {
	if (addexp != NULL) {
		string value_1 = addexp->Dump();
		string value_2 = "";
		switch (op) {
		case 0:value_2 += " add ";break;
		case 1:value_2 += " sub ";break;
		default:assert(false);
		}
		string value_3 = mulexp->Dump();
		code += "  %" + to_string(reg) + " =" + value_2 + value_1 + ", " + value_3 + "\n";
		return "%" + to_string(reg++);
	}
	return mulexp->Dump();
}

int AddExpAST::Cal()const {
	if (addexp != NULL) {
		switch (op) {
		case 0:return addexp->Cal() + mulexp->Cal();
		case 1:return addexp->Cal() - mulexp->Cal();
		default:assert(false);
		}
	}
	return mulexp->Cal();
}

string MulExpAST::Dump()const {
	if (mulexp != NULL) {
		string value_1 = mulexp->Dump();
		string value_2 = "";
		switch (op) {
		case 0:value_2 += " mul ";break;
		case 1:value_2 += " div ";break;
		case 2:value_2 += " mod ";break;
		default:assert(false);
		}
		string value_3 = unaryexp->Dump();
		code += "  %" + to_string(reg) + " =" + value_2 + value_1 + ", " + value_3 + "\n";
		return "%" + to_string(reg++);
	}
	return unaryexp->Dump();
}

int MulExpAST::Cal()const {
	if (mulexp != NULL) {
		switch (op) {
		case 0:return mulexp->Cal() * unaryexp->Cal();
		case 1:return mulexp->Cal() / unaryexp->Cal();
		case 2:return mulexp->Cal() % unaryexp->Cal();
		default:assert(false);
		}
	}
	return unaryexp->Cal();
}


string UnaryExpAST::Dump() const {
	if (primaryexp != NULL) {
		return primaryexp->Dump();
	}
	string value_1 = unaryexp->Dump();
	string value_2 = unaryop->Dump();
	if (value_2 != "") {
		code += "  %" + to_string(reg) + " =" + value_2 + "0, " + value_1 + "\n";
		return "%" + to_string(reg++);
	}
	return value_1;
}

int UnaryExpAST::Cal()const {
	if (primaryexp != NULL) {
		return primaryexp->Cal();
	}
	switch (unaryop->Cal()) {
	case 0:return unaryexp->Cal();
	case 1:return -unaryexp->Cal();
	case 2:return !unaryexp->Cal();
	default:assert(false);
	}
	return unaryexp->Cal();
}

string PrimaryExpAST::Dump()const {
	if (exp != NULL) {
		return exp->Dump();
	}
	if (lval != NULL)
		return lval->Dump();
	return to_string(number);
}

int PrimaryExpAST::Cal()const {
	if (exp != NULL)
		return exp->Cal();
	if (lval != NULL)
		return lval->Cal();
	return number;
}

string LValAST::Dump()const {
	auto it = find(ident);
	if (it.type) {
		code += "  %" + to_string(reg) + " = load @" + it.name + "\n";
		return "%" + to_string(reg++);
	}
	return to_string(it.value);
}

int LValAST::Cal()const {
	return find(ident).value;
}

string LValAST::Assign()const {
	auto it = find(ident);
	if (it.type) {
		return it.name;
	}
	assert(false);
}

//+,-,!
string UnaryOpAST::Dump()const {
	string ans = "";
	switch (op_id)
	{
	case 0:ans += "";break;
	case 1:ans += " sub ";break;
	case 2:ans += " eq ";break;
	default:
		assert(false);
	}
	return ans;
}

int UnaryOpAST::Cal()const {
	return op_id;
}

Symbol find(string name) {
	for (auto it = cur->begin();it != cur->end();++it) {
		if (it->name == name)
			return *it;
	}
	assert(false);
}

bool exist(string name) {
	for (auto it = cur->begin();it != cur->end();++it) {
		if (it->name == name)
			return true;
	}
	return false;
}