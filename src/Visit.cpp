#include "Visit.h"
#define GLOBALVAR -2
#define GLOBALARRAY -3

using namespace std;

string text = "";
string prologue = "";
string epilogue = "";
map<koopa_raw_value_t, vector<int>>Dim;
map<koopa_raw_value_t, int>Map;
int current;
int reg_num = 0;
bool ra = false;
int calling = 0;
bool ret_type = false;
int stack_size = 0;

//caller-saved reg;
int t[7] = { 0 };
//callee-saved reg;
int a[8] = { 0 };

// 访问 raw program
string Visit(const koopa_raw_program_t& program) {
	// 执行一些其他的必要操作
	// 访问所有全局变量,全局函数
	text = "";
	if (!Map.empty())
		Map.clear();
	if (program.values.len) {
		text += "  .data\n";
		Visit(program.values);
	}
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
	if (func->bbs.len == 0)
		return "";
	string name = parse(func->name);
	ra = false;
	text += "  .text\n";
	text += "  .globl " + name + "\n" + name + ":\n";
	stack_size = Cal(func->bbs);
	prologue = "";
	epilogue = "";
	if (stack_size) {
		int m = stack_size % 16;
		stack_size = m ? stack_size - m + 16 : stack_size;
		if (stack_size > 2047) {
			prologue += "  li t0, " + to_string(-stack_size) + "\n" + "  add sp, sp, t0\n";
			if (ra) {
				prologue += "  li t0, " + to_string(stack_size - 4) + "\n" + "  add t0, sp, t0\n" + "  sw ra, 0(t0)\n";
				epilogue += "  li t0, " + to_string(stack_size - 4) + "\n" + "  add t0, sp, t0\n" + "  lw ra, 0(t0)\n";
			}
			epilogue += "  li t0, " + to_string(stack_size) + "\n" + "  add sp, sp, t0\n";
		}
		else {
			prologue += "  addi sp, sp, " + to_string(-stack_size) + "\n";
			if (ra) {
				prologue += "  sw ra, " + to_string(stack_size - 4) + "(sp)\n";
				epilogue += "  lw ra, " + to_string(stack_size - 4) + "(sp)\n";
			}
			epilogue += "  addi sp, sp, " + to_string(stack_size) + "\n";
		}
	}
	text += prologue;
	// 访问所有基本块
	Visit(func->bbs);
	return "";
}
//计算函数需要多大的栈
int Cal(const koopa_raw_slice_t& blocks) {
	//为局部变量分配的栈空间
	int s = 0;
	//为 ra 分配的栈空间 
	int r = 0;
	//为传参预留的栈空间
	int a = 0;
	for (size_t i = 0; i < blocks.len; ++i) {
		auto ptr = blocks.buffer[i];
		auto insts = reinterpret_cast<koopa_raw_basic_block_t>(ptr)->insts;
		for (size_t j = 0; j < insts.len; ++j) {
			auto ptr = insts.buffer[j];
			auto inst = reinterpret_cast<koopa_raw_value_t>(ptr);
			if (inst->ty->tag == KOOPA_RTT_POINTER && inst->kind.tag == KOOPA_RVT_ALLOC) {
				auto it = inst->ty->data.pointer.base;
				int size = 4, sum = 1;
				while (it->tag == KOOPA_RTT_ARRAY)
				{
					sum *= it->data.array.len;
					it = it->data.array.base;
				}
				s += sum * size;
			}
			else if (inst->ty->tag != KOOPA_RTT_UNIT)
				s += 4;
			if (inst->kind.tag == KOOPA_RVT_CALL) {
				r = 4;
				a = max((int)inst->kind.data.call.args.len - 8, a);
			}
		}
	}
	if (r)
		ra = true;
	current = a * 4;
	//text += "?" + to_string(s) + "@"+to_string(t)+" :"+ to_string(r) + "!" + to_string(a) + "\n";
	return s + r + a * 4;
}

// 访问基本块
string Visit(const koopa_raw_basic_block_t& bb) {
	// 执行一些其他的必要操作
	string s = bb->name;
	if (bb->name != NULL && s.compare("%entry")) {
		string ans = parse(s);
		text += ans + ":\n";
	}
	// 访问所有指令
	Visit(bb->insts);
	text += "\n";
	return "";
}

// 访问指令
string Visit(const koopa_raw_value_t& value) {
	// 根据指令类型判断后续需要如何访问
	auto it = Map.find(value);
	// 已执行
	if (it != Map.end()) {
		int i = it->second;
		int args = calling < 9 ? 0 : calling;
		string reg = reg_distribute();
		if (i == GLOBALVAR) {
			text += "  la " + reg + ", " + parse(it->first->name) + "\n";
			text += "  lw " + reg + ", 0(" + reg + ")\n";
			return reg;
		}
		string src = stack_lw(i);
		text += "  lw " + reg + ", " + src + "\n";
		// 存九个及以上参数
		if (args) {
			string dest = stack_lw((args - 9) * 4);
			text += "  sw " + reg + ", " + dest + "\n";
		}
		return reg;
	}
	/*switch (value->ty->tag) {
	case KOOPA_RTT_INT32:text += "0\n";break;
	case KOOPA_RTT_UNIT:text += "1\n";break;
	case KOOPA_RTT_ARRAY:text += "2\n";break;
	case KOOPA_RTT_POINTER:text += "3\n";break;
	default:break;
	}*/
	ret_type = (value->ty->tag != KOOPA_RTT_UNIT);
	const auto& kind = value->kind;
	int type = 0;
	switch (kind.tag) {
		//alloc 是指针，参考原llvm的指针,附带了原数组的信息，不需要存了
	case KOOPA_RVT_ALLOC:
	{
		Map.insert(make_pair(value, current));
		if (value->ty->tag == KOOPA_RTT_POINTER) {
			auto it = value->ty->data.pointer.base;
			int size = 4, sum = 1;
			while (it->tag == KOOPA_RTT_ARRAY)
			{
				sum *= it->data.array.len;
				it = it->data.array.base;
			}
			current += sum * size;
		}
		else
			current += 4;
		break;
	}
	case KOOPA_RVT_LOAD:
		Visit(kind.data.load);
		Map.insert(make_pair(value, current));
		current += 4;
		break;
	case KOOPA_RVT_STORE:
		Visit(kind.data.store);
		break;
	case KOOPA_RVT_GET_PTR:
		Visit(kind.data.get_ptr);
		Map.insert(make_pair(value, current));
		current += 4;
		break;
	case KOOPA_RVT_GET_ELEM_PTR:
		Visit(kind.data.get_elem_ptr);
		Map.insert(make_pair(value, current));
		current += 4;
		break;
	case KOOPA_RVT_BINARY:
		//访问二进制算式指令
		Visit(kind.data.binary);
		Map.insert(make_pair(value, current));
		current += 4;
		break;
	case KOOPA_RVT_BRANCH:
		Visit(kind.data.branch);
		break;
	case KOOPA_RVT_JUMP:
		Visit(kind.data.jump);
		break;
	case KOOPA_RVT_CALL:
		Visit(kind.data.call);
		if (ret_type) {
			Map.insert(make_pair(value, current));
			current += 4;
		}
		break;
	case KOOPA_RVT_RETURN:
		// 访问 return 指令
		Visit(kind.data.ret);
		return"";
	case KOOPA_RVT_INTEGER:
		// 访问 integer 指令
		return Visit(kind.data.integer);
	case KOOPA_RVT_GLOBAL_ALLOC:
		text += "  .globl " + parse(value->name) + "\n" + parse(value->name) + ":\n";
		type = stoi(Visit(kind.data.global_alloc));
		Map.insert(make_pair(value, type));
		break;
	case KOOPA_RVT_FUNC_ARG_REF:
		return Visit(kind.data.func_arg_ref);
	default:
		// 其他类型暂时遇不到
		assert(false);
	}
	return "";
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...
string Visit(const koopa_raw_func_arg_ref_t& func_arg_ref) {
	auto i = func_arg_ref.index;
	if (i < 8)
		return "a" + to_string(i);
	else {
		string reg = reg_distribute();
		string src = stack_lw(stack_size + (i - 8) * 4);
		text += "  lw " + reg + ", " + src + "\n";
		return reg;
	}
}

string Visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr) {
	string s = reg_distribute();
	auto it = Map.find(get_elem_ptr.src);
	if (it != Map.end()) {
		int i = it->second;
		if (i == GLOBALARRAY) {
			s = reg_distribute();
			text += "  la " + s + ", " + parse(it->first->name) + "\n";
		}
		else if (get_elem_ptr.src->kind.tag == KOOPA_RVT_ALLOC) {
			s = reg_distribute();
			if (i > 2047) {
				text += "  li " + s + ", " + to_string(i) + "\n";
				text += "  add " + s + ", sp, " + s + "\n";
			}
			else
				text += "  addi " + s + ", sp, " + to_string(i) + "\n";
		}
		else {
			string reg = stack_lw(i);
			text += "  lw " + s + ", " + reg + "\n";
		}
	}
	else
		assert(false);
	string i = Visit(get_elem_ptr.index);
	auto p = get_elem_ptr.src->ty->data.pointer.base->data.array.base;
	int size = 4, sum = 1;
	while (p->tag == KOOPA_RTT_ARRAY)
	{
		sum *= p->data.array.len;
		p = p->data.array.base;
	}
	string reg_1 = reg_distribute();
	string reg_2 = reg_distribute();
	text += "  li " + reg_1 + ", " + to_string(sum * size) + "\n";
	text += "  mul " + reg_2 + ", " + reg_1 + ", " + i + "\n";
	text += "  add " + reg_2 + ", " + reg_2 + ", " + s + "\n";
	string reg = stack_lw(current);
	text += "  sw " + reg_2 + ", " + reg + "\n";
	return "";
}

string Visit(const koopa_raw_get_ptr_t& get_ptr) {
	string s = Visit(get_ptr.src);
	string i = Visit(get_ptr.index);
	auto it = get_ptr.src->ty->data.pointer.base;
	int size = 4, sum = 1;
	while (it->tag == KOOPA_RTT_ARRAY)
	{
		sum *= it->data.array.len;
		it = it->data.array.base;
	}
	string reg_1 = reg_distribute();
	string reg_2 = reg_distribute();
	text += "  li " + reg_1 + ", " + to_string(sum * size) + "\n";
	text += "  mul " + reg_2 + ", " + reg_1 + ", " + i + "\n";
	text += "  add " + reg_2 + ", " + reg_2 + ", " + s + "\n";
	string reg = stack_lw(current);
	text += "  sw " + reg_2 + ", " + reg + "\n";
	return "";
}

string Visit(const koopa_raw_global_alloc_t& global_alloc) {
	auto it = global_alloc.init;
	if (it != NULL) {
		if (it->ty->tag == KOOPA_RTT_INT32) {
			if (it->kind.tag == KOOPA_RVT_INTEGER) {
				text += "  .word " + to_string(it->kind.data.integer.value) + "\n";
			}
			else if (it->kind.tag == KOOPA_RVT_ZERO_INIT) {
				text += "  .zero 4\n";
			}
			else
				assert(false);
			return "-2";
		}
		else if (it->ty->tag == KOOPA_RTT_ARRAY) {
			if (it->kind.tag == KOOPA_RVT_ZERO_INIT) {
				auto value = it->ty;
				int size = 4, sum = 1;
				while (value->tag == KOOPA_RTT_ARRAY)
				{
					sum *= value->data.array.len;
					value = value->data.array.base;
				}
				text += "  .zero " + to_string(sum * size) + "\n";
			}
			else if (it->kind.tag == KOOPA_RVT_AGGREGATE) {
				Visit(it->kind.data.aggregate);
			}
			else
				assert(false);
			return "-3";
		}
		else
			assert(false);
	}
	return "";
}

string Visit(const koopa_raw_aggregate_t& aggregate) {
	auto it = aggregate.elems;
	for (size_t i = 0;i < it.len;++i) {
		auto ptr = it.buffer[i];
		auto inst = reinterpret_cast<koopa_raw_value_t>(ptr);
		if (inst->kind.tag == KOOPA_RVT_INTEGER) {
			text += "  .word " + to_string(inst->kind.data.integer.value) + "\n";
		}
		else if (inst->kind.tag == KOOPA_RVT_AGGREGATE) {
			Visit(inst->kind.data.aggregate);
		}
		else
			assert(false);
	}
	return "";
}

string Visit(const koopa_raw_call_t& call) {
	string name = parse(call.callee->name);
	calling = 1;
	bool f = ret_type;
	Visit(call.args);
	calling = 0;
	ret_type = f;
	text += "  call " + name + "\n";
	if (f) {
		string reg = stack_lw(current);
		text += "  sw a0, " + reg + "\n";
	}
	return "";
}
string Visit(const koopa_raw_branch_t& branch) {
	string name = parse(branch.true_bb->name);
	text += "  bnez " + Visit(branch.cond) + ", " + name + "\n";
	name = parse(branch.false_bb->name);
	text += "  j " + name + "\n";
	return "";
}

string Visit(const koopa_raw_jump_t& jump) {
	string name = parse(jump.target->name);
	text += "  j " + name + "\n";
	return "";
}

string Visit(const koopa_raw_load_t& load) {

	string s = Visit(load.src);
	if (load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || load.src->kind.tag == KOOPA_RVT_GET_PTR) {
		text += "  lw " + s + ", (" + s + ")\n";
	}
	string reg = stack_lw(current);
	text += "  sw " + s + ", " + reg + "\n";
	return "";
}

string Visit(const koopa_raw_store_t& store) {
	string s = Visit(store.value);
	string d;
	auto it = Map.find(store.dest);
	if (it != Map.end()) {
		int i = it->second;
		if (i == GLOBALVAR) {
			string reg = reg_distribute();
			text += "  la " + reg + ", " + parse(it->first->name) + "\n";
			d = "0(" + reg + ")";
		}
		else if (store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || store.dest->kind.tag == KOOPA_RVT_GET_PTR) {
			string reg = stack_lw(i);
			d = reg_distribute();
			text += "  lw " + d + ", " + reg + "\n";
			d = "0(" + d + ")";
		}
		else
			d = stack_lw(i);
	}
	else
		assert(false);
	string ans = "  sw " + s + ", " + d + "\n";
	text += ans;
	return "";
}

string Visit(const koopa_raw_return_t& ret) {
	if (ret.value != NULL) {
		string ans = "  mv a0, " + Visit(ret.value) + "\n";
		text += ans;
	}
	text += epilogue + "  ret\n";
	return "";
}


string Visit(const koopa_raw_integer_t& integer) {
	int32_t val = integer.value;
	if (!calling && val == 0)
		return "x0";
	string reg = reg_distribute();
	text += "  li " + reg + ", " + to_string(val) + "\n";
	if (calling > 9) {
		text += "  sw " + reg + ", " + to_string((calling - 10) * 4) + "(sp)\n";
	}
	return reg;
}

string Visit(const koopa_raw_binary_t& binary) {
	string reg_1 = Visit(binary.lhs);
	string reg_2 = Visit(binary.rhs);
	string reg_3 = reg_distribute();
	switch (binary.op)
	{
	case KOOPA_RBO_SUB:
		text += "  sub " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_EQ:
		text += "  xor " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz " + reg_3 + ", " + reg_3 + "\n";
		break;
	case KOOPA_RBO_ADD:
		text += "  add " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_MUL:
		text += "  mul " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_DIV:
		text += "  div " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_MOD:
		text += "  rem " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_AND:
		text += "  and " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_OR:
		text += "  or " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_LT:
		text += "  slt " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_GT:
		text += "  sgt " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		break;
	case KOOPA_RBO_NOT_EQ:
		text += "  xor " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  snez " + reg_3 + ", " + reg_3 + "\n";
		break;
	case KOOPA_RBO_LE:
		text += "  sgt " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz " + reg_3 + ", " + reg_3 + "\n";
		break;
	case KOOPA_RBO_GE:
		text += "  slt " + reg_3 + ", " + reg_1 + ", " + reg_2 + "\n";
		text += "  seqz " + reg_3 + ", " + reg_3 + "\n";
		break;
	default:
		assert(false);
		return NULL;
	}
	string reg = stack_lw(current);
	text += "  sw " + reg_3 + ", " + reg + "\n";
	return "";

}
//寄存器分配
string reg_distribute() {
	if (calling) {
		if (calling < 9) {
			string ans = "a" + to_string(calling - 1);
			++calling;
			return ans;
		}
		++calling;
	}
	reg_num++;
	reg_num %= 7;
	//reg_num = reg_num ? reg_num : reg_num + 1;
	return "t" + to_string(reg_num);
}
//溢出转变
string stack_lw(int i) {
	if (i > 2047) {
		int temp = calling;
		calling = 0;
		string reg = reg_distribute();
		calling = temp;
		text += "  li " + reg + ", " + to_string(i) + "\n";
		text += "  add " + reg + ", sp, " + reg + "\n";
		return "0(" + reg + ")";
	}
	else
		return to_string(i) + "(sp)";
}

string parse(string name) {
	return name.substr(1, name.length());
}

