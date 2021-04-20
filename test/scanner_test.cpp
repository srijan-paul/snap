#include "assert.hpp"
#include "test_utils.hpp"
#include "token.hpp"
#include <scanner.hpp>

using namespace snap;
using TT = TokenType;

static bool compare_ttypes(const std::string* code, std::vector<TT> expected) {
	Scanner sc{code};
	for (auto tt : expected) {
		TT type = sc.next_token().type;
		if (type != tt) {
			std::printf("Expected token: ");
			print_ttype(tt);
			std::printf(" Got: ");
			print_ttype(type);
			std::printf("\n");
			return false;
		}
	}
	return true;
}

static bool run_tests() {
	bool passed = true;
	std::string code = "123 4.55 + - -= += >= >> << > < <=";
	passed = passed && compare_ttypes(&code, {TT::Integer, TT::Float, TT::Plus, TT::Minus,
																						TT::MinusEq, TT::PlusEq, TT::GtEq, TT::BitRShift,
																						TT::BitLShift, TT::Gt, TT::Lt, TT::LtEq});

	// test keyword and identifier scanning
	code = "let true false xyz else break continue";
	passed = passed && compare_ttypes(&code, {TT::Let, TT::True, TT::False, TT::Id, TT::Else,
																						TT::Break, TT::Continue, TT::Eof});

	code = "'this is a string' .. 'this is also string'";
	passed = passed && compare_ttypes(&code, {TT::String, TT::Concat, TT::String, TT::Eof});

	return passed;
}

int main() {
	if (run_tests()) return 0;
	return 1;
}