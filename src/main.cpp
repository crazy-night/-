#include <cassert>
#include <cstdio>
#include <iostream>
#include<sstream>
#include <memory>
#include <string>
#include<string.h>
#include "AST.h"
#include "koopa.h"
#include "Visit.h"


using namespace std;

// ���� lexer ������, �Լ� parser ����
// Ϊʲô������ sysy.tab.hpp ��? ��Ϊ��������û�� yyin �Ķ���
// ���, ��Ϊ����ļ����������Լ�д��, ���Ǳ� Bison ���ɳ�����
// ��Ĵ���༭��/IDE �ܿ����Ҳ�������ļ�, Ȼ�����㱨�� (��Ȼ���벻�����)
// ��������ܷ���, ���Ǹɴ�������ֿ����� dirty ��ʵ�ʺ���Ч���ֶ�
extern FILE* yyin;
extern int yyparse(unique_ptr<BaseAST>& ast);

extern string text;



int main(int argc, const char* argv[]) {
	// ���������в���. ���Խű�/����ƽ̨Ҫ����ı������ܽ������²���:
	// compiler ģʽ �����ļ� -o ����ļ�
	assert(argc == 5);
	auto mode = argv[1];
	auto input = argv[2];
	auto output = argv[4];

	// �������ļ�, ����ָ�� lexer �ڽ�����ʱ���ȡ����ļ�
	yyin = fopen(input, "r");
	assert(yyin);

	FILE* yyout = fopen(output, "w");
	assert(yyout != NULL);

	// parse input file
	unique_ptr<BaseAST> ast;
	auto ret = yyparse(ast);
	assert(!ret);

	// dump AST
	string s = ast->Dump();

	if (strcmp(mode, "-riscv") == 0) {
		// �����ַ��� str, �õ� Koopa IR ����
		koopa_program_t program;
		koopa_error_code_t ret = koopa_parse_from_string(s.c_str(), &program);
		assert(ret == KOOPA_EC_SUCCESS);  // ȷ������ʱû�г���
		// ����һ�� raw program builder, �������� raw program
		koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
		// �� Koopa IR ����ת��Ϊ raw program
		koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
		// �ͷ� Koopa IR ����ռ�õ��ڴ�
		koopa_delete_program(program);

		// ���� raw program
		Visit(raw);
		fprintf(yyout, "%s", text.c_str());

		// �������, �ͷ� raw program builder ռ�õ��ڴ�
		// ע��, raw program �����е�ָ��ָ����ڴ��Ϊ raw program builder ���ڴ�
		// ���Բ�Ҫ�� raw program �������֮ǰ�ͷ� builder
		koopa_delete_raw_program_builder(builder);
	}
	else if (strcmp(mode, "-koopa") == 0) {
		fprintf(yyout, "%s", s.c_str());
	}
	return 0;
}