#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"

using namespace std;

// ���� lexer ������, �Լ� parser ����
// Ϊʲô������ sysy.tab.hpp ��? ��Ϊ��������û�� yyin �Ķ���
// ���, ��Ϊ����ļ����������Լ�д��, ���Ǳ� Bison ���ɳ�����
// ��Ĵ���༭��/IDE �ܿ����Ҳ�������ļ�, Ȼ�����㱨�� (��Ȼ���벻�����)
// ��������ܷ���, ���Ǹɴ�������ֿ����� dirty ��ʵ�ʺ���Ч���ֶ�
extern FILE* yyin;
extern int yyparse(unique_ptr<BaseAST>& ast);

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
	ast->Dump(yyout);
	return 0;
}