#include "assert.hpp"
#include "test_utils.hpp"
#include "token.hpp"
#include <scanner.hpp>

using namespace snap;
using TT = TokenType;

static void assert_token(snap::TokenType a, snap::TokenType b) {
	if (a != b) {
		printf("Expected token: ");
		print_ttype(a);
		printf(" Got: ");
		print_ttype(b);
		printf("\n");
	}

	assert(a == b);
}

static bool compare_ttypes(const std::string* code, std::vector<TT> expected) {
	Scanner sc{code};
	for (auto tt : expected) {
		assert_token(tt, sc.next_token().type);
	}
	return true;
}

static void run_tests() {
	std::string code = "123 4.55 + - -= += >= >> << > < <=";
	compare_ttypes(&code, {TT::Integer, TT::Float, TT::Plus, TT::Minus,
											  TT::MinusEq, TT::PlusEq, TT::GtEq, TT::BitRShift,
											  TT::BitLShift, TT::Gt, TT::Lt, TT::LtEq});

	// test keyword and identifier scanning
	code = "let true false xyz";
	compare_ttypes(&code, {TT::Let, TT::True, TT::False, TT::Id, TT::Eof});

	code = "'this is a string' .. 'this is also string'";
	compare_ttypes(&code, {TT::String, TT::Concat, TT::String, TT::Eof});
}

int main() {
    run_tests();
    return 0;
}