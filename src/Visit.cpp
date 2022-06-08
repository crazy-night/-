#include "Visit.h"
using namespace std;

string text = "";
string prologue = "";
string epilogue = "";
map<koopa_raw_value_t, int>Map;
int current;
int reg_num = 0;

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
	int stack_size = Cal(func->bbs);
	int m = stack_size % 16;
	stack_size = m ? stack_size - m + 16 : stack_size;
	if (stack_size > 2047) {
		prologue = "  li t0, " + to_string(-stack_size) + "\n" + "  add sp, sp, t0\n";
		epilogue = "  li t0, " + to_string(stack_size) + "\n" + "  add sp, sp, t0\n";
	}
	else {
		prologue = "  addi sp, sp, " + to_string(-stack_size) + "\n";
		epilogue = "  addi sp, sp, " + to_string(stack_size) + "\n";
	}
	text += prologue;
	current = 0;
	// 访问所有基本块
	Visit(func->bbs);
	return "";
}
int Cal(const koopa_raw_slice_t& blocks) {
	int r = 0;
	for (size_t i = 0; i < blocks.len; ++i) {
		auto ptr = blocks.buffer[i];
		auto insts = reinterpret_cast<koopa_raw_basic_block_t>(ptr)->insts;
		for (size_t j = 0; j < insts.len; ++j) {
			auto ptr = insts.buffer[i];
			auto inst = reinterpret_cast<koopa_raw_value_t>(ptr);
			//???似乎同在线文档不一致，store指令也有返回值？？？
			if (inst->ty->tag != KOOPA_RTT_UNIT) {
				r += 4;
			}
			else if (inst->kind.tag == KOOPA_RVT_ALLOC) {
				r += 4;
			}
		}
	}
	return r;
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
	if (it != Map.end()) {
		int i = it->second;
		string reg = stack_lw(i);
		text += "  lw t0, " + reg + "\n";
		return "t0";
	}
	const auto& kind = value->kind;
	switch (kind.tag) {
	case KOOPA_RVT_ALLOC:
		Map.insert(make_pair(value, current));
		current += 4;
		break;
	case KOOPA_RVT_LOAD:
		Visit(kind.data.load);
		Map.insert(make_pair(value, current));
		current += 4;
		break;
	case KOOPA_RVT_STORE:
		Visit(kind.data.store);
		break;
	case KOOPA_RVT_GET_PTR:
		break;
	case KOOPA_RVT_GET_ELEM_PTR:
		break;
	case KOOPA_RVT_BINARY:
		//访问二进制算式指令
		Visit(kind.data.binary);
		Map.insert(make_pair(value, current));
		current += 4;
		break;
	case KOOPA_RVT_BRANCH:
		break;
	case KOOPA_RVT_JUMP:
		break;
	case KOOPA_RVT_CALL:
		break;
	case KOOPA_RVT_RETURN:
		// 访问 return 指令
		Visit(kind.data.ret);return"";
	case KOOPA_RVT_INTEGER:
		// 访问 integer 指令
		return Visit(kind.data.integer);

	default:
		// 其他类型暂时遇不到
		assert(false);
	}
	return "";
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...


string Visit(const koopa_raw_load_t& load) {
	Visit(load.src);
	string reg = stack_lw(current);
	text += "  sw t0, " + reg + "\n";
	return "";
}

string Visit(const koopa_raw_store_t& store) {
	string s = Visit(store.value);
	string d;
	auto it = Map.find(store.dest);
	if (it != Map.end()) {
		int i = it->second;
		d = stack_lw(i);
	}
	else
		assert(false);
	string ans = "  sw " + s + ", " + d + "\n";
	text += ans;
	return "";
}

string Visit(const koopa_raw_return_t& ret) {
	string ans = "  mv a0, " + Visit(ret.value) + "\n";
	text += ans;
	text += epilogue + "  ret\n";
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
	string reg = "t0";
	switch (binary.op)
	{
	case KOOPA_RBO_SUB:
		text += "  sub t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_EQ:
		text += "  xor t0, " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz t0, t0\n";
		break;
	case KOOPA_RBO_ADD:
		text += "  add t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_MUL:
		text += "  mul t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_DIV:
		text += "  div t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_MOD:
		text += "  mod t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_AND:
		text += "  and t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_OR:
		text += "  or t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_LT:
		text += "  slt t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_GT:
		text += "  sgt t0, " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_NOT_EQ:
		text += "  xor t0, " + reg_1 + ", " + reg_2 + "\n";
		text += "  snez t0" + reg + ", " + reg + "\n";
		break;
	case KOOPA_RBO_LE:
		text += "  sgt t0, " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz t0, t0\n";
		break;
	case KOOPA_RBO_GE:
		text += "  slt t0, " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz t0, t0\n";
		break;
	default:
		assert(false);
		return NULL;
	}
	reg = stack_lw(current);
	text += "  sw t0, " + reg + "\n";
	return "";

}

string reg_distribute() {
	reg_num++;
	reg_num %= 7;
	reg_num = reg_num ? reg_num : reg_num + 1;
	return "t" + to_string(reg_num);
}

string stack_lw(int i) {
	if (i > 2047) {
		text += "  li t1, " + to_string(i) + "\n";
		text += "  add t1, sp, t1\n"; 
		return "0(t1)";
	}
	else
		return to_string(i) + "(sp)";
}

