#ifndef Visit_H
#define Visit_H
#include<cassert>
#include<string>
#include<iostream>
#include<map>
#include"koopa.h"


std::string Visit(const koopa_raw_program_t&);
std::string Visit(const koopa_raw_slice_t&);
std::string Visit(const koopa_raw_function_t&);
int Cal(const koopa_raw_slice_t&);
std::string Visit(const koopa_raw_basic_block_t&);
std::string Visit(const koopa_raw_value_t&);
std::string Visit(const koopa_raw_branch_t&);
std::string Visit(const koopa_raw_jump_t&);
std::string Visit(const koopa_raw_return_t&);
std::string Visit(const koopa_raw_integer_t&);
std::string Visit(const koopa_raw_binary_t&);
std::string Visit(const koopa_raw_load_t&);
std::string Visit(const koopa_raw_store_t&);
std::string reg_distribute();
std::string stack_lw(int);
std::string parse(std::string);
#endif