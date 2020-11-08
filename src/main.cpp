#include "scanner.hpp"
#include "token.hpp"
#include <cstdio>
#include <stdio.h>

using namespace snap;
using TT = snap::TokenType;

void print_ttype(TT type) {
	std::string type_strs[] = {
		"Number",  "String", "Id",	   "Error", "Eof",	 "Plus", "PlusEq", "Minus",
		"MinusEq", "Mult",	 "MultEq", "Div",	"DivEq", "Mod",	 "ModEq",  "Exp",
	};

	printf("%s", type_strs[(size_t)type].c_str());
}

void print_token(const Token& token, const std::string& src) {
	printf("'%s' ", src.substr(token.pos.source_pos.start, token.pos.source_pos.end).c_str());
	print_ttype(token.type);
	printf("\n");
}

int main() {
	std::string s = "+-";
	Scanner sc{&s};
	while (true) {
		const Token token = sc.next_token();
		print_token(token, s);
		if (token.type == TT::Eof)
			break;
	}
	return 0;
}