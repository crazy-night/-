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
std::string Visit(const koopa_raw_basic_block_t&);
std::string Visit(const koopa_raw_value_t&);
std::string Visit(const koopa_raw_return_t&);
std::string Visit(const koopa_raw_integer_t&);
std::string Visit(const koopa_raw_binary_t&);
std::string reg_distribute();
#endif