#include "AST.h"
using namespace std;
string code = "";
int reg = 0;

string CompUnitAST::Dump() const {
	code += "fun ";
	func_def->Dump();
	return code;
}

string FuncDefAST::Dump() const {
	code += "@" + ident + "(): ";
	code += func_type->Dump();
	block->Dump();
	return "";
}

// ...
string FuncTypeAST::Dump() const {
	return type[func_typeid];
}

string BlockAST::Dump()const {
	code += " {\n%entry:\n";
	stmt->Dump();
	code += "}\n";
	return "";
}

string StmtAST::Dump()const {
	string value = exp->Dump();
	code += "  ret " + value + "\n";
	return "";
}

string ExpAST::Dump() const {
	return lorexp->Dump();
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

string PrimaryExpAST::Dump()const {
	if (exp != NULL) {
		return exp->Dump();
	}
	return to_string(number);
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

