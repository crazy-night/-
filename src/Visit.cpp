#include "Visit.h"
using namespace std;

string text = "";
map<koopa_raw_value_t, string>Map;
int t[7] = { 0 };
int a[7] = { 0 };

// 访问 raw program
string Visit(const koopa_raw_program_t& program) {
	// 执行一些其他的必要操作
	// 访问所有全局变量,全局函数
	text = "";
	if (!Map.empty())
		Map.clear();
	text += "  .text\n";
	Visit(program.values);
	Visit(program.funcs);
	return "";
}

// 访问 raw slice
string Visit(const koopa_raw_slice_t& slice) {
	for (size_t i = 0; i < slice.len; ++i) {
		auto ptr = slice.buffer[i];
		// 根据 slice 的 kind 决定将 ptr 视作何种元素
		switch (slice.kind) {
		case KOOPA_RSIK_FUNCTION:
			// 访问函数
			Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
			break;
		case KOOPA_RSIK_BASIC_BLOCK:
			// 访问基本块
			Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
			break;
		case KOOPA_RSIK_VALUE:
			// 访问指令
			Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
			break;
		default:
			// 我们暂时不会遇到其他内容, 于是不对其做任何处理
			assert(false);
		}
	}
	return "";
}

// 访问函数
string Visit(const koopa_raw_function_t& func) {
	// 执行一些其他的必要操作
	string name = func->name;
	name = name.substr(1, name.length());
	text += "  .globl " + name + "\n" + name + ":\n";
	// 访问所有基本块
	Visit(func->bbs);
	return "";
}

// 访问基本块
string Visit(const koopa_raw_basic_block_t& bb) {
	// 执行一些其他的必要操作
	// ...
	// 访问所有指令
	Visit(bb->insts);
	return "";
}

// 访问指令
string Visit(const koopa_raw_value_t& value) {
	// 根据指令类型判断后续需要如何访问
	auto it = Map.find(value);
	if (it != Map.end())
		return it->second;
	const auto& kind = value->kind;
	switch (kind.tag) {
	case KOOPA_RVT_RETURN:
		// 访问 return 指令
		Visit(kind.data.ret);return"";
	case KOOPA_RVT_INTEGER:
		// 访问 integer 指令
		Map.insert(make_pair(value, Visit(kind.data.integer)));break;
	case KOOPA_RVT_BINARY:
		//访问二进制算式指令
		Map.insert(make_pair(value, Visit(kind.data.binary)));break;
	default:
		// 其他类型暂时遇不到
		assert(false);
	}
	return Map[value];
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...
string Visit(const koopa_raw_return_t& ret) {
	string ans = "  mv a0, " + Visit(ret.value) + "\n  ret\n";
	text += ans;
	return "";
}


string Visit(const koopa_raw_integer_t& integer) {
	int32_t val = integer.value;
	if (val == 0)
		return "x0";
	string reg = reg_distribute();
	text += "  li " + reg + ", " + to_string(val) + "\n";
	return reg;
}

string Visit(const koopa_raw_binary_t& binary) {
	string reg_1 = Visit(binary.lhs);
	string reg_2 = Visit(binary.rhs);
	string reg = reg_distribute();
	switch (binary.op)
	{
	case KOOPA_RBO_SUB:
		text += "  sub " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_EQ:
		text += "  xor " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz " + reg + ", " + reg + "\n";
		break;
	case KOOPA_RBO_ADD:
		text += "  add " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_MUL:
		text += "  mul " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_DIV:
		text += "  div " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_MOD:
		text += "  mod " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_AND:
		text += "  and " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_OR:
		text += "  or " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_LT:
		text += "  slt " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_GT:
		text += "  sgt " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_NOT_EQ:
		text += "  xor " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  snez " + reg + ", " + reg + "\n";
		break;
	case KOOPA_RBO_LE:
		text += "  sgt " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz " + reg + ", " + reg + "\n";
		break;
	case KOOPA_RBO_GE:
		text += "  slt " + reg + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz " + reg + ", " + reg + "\n";
		break;
	default:
		assert(false);
		return NULL;
	}
	return reg;

}

string reg_distribute() {
	for (int i = 0;i < 7;++i) {
		if (t[i] == 0) {
			t[i] = 1;
			return "t" + to_string(i);
		}
	}
	for (int i = 1;i < 7;++i) {
		if (a[i] == 0) {
			a[i] = 1;
			return "a" + to_string(i);
		}
	}
	return NULL;
}
