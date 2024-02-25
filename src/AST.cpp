#include "AST.h"
#define VOID -1
#define CONST 0
#define VAR 1
#define FUNCINT 2
#define FUNCPARAM 3
#define FUNCVOID 4
#define ARRAY 5
#define FUNCPARRAY 6
#define RETURN 1
#define CONTINUE 2
#define BREAK 3

using namespace std;
string code = "";
int reg = 0;


string Type[2] = { "i32", "*i32" };
vector<Loop>loop;
SymbolTable global_symbol_table;
SymbolTable* cur_tab = &global_symbol_table;

//区别不同符号表重名符号
int tab_num = 0;
int entry_num = 0;

Loop::Loop(int x) {
	begin = "%while_begin_" + to_string(x);
	end = "%while_end_" + to_string(x);
}
//处理初始化列表
vector<string> InitList(vector<int>& index, vector<int>& dim, string s) {
	// 可能会有溢出
	vector<string> ret;
	int m = index.size(), len = s.length(), i = 0;
	stack<pair<int, int>> st;//当前列表长度
	int cnt = 0, pre = m;
	while (i < len) {
		if (s[i] == ' ' || s[i] == ',') {
			++i;
			continue;
		}
		else if (s[i] == '{') {
			//assert(v[0]);
			int j = 0;
			while (j < pre) {
				if (cnt % dim[j + 1]) {
					break;
				}
				++j;
			}
			--pre;
			st.push(make_pair(j, cnt));
			++i;
			continue;
		}
		else if (s[i] == '}') {
			int k = st.top().first, size = dim[k] + st.top().second - cnt;
			st.pop();
			for (int j = 0;j < size;++j) {
				ret.push_back("0");
			}
			++i;
			cnt += size;
			++pre;
		}
		else {
			//成员
			string val = "";
			while (i < len && s[i] != ',' && s[i] != '}') {
				val += s[i];
				++i;
			}
			ret.push_back(val);
			++cnt;
		}
	}
	return ret;
}

string StartAST::Dump() const {
	init();
	compunit->Dump();
	return code;
}

string CompUnitAST::Dump() const {
	if (compunit != NULL) {
		compunit->Dump();
	}
	cur_tab = &global_symbol_table;
	if (funcdef != NULL)
		funcdef->Dump();
	else
		decl->Dump();
	return "";
}

string FuncDefAST::Dump() const {
	code += "fun @" + ident + "(";

	tab_num = 0;
	reg = 0;

	Symbol s;
	s.name = ident;
	if (type->type_id == VOID)
		s.type = FUNCVOID;
	else s.type = FUNCINT;
	s.depth = tab_num;
	cur_tab->table.push_back(s);
	SymbolTable func_symbol_table;
	tab_num++;
	func_symbol_table.pre = cur_tab;
	cur_tab = &func_symbol_table;
	
	vector<string> v;
	for (auto it = funcfparam->begin();it != funcfparam->end();it++) {
		if (it != funcfparam->begin())
			code += ", ";
		v.push_back((*it)->Dump());

	}
	code+=")";
	if (type->type_id != VOID)
		code += ": " + type->Dump();
	code += " {\n%entry:\n";

	int i = 0;
	for (auto it = cur_tab->table.begin();it != cur_tab->table.end();it++) {
		code += "  " + it->dump() + " = alloc " + v[i++] + "\n";
		code += "  store @" + it->name + "_" + to_string(it->depth) + ", " + it->dump() + "\n";
	}

	
	string ret = block->Dump();
	if (ret != END) {
		if (type->type_id == VOID)
			code += "  ret\n";
		else
			code += "  ret 0\n";
	}

	code += "}\n";
	cur_tab = cur_tab->pre;
	return "FuncDefAST";
}


string FuncFParamAST::Dump()const {
	if (exist(ident))
		assert(false);
	Symbol s;
	s.name = ident;
	if (constexp) {
		s.type = FUNCPARRAY;
		//存数组维度，方便处理实参
		s.value = constexp->size() + 1;
	}
	else {
		s.type = FUNCPARAM;
	}
	s.depth = tab_num;
	cur_tab->table.push_back(s);
	string t = type->Dump();
	if (constexp) {
		for (auto it = constexp->rbegin();it != constexp->rend();++it) {
			string i = (*it)->Dump();
			t = "[" + t + ", " + i + "]";
		}
		t = "*" + t;
	}
	code += "@" + ident + "_" + to_string(tab_num) + ": " + t;
	return t;
}

// ...
string TypeAST::Dump() const {
	if (type_id == VOID)
		return "";
	return Type[type_id];
}

string BlockAST::Dump()const {
	SymbolTable block_symbol_table;
	tab_num++;
	block_symbol_table.pre = cur_tab;
	cur_tab = &block_symbol_table;
	for (auto it = blockitem->begin();it != blockitem->end();++it)
		if ((*it)->Dump() == END) {
			cur_tab = cur_tab->pre;
			return END;
		}
	cur_tab = cur_tab->pre;
	return "";
}

string BlockItemAST::Dump()const {
	if (decl != NULL)
		return decl->Dump();
	else
		return stmt->Dump();;
}

string DeclAST::Dump()const {
	if (constdecl != NULL)
		constdecl->Dump();
	else
		vardecl->Dump();
	return "DeclAST";
}

string ConstDeclAST::Dump()const {
	string t = type->Dump();
	for (auto it = constdef->begin();it != constdef->end();++it)
		(*it)->Dump();
	return "ConstDeclAST";
}



string ConstDefAST::Dump()const {
	if (exist(ident))
		assert(false);
	Symbol s;
	s.name = ident;
	if (!arraysize) {
		s.value = constinitval->Cal();
		s.type = CONST;
		s.depth = tab_num;
		cur_tab->table.push_back(s);
	}
	else {
		s.type = ARRAY;
		s.value = arraysize->size();
		s.depth = tab_num;
		cur_tab->table.push_back(s);
		if (cur_tab->pre != NULL) {
			string t = Type[0];
			//逆序
			vector<int> index;
			vector<int> dim;
			int sum = 1;
			dim.push_back(1);
			for (auto it = arraysize->rbegin();it != arraysize->rend();++it) {
				int i = (*it)->Cal();
				index.push_back(i);
				sum *= i;
				dim.push_back(sum);
				t = "[" + t + ", " + to_string(i) + "]";
			}
			code += "  " + s.dump() + " = alloc " + t + "\n";
			if (constinitval) {
				string ans = constinitval->Dump();
				vector<string> strs = InitList(index, dim, ans);
				int m = index.size();
				vector<int> v = vector<int>(m + 1, 0);
				for (const auto& str : strs) {
					int j = 0;
					string pre = s.dump();
					for (int i = m - 1;i >= 0;--i) {
						code += "  %" + to_string(reg) + " = getelemptr " + pre + ", " + to_string(v[i]) + "\n";
						pre = "%" + to_string(reg++);
					}
					while (j < m && (++v[j]) == index[j]) {
						v[j] = 0;
						++j;
					}
					code += "  store " + str + ", " + pre + "\n";
				}
			}
		}
		else {
			string t = Type[0];
			//逆序
			vector<int> index;
			vector<int> dim;
			int sum = 1;
			dim.push_back(1);
			for (auto it = arraysize->rbegin();it != arraysize->rend();++it) {
				int i = (*it)->Cal();
				index.push_back(i);
				sum *= i;
				dim.push_back(sum);
				t = "[" + t + ", " + to_string(i) + "]";
			}
			code += "global " + s.dump() + " = alloc " + t;
			if (constinitval) {
				string ans = constinitval->Dump();
				if (ans != "{}") {
					vector<string> strs = InitList(index, dim, ans);
					int m = index.size(), n = strs.size();
					string t = "";
					for (int k = 0;k < n;++k) {
						int j = 1;
						while (j <= m && (k % dim[j] == 0)) {
							t += "{";
							++j;
						}
						t += strs[k];
						j = 1;
						while (j <= m && ((k + 1) % dim[j] == 0)) {
							t += "}";
							++j;
						}
						if (k != n - 1) {
							t += ", ";
						}
					}
					code += ", " + t + "\n";
					return s.dump();
				}

			}
			code += ", zeroinit\n";
		}
	}
	return "ConstDefAST";
}

string ConstInitValAST::Dump()const {
	if (constexp)
		return constexp->Dump();
	string ans = "";
	for (auto it = array->begin();it != array->end();++it) {
		if (it != array->begin())
			ans += ", ";
		ans += (*it)->Dump();
	}
	return "{" + ans + "}";
}

int ConstInitValAST::Cal()const {
	return constexp->Cal();
}

string ConstExpAST::Dump()const {
	return to_string(exp->Cal());
}

int ConstExpAST::Cal()const {
	return exp->Cal();
}

string VarDeclAST::Dump()const {
	type->Dump();
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
	if (arraysize) {
		s.type = ARRAY;
		s.value = arraysize->size();
	}
	else {
		s.type = VAR;
	}
	s.depth = tab_num;
	cur_tab->table.push_back(s);
	if (cur_tab->pre != NULL) {
		string t = Type[0];
		//逆序
		vector<int> index;
		vector<int> dim;
		int sum = 1;
		dim.push_back(1);
		if (arraysize) {
			for (auto it = arraysize->rbegin();it != arraysize->rend();++it) {
				int i = (*it)->Cal();
				index.push_back(i);
				sum *= i;
				dim.push_back(sum);
				t = "[" + t + ", " + to_string(i) + "]";
			}
		}
		code += "  " + s.dump() + " = alloc " + t + "\n";
		if (initval) {
			string ans = initval->Dump();
			if (arraysize) {
				vector<string> strs = InitList(index, dim, ans);
				int m = index.size();
				vector<int> v = vector<int>(m + 1, 0);
				for (const auto& str : strs) {
					int j = 0;
					string pre = s.dump();
					for (int i = m - 1;i >= 0;--i) {
						code += "  %" + to_string(reg) + " = getelemptr " + pre + ", " + to_string(v[i]) + "\n";
						pre = "%" + to_string(reg++);
					}
					while (j < m && (++v[j]) == index[j]) {
						v[j] = 0;
						++j;
					}
					code += "  store " + str + ", " + pre + "\n";
				}
			}
			else {
				code += "  store " + ans + ", " + s.dump() + "\n";
			}
		}
	}
	else {
		string t = Type[0];
		//逆序
		vector<int> index;
		vector<int> dim;
		int sum = 1;
		dim.push_back(1);
		if (arraysize) {
			for (auto it = arraysize->rbegin();it != arraysize->rend();++it) {
				int i = (*it)->Cal();
				index.push_back(i);
				sum *= i;
				dim.push_back(sum);
				t = "[" + t + ", " + to_string(i) + "]";
			}
		}
		code += "global " + s.dump() + " = alloc " + t;
		if (initval) {
			string ans = initval->Dump();
			if (ans != "{}") {
				if (arraysize) {
					vector<string> strs = InitList(index, dim, ans);
					int m = index.size(), n = strs.size();
					string t = "";
					for (int k = 0;k < n;++k) {
						int j = 1;
						while (j <= m && (k % dim[j] == 0)) {
							t += "{";
							++j;
						}
						t += strs[k];
						j = 1;
						while (j <= m && ((k + 1) % dim[j] == 0)) {
							t += "}";
							++j;
						}
						if (k != n - 1) {
							t += ", ";
						}
					}
					code += ", " + t + "\n";
					return s.dump();
				}
				else if (ans != "0") {
					code += ", " + ans + "\n";
					return s.dump();
				}
			}
		}
		code += ", zeroinit\n";
	}
	return s.dump();
}

string InitValAST::Dump()const {
	if (exp) {
		if (cur_tab->pre)
			return exp->Dump();
		else
			return to_string(exp->Cal());
	}
	string ans = "";
	for (auto it = array->begin();it != array->end();++it) {
		if (it != array->begin())
			ans += ", ";
		ans += (*it)->Dump();
	}
	return "{" + ans + "}";
}

int InitValAST::Cal()const {
	return exp->Cal();
}

string StmtAST::Dump()const {
	if (matchedstmt != NULL) {
		return matchedstmt->Dump();
	}
	else {
		return openstmt->Dump();
	}
}

string MatchedStmtAST::Dump()const {
	if (matchedstmt_1 != NULL) {
		string value = exp->Dump();
		string ret;
		bool f = false;
		int num = entry_num;
		code += "  br " + value + ", %if_" + to_string(num) + ", %else_" + to_string(num) + "\n";
		entry_num += 2;
		code += "%if_" + to_string(num) + ":\n";
		ret = matchedstmt_1->Dump();
		if (ret != END) {
			code += "  jump %then_" + to_string(num + 1) + "\n";
			f = true;
		}
		code += "%else_" + to_string(num) + ":\n";
		ret = matchedstmt_2->Dump();
		if (ret != END) {
			code += "  jump %then_" + to_string(num + 1) + "\n";
			f = true;
		}
		if (f) {
			code += "%then_" + to_string(num + 1) + ":\n";
		}
		else {
			return END;
		}
	}
	else if (stmt != NULL) {
		code += "  jump %while_begin_" + to_string(entry_num) + "\n";
		code += "%while_begin_" + to_string(entry_num++) + ":\n";
		int num = entry_num - 1;
		string value = exp->Dump();
		string ret;
		Loop temp(num);
		loop.push_back(temp);
		code += "  br " + value + ", %while_body_" + to_string(num) + ", %while_end_" + to_string(num) + "\n";
		code += "%while_body_" + to_string(num) + ":\n";
		ret = stmt->Dump();
		loop.pop_back();
		if (ret != END)
			code += "  jump %while_begin_" + to_string(num) + "\n";
		code += "%while_end_" + to_string(num) + ":\n";
	}
	else {
		if (block != NULL) {
			return block->Dump();
		}
		else if (exp != NULL) {
			string value = exp->Dump();
			if (lval != NULL) {
				code += "  store " + value + ", " + lval->Assign() + "\n";
			}
			else if (flag == RETURN) {
				code += "  ret " + value + "\n";
				return END;
			}
		}
		else if (flag == RETURN) {
			code += "  ret\n";
			return END;
		}
		else if (flag == CONTINUE) {
			code += "  jump " + loop.back().begin + "\n";
			return END;
		}
		else if (flag == BREAK) {
			code += "  jump " + loop.back().end+ "\n";
			return END;
		}
	}
	return "MatchedStmtAST";
}

string OpenStmtAST::Dump()const {
	if (stmt != NULL) {
		string value = exp->Dump();
		string ret;
		int num = entry_num;
		code += "  br " + value + ", %if_" + to_string(num) + ", %then_" + to_string(num) + "\n";
		entry_num += 1;
		code += "%if_" + to_string(num) + ":\n";
		ret = stmt->Dump();
		if (ret != END)
			code += "  jump %then_" + to_string(num) + "\n";
		code += "%then_" + to_string(num) + ":\n";
	}
	else {
		string value = exp->Dump();
		string ret;
		int num = entry_num;
		code += "  br " + value + ", %entry_" + to_string(num) + ", %entry_" + to_string(num + 1) + "\n";
		entry_num += 3;
		code += "%entry_" + to_string(num) + ":\n";
		ret = matchedstmt->Dump();
		if (ret != END)
			code += "  jump %entry_" + to_string(num + 2) + "\n";
		code += "%entry_" + to_string(num + 1) + ":\n";
		openstmt->Dump();
		code += "  jump %entry_" + to_string(num + 2) + "\n";
		code += "%entry_" + to_string(num + 2) + ":\n";
	}
	return "OpenStmtAST";
}

string ExpAST::Dump() const {
	return lorexp->Dump();
}

int ExpAST::Cal()const {
	return lorexp->Cal();
}

string LOrExpAST::Dump()const {
	if (lorexp != NULL) {
		int num = entry_num;
		entry_num += 2;
		int r = reg;
		reg += 4;
		code += "  %" + to_string(r) + " = alloc i32\n";
		code += "  store 1, %" + to_string(r) + "\n";
		string value_1 = lorexp->Dump();
		//为了让非1非0变成1
		code += "  %" + to_string(r + 1) + " = ne 0, " + value_1 + "\n";
		code += "  br %" + to_string(r + 1) + ", %entry_" + to_string(num + 1) + ", %entry_" + to_string(num) + "\n";
		code += "%entry_" + to_string(num) + ":\n";
		string value_2 = landexp->Dump();
		code += "  %" + to_string(r + 2) + " = ne 0, " + value_2 + "\n";
		code += "  store %" + to_string(r + 2) + ", %" + to_string(r) + "\n";
		code += "  jump %entry_" + to_string(num + 1) + "\n";
		code += "%entry_" + to_string(num + 1) + ":\n";
		code += "  %" + to_string(r + 3) + " = load %" + to_string(r) + "\n";
		return "%" + to_string(r + 3);
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
		int num = entry_num;
		entry_num += 2;
		int r = reg;
		reg += 4;
		code += "  %" + to_string(r) + " = alloc i32\n";
		code += "  store 0, %" + to_string(r) + "\n";
		string value_1 = landexp->Dump();
		code += "  %" + to_string(r + 1) + " = ne 0, " + value_1 + "\n";
		code += "  br %" + to_string(r + 1) + ", %entry_" + to_string(num) + ", %entry_" + to_string(num + 1) + "\n";

		code += "%entry_" + to_string(num) + ":\n";
		string value_2 = eqexp->Dump();
		code += "  %" + to_string(r + 2) + " = ne 0, " + value_2 + "\n";
		code += "  store %" + to_string(r + 2) + ", %" + to_string(r) + "\n";
		code += "  jump %entry_" + to_string(num + 1) + "\n";

		code += "%entry_" + to_string(num + 1) + ":\n";
		code += "  %" + to_string(r + 3) + " = load %" + to_string(r) + "\n";

		return "%" + to_string(r + 3);
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
	if (unaryexp != NULL) {
		string value_1 = unaryexp->Dump();
		string value_2 = unaryop->Dump();
		if (value_2 != "") {
			code += "  %" + to_string(reg) + " =" + value_2 + "0, " + value_1 + "\n";
			return "%" + to_string(reg++);
		}
		return value_1;
	}
	auto s = find(ident);
	string ans = "";
	if (s.type != FUNCVOID) {
		for (auto it = funcrparam->begin();it != funcrparam->end();++it) {
			if (it != funcrparam->begin())
				ans += ", ";
			ans += (*it)->Dump();
		}
		ans += ")\n";
		string head = "  %" + to_string(reg) + " = call @" + s.dump() + "(";
		code += head + ans;
		return "%" + to_string(reg++);
	}
	else {
		for (auto it = funcrparam->begin();it != funcrparam->end();++it) {
			if (it != funcrparam->begin())
				ans += ", ";
			ans += (*it)->Dump();
		}
		ans += ")\n";
		string head = "  call @" + s.dump() + "(";
		code += head + ans;
		return "";
	}
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
	if (it.type == VAR || it.type == FUNCPARAM) {
		string t = it.dump();
		code += "  %" + to_string(reg) + " = load " + t + "\n";
		return "%" + to_string(reg++);
	}
	else if (it.type == ARRAY) {
		string pre = it.dump();
		//解引用,默认第一个元素
		int size = exp->size();
		bool flag = (size != it.value);
		if (flag) {
			auto ast = new PrimaryExpAST();
			ast->number = 0;
			auto p = unique_ptr<SubBaseAST>(ast);
			exp->push_back(move(p));
		}
		for (auto i = exp->begin();i != exp->end();++i) {
			string t = (*i)->Dump();
			code += "  %" + to_string(reg) + " = getelemptr " + pre + ", " + t + "\n";
			pre = "%" + to_string(reg++);
		}
		if (flag) {
			return pre;
		}
		code += "  %" + to_string(reg) + " = load " + pre + "\n";
		return "%" + to_string(reg++);
	}
	else if (it.type == FUNCPARRAY) {
		string pre = it.dump();
		code += "  %" + to_string(reg) + " = load " + pre + "\n";
		pre = "%" + to_string(reg++);
		int size = exp->size();
		bool flag = (size != it.value);
		if (flag) {
			auto ast = new PrimaryExpAST();
			ast->number = 0;
			auto p = unique_ptr<SubBaseAST>(ast);
			exp->push_back(move(p));
		}
		for (auto i = exp->begin();i != exp->end();++i) {
			string t = (*i)->Dump();
			if (i == exp->begin()) {
				code += "  %" + to_string(reg) + " = getptr " + pre + ", " + t + "\n";
			}
			else {
				code += "  %" + to_string(reg) + " = getelemptr " + pre + ", " + t + "\n";
			}
			pre = "%" + to_string(reg++);
		}
		if (flag) {
			return pre;
		}
		code += "  %" + to_string(reg) + " = load " + pre + "\n";
		return "%" + to_string(reg++);
	}
	return to_string(it.value);
}

int LValAST::Cal()const {
	return find(ident).value;
}

string LValAST::Assign()const {
	auto it = find(ident);
	if (it.type == VAR||it.type==FUNCPARAM) {
		return it.dump();
	}
	else if (it.type == ARRAY) {
		string pre = it.dump();
		for (auto i = exp->begin();i != exp->end();++i) {
			string t = (*i)->Dump();
			code += "  %" + to_string(reg) + " = getelemptr " + pre + ", " + t + "\n";
			pre = "%" + to_string(reg++);
		}
		return pre;
	}
	else if (it.type == FUNCPARRAY) {
		string pre = it.dump();
		code += "  %" + to_string(reg) + " = load " + pre + "\n";
		pre = "%" + to_string(reg++);
		for (auto i = exp->begin();i != exp->end();++i) {
			string t = (*i)->Dump();
			if (i == exp->begin()) {
				code += "  %" + to_string(reg) + " = getptr " + pre + ", " + t + "\n";
			}
			else {
				code += "  %" + to_string(reg) + " = getelemptr " + pre + ", " + t + "\n";
			}
			pre = "%" + to_string(reg++);
		}
		return pre;

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

string Symbol::dump() {
	string prefix = "";
	if (type == FUNCINT || type == FUNCVOID)
		return name;
	if (type == VAR || type == ARRAY)
		prefix += "@";
	else if (type == FUNCPARAM || type == FUNCPARRAY)
		prefix += "%";
	return prefix + name + "_" + to_string(depth);
}

Symbol find(string name) {
	auto p = cur_tab;
	while (p) {
		for (auto it = p->table.begin();it != p->table.end();++it) {
			if (it->name == name)
				return *it;
		}
		p = p->pre;
	}
	assert(false);
}

bool exist(string name) {
	for (auto it = cur_tab->table.begin();it != cur_tab->table.end();++it) {
		if (it->name == name)
			return true;
	}
	return false;
}


void init() {
	code += "decl @getint(): i32\n";
	code += "decl @getch(): i32\n";
	code += "decl @getarray(*i32): i32\n";
	code += "decl @putint(i32)\n";
	code += "decl @putch(i32)\n";
	code += "decl @putarray(i32, *i32)\n";
	code += "decl @starttime()\n";
	code += "decl @stoptime()\n";
	Symbol s;
	s.name = "getint";
	s.type = FUNCINT;
	s.depth = 0;
	cur_tab->table.push_back(s);
	s.name = "getch";
	s.type = FUNCINT;
	s.depth = 0;
	cur_tab->table.push_back(s);
	s.name = "getarray";
	s.type = FUNCINT;
	s.depth = 0;
	cur_tab->table.push_back(s);
	s.name = "putint";
	s.type = FUNCVOID;
	s.depth = 0;
	cur_tab->table.push_back(s);
	s.name = "putch";
	s.type = FUNCVOID;
	s.depth = 0;
	cur_tab->table.push_back(s);
	s.name = "putarray";
	s.type = FUNCVOID;
	s.depth = 0;
	cur_tab->table.push_back(s);
	s.name = "starttime";
	s.type = FUNCVOID;
	s.depth = 0;
	cur_tab->table.push_back(s);
	s.name = "stoptime";
	s.type = FUNCVOID;
	s.depth = 0;
	cur_tab->table.push_back(s);
}