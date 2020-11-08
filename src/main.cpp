#include "scanner.hpp"
#include "token.hpp"
#include <cstdio>
#include <stdio.h>

using namespace snap;
using TT = snap::TokenType;

void print_ttype(TT type) {
	std::string type_strs[] = {
		"Integer", "Float",	  "String",		"Id",		  "Error",	  "Eof",	 "Plus",  "PlusEq",
		"Minus",   "MinusEq", "Mult",		"MultEq",	  "Div",	  "DivEq",	 "Mod",	  "ModEq",
		"Exp",	   "Eq",	  "Bang",		"BangEq",	  "Semi",	  "Dot",	 "Colon", "Comma",
		"LParen",  "RParen",  "LCurlBrace", "RCurlBrace", "LSqBrace", "RSqBrace"};

	const std::string& str = type_strs[(size_t)type];
	printf("%-10s", str.c_str());
}

void print_token(const Token& token, const std::string& src) {
	print_ttype(token.type);
	printf("'%s'", src.substr(token.pos.source_pos.start, token.pos.source_pos.end).c_str());
	printf("\n");
}

int main() {
	std::string s = "123 + 456 * 35.6";
	Scanner sc{&s};
	while (true) {
		const Token token = sc.next_token();
		print_token(token, s);
		if (token.type == TT::Eof)
			break;
	}
	return 0;
}