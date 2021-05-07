#include "common.hpp"
#include "debug.hpp"
#include "opcode.hpp"
#include "str_format.hpp"
#include <compiler.hpp>
#include <cstring>
#include <string>
#include <vm.hpp>

#define TOK2NUM(t) VYSE_NUM(std::stod(t.raw(*m_source)))
#define THIS_BLOCK (m_codeblock->block())
#define ERROR(...) (error_at_token(kt::format_str(__VA_ARGS__).c_str(), token))

#define DEFINE_PARSE_FN(name, cond, next_fn)                                                       \
	void name() {                                                                                    \
		next_fn();                                                                                     \
		while (cond) {                                                                                 \
			const Token op_token = token;                                                                \
			next_fn();                                                                                   \
			emit(toktype_to_op(op_token.type), op_token);                                                \
		}                                                                                              \
	}

namespace vyse {

using Op = Opcode;
using TT = TokenType;

Compiler::Compiler(VM* vm, const std::string* src) noexcept : m_vm{vm}, m_source{src} {
	m_scanner = new Scanner{src};
	advance(); // set `peek` to the first token in the token stream.
	String* fname = &vm->make_string("<script>", 8);
	// reserve the first slot for this toplevel function.
	m_symtable.add("<script>", 8, true);

	// When allocating [m_codeblock], the String "script"
	// not reachable by the VM, so we protect it from GC.
	m_vm->gc_protect(fname);
	m_codeblock = &vm->make<CodeBlock>(fname);

	// Now the string is reachable through the code
	// block, So we can unprotect it.
	m_vm->gc_unprotect(fname);
}

Compiler::Compiler(VM* vm, Compiler* parent, String* name) noexcept : m_vm{vm}, m_parent{parent} {
	m_scanner = m_parent->m_scanner;
	m_codeblock = &m_vm->make<CodeBlock>(name);

	m_symtable.add(name->c_str(), name->len(), false);

	m_source = parent->m_source;
	vm->m_compiler = this;

	prev = parent->prev;
	token = parent->token;
	peek = parent->peek;
}

Compiler::~Compiler() {
	// If this is the top-level compiler then we can
	// free the scanner assosciated with it.
	if (m_parent == nullptr) {
		delete m_scanner;
	}
}

CodeBlock* Compiler::compile() {
	while (!eof()) {
		toplevel();
	}

	emit(Op::load_nil, Op::return_val);
	m_codeblock->m_num_upvals = m_symtable.m_num_upvals;
	return m_codeblock;
}

CodeBlock* Compiler::compile_func() {
	test(TT::LCurlBrace, "Expected '{' before function body.");

	block_stmt();

	// In case the function does not return anything, we
	// add an implicit 'return nil'. If the closure *does*
	// return explicitly then these 2 opcodes will never be
	// reached anyway.
	emit(Op::load_nil, Op::return_val);
	m_codeblock->m_num_upvals = m_symtable.m_num_upvals;
	m_vm->m_compiler = m_parent;
	return m_codeblock;
}

// top level statements are one of:
// - var declaration
// - function declaration
// - if statement
// - loop (while/for)
// - expression statement
// - export statement
void Compiler::toplevel() {
	if (panic) recover();
	if (eof()) return;

	const TT tt = peek.type;

	switch (tt) {
	case TT::Const:
	case TT::Let: var_decl(); break;
	case TT::LCurlBrace: block_stmt(); break;
	case TT::If: if_stmt(); break;
	case TT::While: while_stmt(); break;
	case TT::For: for_stmt(); break;
	case TT::Fn: fn_decl(); break;
	case TT::Return: ret_stmt(); break;
	case TT::Break: break_stmt(); break;
	case TT::Continue: continue_stmt(); break;
	default: expr_stmt(); break;
	}
	match(TT::Semi);
}

void Compiler::var_decl() {
	advance(); // eat the 'let'|'const'
	const bool is_const = token.type == TT::Const;
	do {
		declarator(is_const);
	} while (match(TT::Comma));
}

void Compiler::declarator(bool is_const) {
	expect(TT::Id, "Expected variable name.");
	// add the new variable to the symbol table.
	const Token name = token;

	// default value for variables is 'nil'.
	match(TT::Eq) ? expr() : emit(Op::load_nil, token);
	new_variable(name, is_const);
}

void Compiler::block_stmt() {
	advance(); // eat the opening '{'
	enter_block();
	while (!(eof() or check(TT::RCurlBrace))) {
		toplevel();
	}
	expect(TT::RCurlBrace, "Expected '}' to close block.");
	exit_block();
}

void Compiler::if_stmt() {
	advance(); // consume 'if'
	expr();		 // parse condition.

	// If the condition is false, we simply pop
	// it and jump to the end of the if statement.
	// This puts as after the closing '}', which
	// might be an 'else' block sometimes.
	size_t jmp = emit_jump(Op::pop_jmp_if_false);
	toplevel();

	if (match(TT::Else)) {
		// If the 'if' block executed
		// then the 'else' shouldn't run, to
		// do this, we jump straight to the end
		// of the 'else' block after evaluating
		// the 'if'.
		size_t else_jmp = emit_jump(Op::jmp);
		patch_jump(jmp);
		toplevel();
		patch_jump(else_jmp);
		return;
	}

	patch_jump(jmp);
}

void Compiler::enter_loop(Loop& loop) {
	loop.enclosing = m_loop;
	// loop.start stores index of the first instruction in the loop
	// body/conditon. Which here is the next instruction to be emitted
	loop.start = THIS_BLOCK.op_count();
	loop.scope_depth = m_symtable.m_scope_depth;
	m_loop = &loop;
}

void Compiler::exit_loop(Op op_loop) {
	VYSE_ASSERT(m_loop != nullptr, "Attempt to exit loop in a top-level block.");

	int n_ops = THIS_BLOCK.op_count();
	// Emit the jmp_back instruction that connects the end of the
	// loop to the beginning.
	int back_jmp = emit_jump(op_loop);
	patch_backwards_jump(back_jmp, m_loop->start);

	for (int i = m_loop->start; i < n_ops;) {
		// no_op instructions are 'placeholders' for
		// jumps resulting from a break or continue statement.
		if (THIS_BLOCK.code[i] == Op::no_op) {

			// If it's a break statement then patch it to the
			// end of the loop.
			if (u8(THIS_BLOCK.code[i + 1]) == 0) {
				THIS_BLOCK.code[i] = Op::jmp;
				patch_jump(i + 1);
			} else {
				VYSE_ASSERT(u8(THIS_BLOCK.code[i + 1]) == 0xff, "Bad jump.");
				THIS_BLOCK.code[i] = (m_loop->loop_type == Loop::Type::While) ? Op::jmp_back : Op::for_loop;
				patch_backwards_jump(i + 1, m_loop->start);
			}
		}

		// It is crucial that we increment the counter by the
		// current instruction's arity (number of operands needed)
		// instead of simply doing `i++`. If we do `i++` then we'd
		// be reading operands to other instructions like `load_const`
		// as bytecode instructions and misinterpreting the values.
		i += op_arity(i) + 1;
	}

	m_loop = m_loop->enclosing;
}

void Compiler::discard_loop_locals(u32 depth) {
	VYSE_ASSERT(m_symtable.m_scope_depth > depth, "Bad call to discard_locals.");

	for (int i = m_symtable.m_num_symbols - 1; i >= 0; i--) {
		const LocalVar& var = m_symtable.m_symbols[i];
		if (var.depth <= depth) break;
		emit(var.is_captured ? Op::close_upval : Op::pop);
	}
}

void Compiler::break_stmt() {
	advance(); // consume 'break' token.
	if (m_loop == nullptr) {
		ERROR("break statement outside a loop.");
		return;
	}

	/// remove all the local variables inside the
	/// loop's body before jumping out of it.
	discard_loop_locals(m_loop->scope_depth);

	// A 'break' statement's jump instruction is
	// characterized by a `no_op` instruction. So when
	// the `Compiler::loop_exit` method is patching old loops, it
	// doesn't get confused between 'jmp' instructions from
	// break statements and 'jmp' instructions from other
	// statements like "if".
	int jmp = emit_jump(Op::no_op);

	// A no_op instruction followed by a 0x00
	// is recognized as a 'break' statement.
	// Later the 0x00 and the following byte are
	// changed into the actual jump offset.
	THIS_BLOCK.code[jmp] = Op(0x00);
}

void Compiler::continue_stmt() {
	advance(); // consume 'continue'
	if (m_loop == nullptr) {
		ERROR("continue statement outside a loop.");
		return;
	}

	discard_loop_locals(m_loop->scope_depth);

	// continue statement jumps that need to be patched
	// later are marked with a `no_op` instruction followed
	// by two `0xff`s.
	emit_jump(Op::no_op);
}

void Compiler::while_stmt() {
	advance(); // consume 'while'

	Loop loop(Loop::Type::While);
	enter_loop(loop);

	expr(); // parse condition.
	u32 jmp = emit_jump(Opcode::pop_jmp_if_false);
	toplevel();
	exit_loop(Op::jmp_back);
	patch_jump(jmp);
}

void Compiler::for_stmt() {
	advance(); // consume the 'for'
	expect(TT::Id, "Expected for-loop variable.");

	const Token name = token;

	// Enter the scope for the for loop
	// Note that the loop iterator variable belongs
	// inside this block.
	enter_block();


	new_variable("<for-start>", 11);
	// Loop start
	expect(TT::Eq, "Expected '=' after for-loop variable.");
	expr();


	// Loop limit
	expect(TT::Comma, "Expected ',' to separate for-loop variable and limit.");
	new_variable("<for-limit>", 11);
	expr();



	// Optional loop step, 1 by default.
	new_variable("<for-step>", 10);
	if (match(TT::Comma)) {
		expr();
	} else {
		int idx = emit_value(VYSE_NUM(1));
		emit_with_arg(Op::load_const, idx);
	}

	// Add the actual loop variable that is
	// exposed to the user. (i)
	new_variable(name);
	size_t prep_jump = emit_jump(Op::for_prep);

	// Loop body
	Loop loop(Loop::Type::For);
	enter_loop(loop);
	toplevel();
	patch_jump(prep_jump);
	exit_loop(Op::for_loop);

	exit_block();
}

void Compiler::fn_decl() {
	advance(); // consume 'fn' token.
	expect(TT::Id, "expected function name");

	const Token name_token = token;
	String* fname = &m_vm->make_string(name_token.raw_cstr(*m_source), name_token.length());

	func_expr(fname);
	new_variable(name_token);
}

// Compile the body of a function or method assuming everything until the
// the opening parenthesis has been consumed.
void Compiler::func_expr(String* fname, bool is_method) {
	// When compiling the body of the function, we allocate
	// a code block; at that point in time, the name of the
	// function is not reachable by the Garbage Collector,
	// so we protect it.
	m_vm->gc_protect(fname);
	Compiler compiler{m_vm, this, fname};

	compiler.expect(TT::LParen, "Expected '(' before function parameters.");

	int param_count = 0;

	// Methods have an implicit 'self' parameter, used to
	// reference the object itself.
	if (is_method) {
		++param_count;
		compiler.add_self_param();
	}

	if (!compiler.check(TT::RParen)) {
		do {
			compiler.expect(TT::Id, "Expected parameter name.");
			compiler.add_param(compiler.token);
			++param_count;
		} while (compiler.match(TT::Comma));
	}

	if (param_count > MaxFuncParams) {
		compiler.error_at_token("Function cannot have more than 200 parameters", compiler.token);
	}

	compiler.expect(TT::RParen, "Expected ')' after function parameters.");

	CodeBlock* code = compiler.compile_func();
	if (compiler.has_error) has_error = true;
	const u8 idx = emit_value(VYSE_OBJECT(code));

	emit(Op::make_func);
	emit_arg(idx);
	emit_arg(code->m_num_upvals);

	// Now, [fname] can be reached via the code block itself.
	m_vm->gc_unprotect(fname);

	for (int i = 0; i < compiler.m_symtable.m_num_upvals; ++i) {
		const UpvalDesc& upval = compiler.m_symtable.m_upvals[i];

		// An operand of '1' means that the upvalue exists in the call
		// frame of the currently executing function while this closure is
		// being created. '0' means the upvalue exists in the current
		// function's upvalue list.
		emit_arg(upval.is_local ? 1 : 0);
		emit_arg(upval.index);
	}

#ifdef VYSE_DEBUG_DISASSEMBLY
	disassemble_block(code->name_cstr(), code->block());
#endif

	// synchronize the parent compiler with the child so we can continue compiling
	// from where the child left off.
	prev = compiler.prev;
	token = compiler.token;
	peek = compiler.peek;
}

void Compiler::ret_stmt() {
	advance(); // eat the 'return' keyword.
	// If the next token marks the start of an expression, then
	// compile this statement as `return EXPR`, else it's just a `return`.
	// where a `nil` after the return is implicit.
	if (is_literal(peek.type) or check(TT::Id) or check(TT::Bang) or check(TT::Minus) or
			check(TT::LParen) or check(TT::Fn) or check(TT::LCurlBrace)) {
		expr();
	} else {
		emit(Op::load_nil);
	}
	emit(Op::return_val);
}

void Compiler::expr_stmt() {
	ExpKind prefix_type = prefix();
	// If a toplevel assignment statement
	// was already compiled then exit.
	if (prefix_type == ExpKind::none) return;

	// Otherwise keep parsing to get a valid
	// assignment statement.
	complete_expr_stmt(prefix_type);

	/// TODO: The pop instruction here can be implicit
	/// for 'set' and 'index' instructions.
	emit(Op::pop);
}

/// Compiling expression statements ///
// There are no 'expression statements' as such in vyse.
// The only valid toplevel expression is a function call.
// Assignment ('a=b') is strictly a statement and not allowed
// in expression contexts.
//
// A call expression can be an arbitrarily long suffixed
// expression followed by a '()'. So all of the following
// are valid call expressions:
// 1. foo()
// 2. foo.bar()
// 3. t['k']()
// 4. foo:bar().baz['key']()
// 5. f()()()
// 6. g()['k']()
//
// Similarly, all of the following are valid assignment
// statements:
// 1. a = 1
// 2. a.b = 1
// 3. t['k'] = 1
// 4. t['k'].bar().baz = 123
//
// To make sure the all the above cases are covered, we clearly define
// what is and isn't a valid LHS for assignment.
// The following is the grammar for a valid assignment statement:
//
// ASSIGN            := LHS ASSIGN_TOK EXPRESSION
// LHS               := ID | (SUFFIXED_EXP ASSIGNABLE_SUFFIX)
// SUFFIXED_EXP      := (PREFIX | SUFFIXED_EXP) SUFFIX
// PREFIX            := (ID | STRING | NUMBER | BOOLEAN)
// SUFFIX            := ASSIGNABLE_SUFFIX | '('ARGS')' | METHOD_CALL
// ASSIGNABLE_SUFFIX := '[' EXPRESSION ']' | '.'ID
// METHOD_CALL       := ':' ID '(' ARGS ')'
// ASSIGN_TOK        := '=' | '+=' | '-=' | '*=' | '%=' | '/=' | '//='
//
// It may be noticed that parsing an assignment when the LHS can be an arbitrarily
// long chain of '.', '[]', '()' etc will require indefinite backtracking, or some
// form of an AST. To tackle this, we follow a simple idea:
// 		Keep track of the current state / expr kind in while parsing, if we see an '=' and
// 		the last state was a valid assignment LHS, then we compile an assignment.

void Compiler::complete_expr_stmt(ExpKind prefix_type) {
	ExpKind exp_kind = prefix_type;

	// Keep compiling the rest of the prefix, however if a
	// an '=' is seen, then compile a top level assignment
	// and stop.
	while (true) {
		switch (peek.type) {
		case TT::LSqBrace: {
			advance();
			expr();
			expect(TT::RSqBrace, "Expected ']' to close index expression.");
			if (is_assign_tok(peek.type)) {
				table_assign(Op::index_no_pop, -1);
				emit(Op::index_set);
				return;
			} else {
				emit(Op::index);
				exp_kind = ExpKind::prefix;
			}
			break;
		}

		case TT::LParen: {
			if (exp_kind == ExpKind::literal_value) {
				ERROR("Unexpected '('");
				return;
			}
			compile_args();
			exp_kind = ExpKind::call;
			break;
		}

		case TT::Dot: {
			advance();
			expect(TT::Id, "Expected field name.");
			const u8 index = emit_id_string(token);

			if (is_assign_tok(peek.type)) {
				table_assign(Op::table_get_no_pop, index);
				emit_with_arg(Op::table_set, index);
				return;
			} else {
				exp_kind = ExpKind::prefix;
				emit_with_arg(Op::table_get, index);
			}
			break;
		}

		case TT::Colon: {
			advance();
			expect(TT::Id, "Expected method name.");
			u8 index = emit_id_string(token);
			emit_with_arg(Op::prep_method_call, index);
			compile_args(true);
			exp_kind = ExpKind::call;
			break;
		}

		default: {
			if (exp_kind == ExpKind::call) return;
			// If the expression type that was compiled last is not a
			// method or closure call, and we haven't found
			// a proper LHS for assignment yet, then this is
			// incorrect source code.
			ERROR("Unexpected expression.");
			return;
		}
		}
	}
}

// Compile a prefix or a variable assignment.
// A prefix can be (as described above):
// (ID | STRING | NUMBER | BOOLEAN)
// A var assignment is simply ID '=' EXPRESSION
ExpKind Compiler::prefix() {
	if (check(TT::LParen)) {
		grouping();
		return ExpKind::prefix;
	}

	if (match(TT::Id)) {
		if (is_assign_tok(peek.type)) {
			variable(true);
			// Since we have successfully compiled a valid toplevel
			// statement, it is longer an expression. We use
			// ExpKind::none to indicate this.
			return ExpKind::none;
		} else {
			variable(false);
			return ExpKind::prefix;
		}
	}

	if (!is_literal(peek.type)) {
		ERROR("Unexpected '{}'.", peek.raw(*m_source));
		return ExpKind::literal_value;
	}

	if (peek.type == TT::Nil) {
		ERROR("Unexpected 'nil'.");
		return ExpKind::none;
	}

	literal();
	return ExpKind::literal_value;
}

void Compiler::expr() {
	logic_or();
}

void Compiler::logic_or() {
	logic_and();
	if (match(TT::Or)) {
		const std::size_t jump = emit_jump(Op::jmp_if_true_or_pop);
		logic_or();
		patch_jump(jump);
	}
}

void Compiler::logic_and() {
	bit_or();
	if (match(TT::And)) {
		std::size_t jump = emit_jump(Op::jmp_if_false_or_pop);
		logic_and();
		patch_jump(jump);
	}
}

DEFINE_PARSE_FN(Compiler::bit_or, match(TT::BitOr), bit_and)
DEFINE_PARSE_FN(Compiler::bit_and, match(TT::BitAnd), equality)
DEFINE_PARSE_FN(Compiler::equality, match(TT::EqEq) or match(TT::BangEq), comparison)
DEFINE_PARSE_FN(Compiler::comparison,
								match(TT::Gt) or match(TT::Lt) or match(TT::GtEq) or match(TT::LtEq), b_shift)
DEFINE_PARSE_FN(Compiler::b_shift, match(TT::BitLShift) or match(TT::BitRShift), sum)
DEFINE_PARSE_FN(Compiler::sum, (match(TT::Plus) or match(TT::Minus) or match(TT::Concat)), mult)
DEFINE_PARSE_FN(Compiler::mult, (match(TT::Mult) or match(TT::Mod) or match(TT::Div)), unary)

void Compiler::unary() {
	if (check(TT::Bang) or check(TT::Minus)) {
		advance();
		const Token op_token = token;
		atomic();
		switch (op_token.type) {
		case TT::Bang: emit(Op::lnot, op_token); break;
		case TT::Minus: emit(Op::negate, op_token); break;
		default: VYSE_ERROR("Impossible token.");
		}
		return;
	}
	atomic();
}

void Compiler::atomic() {
	grouping();
	suffix_expr();
}

void Compiler::suffix_expr() {
	while (true) {
		switch (peek.type) {
		case TT::LSqBrace: {
			advance();
			expr();
			expect(TT::RSqBrace, "Expected ']' to close index expression.");
			emit(Op::index);
			break;
		}
		case TT::LParen: compile_args(); break;
		case TT::Dot: {
			advance();
			expect(TT::Id, "Expected field name.");
			const u8 index = emit_id_string(token);
			emit_with_arg(Op::table_get, index);
			break;
		}
		case TT::Colon: {
			advance();
			expect(TT::Id, "Expected method name.");
			u8 index = emit_id_string(token);
			emit_with_arg(Op::prep_method_call, index);
			compile_args(true);
			break;
		}
		default: return;
		}
	}
}

void Compiler::table_assign(Op get_op, int idx) {
	VYSE_ASSERT(is_assign_tok(peek.type), "Bad call to Compiler::table_assign");

	advance();
	const TT ttype = token.type;

	/// if this is a simple assignment with '=' operator,
	/// then simply compile the RHS as an expression and
	/// return to the call site, wherein it's the caller's
	/// responsibility to emit the 'set' opcode.
	if (ttype == TT::Eq) {
		expr();
		return;
	}

	/// If we've reached here then it must be a compound
	/// assignment operator. So we first need to get the
	/// original field value, push it on top of the stack,
	/// modify this value then use a 'set' opcode to store
	/// it back into the table/array. The 'set' opcode is
	/// emitted by the caller.
	emit(get_op);
	if (idx > 0) emit_arg(idx);
	expr();
	emit(toktype_to_op(ttype));
}

void Compiler::compile_args(bool is_method) {
	advance(); // eat opening '('

	/// If it's a method call, then start with 1
	/// argument count for the implicit 'self' argument.
	u32 argc = is_method ? 1 : 0;

	if (!check(TT::RParen)) {
		do {
			++argc;
			if (argc > MaxFuncParams) ERROR("Too many arguments to function call.");
			expr(); // push the arguments on the stack,
		} while (match(TT::Comma));
	}

	expect(TT::RParen, "Expected ')' after call.");
	emit_with_arg(Op::call_func, argc);
}

void Compiler::grouping() {
	if (match(TT::LParen)) {
		expr();
		expect(TT::RParen, "Expected ')' after grouping expression.");
		return;
	}
	primary();
}

void Compiler::primary() {
	if (is_literal(peek.type)) {
		literal();
	} else if (match(TT::Fn)) {
		static constexpr const char* name = "<anonymous>";
		String* fname = &m_vm->make_string(name, strlen(name));
		if (check(TT::LParen)) return func_expr(fname);
		// Names of lambda expressions are simply ignored
		// Unless found in a statement context.
		expect(TT::Id, "Expected function name or '('.");
		return func_expr(fname);
	} else if (match(TT::Id)) {
		variable(false);
	} else if (match(TT::LCurlBrace)) {
		table();
	} else {
		ERROR("Unexpected '{}'.", peek.raw(*m_source));
		advance();
	}
}

void Compiler::table() {
	emit(Opcode::new_table);

	/// empty table.
	if (match(TT::RCurlBrace)) return;

	do {
		if (match(TT::LSqBrace)) {
			/// a computed table key like in { [1 + 2]: 3 }
			expr();
			expect(TT::RSqBrace, "Expected ']' near table key.");
		} else {
			expect(TT::Id, "Expected identifier as table key.");
			String* key_string = &m_vm->make_string(token.raw_cstr(*m_source), token.length());
			const int key_idx = emit_value(VYSE_OBJECT(key_string));
			emit_with_arg(Op::load_const, key_idx);
			if (check(TT::LParen)) {
				func_expr(key_string, true);
				emit(Op::table_add_field);
				if (check(TT::RCurlBrace)) break;
				continue;
			}
		}

		expect(TT::Colon, "Expected ':' after table key.");
		expr();
		emit(Op::table_add_field);

		if (check(TT::RCurlBrace)) break;
	} while (!eof() and match(TT::Comma));

	if (eof()) {
		error("Reached end of file while compiling.");
		return;
	}

	expect(TT::RCurlBrace, "Expected '}' to close table or ',' to separate entry.");
}

void Compiler::variable(bool can_assign) {
	Op get_op = Op::get_var;
	Op set_op = Op::set_var;

	int index = find_local_var(token);
	bool is_const = (index == -1) ? false : m_symtable.find_by_slot(index)->is_const;

	// if no local variable with that name was found then look for an
	// upvalue.
	if (index == -1) {
		index = find_upvalue(token);

		if (index == -1) {
			get_op = Opcode::get_global;
			set_op = Opcode::set_global;
			index = emit_id_string(token);
		} else {
			get_op = Opcode::get_upval;
			set_op = Opcode::set_upval;
			is_const = m_symtable.m_upvals[index].is_const;
		}
	}

	if (can_assign) {
		VYSE_ASSERT(is_assign_tok(peek.type), "Not in an assignment context.");
		if (is_const) {
			std::string message =
					kt::format_str("Cannot assign to variable '{}' marked const.", token.raw(*m_source));
			error_at_token(message.c_str(), token);
			panic = false; // Don't send the compiler into error recovery mode.
		}

		/// Compile the RHS of the assignment, and any necessary arithmetic ops if its a
		/// compound assignment operator. So by the time we are setting the value, the RHS
		/// is sitting ready on top of the stack.
		var_assign(get_op, index);
		emit_with_arg(set_op, index);
	} else {
		emit_with_arg(get_op, index);
	}
}

void Compiler::var_assign(Op get_op, u32 idx_or_name_str) {
	advance();
	const TT ttype = token.type;
	VYSE_ASSERT(is_assign_tok(ttype), "Bad call to Compiler::var_assign");
	if (ttype == TT::Eq) {
		expr();
		return;
	}

	emit_with_arg(get_op, idx_or_name_str);
	expr();
	emit(toktype_to_op(ttype));
}

void Compiler::literal() {
	advance();
	u32 index = 0;
	switch (token.type) {
	case TT::Integer:
	case TT::Float: index = emit_value(TOK2NUM(token)); break;
	case TT::String: index = emit_string(token); break;
	case TT::True: index = emit_value(VYSE_BOOL(true)); break;
	case TT::False: index = emit_value(VYSE_BOOL(false)); break;
	case TT::Nil: {
		emit(Op::load_nil);
		return;
	}
	default: VYSE_UNREACHABLE();
	}

	if (index > MaxLocalVars) {
		ERROR("Too many literal constants in one function.");
	}

	/// TODO: handle indices larger than UINT8_MAX, by adding a
	/// load_const_long instruction that takes 2 operands.
	emit_with_arg(Op::load_const, static_cast<u8>(index));
}

void Compiler::recover() {
	while (!eof()) {
		advance();
		if (check(TT::Semi)) break;
	}
	panic = false;
}

void Compiler::enter_block() noexcept {
	++m_symtable.m_scope_depth;
}

void Compiler::exit_block() {
	for (int i = m_symtable.m_num_symbols - 1; i >= 0; i--) {
		const LocalVar& var = m_symtable.m_symbols[i];
		if (var.depth != m_symtable.m_scope_depth) break;
		emit(var.is_captured ? Op::close_upval : Op::pop);
		--m_symtable.m_num_symbols;
	}
	--m_symtable.m_scope_depth;
}

size_t Compiler::emit_jump(Opcode op) {
	size_t index = THIS_BLOCK.op_count();
	emit(op);
	emit_arg(0xff);
	emit_arg(0xff);
	return index + 1;
}

void Compiler::patch_jump(size_t index) {
	u32 jump_dist = THIS_BLOCK.op_count() - index - 2;
	if (jump_dist > UINT16_MAX) {
		ERROR("Too much code to jump over");
		return;
	}

	// If we use a single opcode for the jump offset, then we're only allowed
	// to jump over 255 instructions, which is unfortunately a very small number.
	// To counter this, jumps are broken down into two Ops, the first Op contains the
	// first byte and the second Op contains the second byte. The bytes are then stitched
	// together to form a short int, thus allowing us to jump over UINT16_MAX bytes of code
	// (65535 instructions) at best.

	// For example, if we want to jump over 25000 opcodes, that means our jump offset
	// is `0x61A8`. The first byte, `0x61` goes in the first opcode, and the second
	// byte, `0xA8` goes in the second opcode. At runtime, the VM reads both of these,
	// and joins them together using some bit operators.
	THIS_BLOCK.code[index] = static_cast<Op>((jump_dist >> 8) & 0xff);
	THIS_BLOCK.code[index + 1] = static_cast<Op>(jump_dist & 0xff);
}

void Compiler::patch_backwards_jump(size_t index, u32 dst_index) {
	u32 distance = index - dst_index + 2;
	if (distance > UINT16_MAX) {
		ERROR("Too much code to jump over.");
		return;
	}
	THIS_BLOCK.code[index] = static_cast<Op>((distance >> 8) & 0xff);
	THIS_BLOCK.code[index + 1] = static_cast<Op>(distance & 0xff);
}

void Compiler::add_param(const Token& token) {
	new_variable(token);
	m_codeblock->add_param();
}

void Compiler::add_self_param() {
	VYSE_ASSERT(m_symtable.m_num_symbols == 1, "'self' must be the first parameter.");

	constexpr const char* self = "self";
	m_codeblock->add_param();
	m_symtable.add(self, 4, true);
}

// Helper functions:

void Compiler::advance() {
	prev = token;
	token = peek;
	peek = m_scanner->next_token();
}

bool Compiler::match(TT expected) noexcept {
	if (check(expected)) {
		advance();
		return true;
	}
	return false;
}

void Compiler::expect(TT expected, const char* err_msg) {
	if (check(expected)) {
		advance();
		return;
	}

	error_at_token(err_msg, token);
}

void Compiler::test(TT expected, const char* errmsg) {
	if (!check(expected)) {
		error_at_token(errmsg, peek);
	}
}

void Compiler::error_at(const char* message, u32 line) {
	error(kt::format_str("[line {}]: {}", line, message));
}

void Compiler::error_at_token(const char* message, const Token& token) {
	error(kt::format_str("[line {}]: near '{}': {}", token.location.line, token.raw(*m_source),
											 message));
}

void Compiler::error(std::string&& message) {
	if (panic) return;
	m_vm->on_error(*m_vm, message);
	has_error = true;
	panic = true;
}

bool Compiler::ok() const noexcept {
	return !has_error;
}

u32 Compiler::emit_string(const Token& token) {
	u32 length = token.length() - 2; // minus the quotes
	// +1 to skip the openening quote.
	String& string = m_vm->make_string(token.raw_cstr(*m_source) + 1, length);
	return emit_value(VYSE_OBJECT(&string));
}

u32 Compiler::emit_id_string(const Token& token) {
	String* s = &m_vm->make_string(token.raw_cstr(*m_source), token.length());
	return emit_value(VYSE_OBJECT(s));
}

int Compiler::find_local_var(const Token& name_token) const noexcept {
	const char* name = name_token.raw_cstr(*m_source);
	int length = name_token.length();
	const int idx = m_symtable.find(name, length);
	return idx;
}

int Compiler::find_upvalue(const Token& token) {
	if (m_parent == nullptr) return -1;

	// First search among the local variables of the enclosing
	// compiler.
	int index = m_parent->find_local_var(token);

	// If found the local var, then add it to the upvalues list.
	// and mark the upvalue is "local".
	if (index != -1) {
		LocalVar& local = m_parent->m_symtable.m_symbols[index];
		local.is_captured = true;
		return m_symtable.add_upvalue(index, true, local.is_const);
	}

	// If not found within the parent compiler's local vars
	// then look into the parent compiler's upvalues.
	index = m_parent->find_upvalue(token);

	// If found in some enclosing scope, add it to the current
	// upvalues list and return it.
	if (index != -1) {
		// is not local since we found it in an enclosing compiler.
		const UpvalDesc& upval = m_parent->m_symtable.m_upvals[index];
		return m_symtable.add_upvalue(index, false, upval.is_const);
	}

	// No local variable in any of the enclosing scopes was found with the same
	// name.
	return -1;
}

size_t Compiler::emit_value(Value v) {
	size_t index = THIS_BLOCK.add_value(v);
	if (index >= Compiler::MaxLocalVars) {
		error_at_token("Too many constants in a single block.", token);
	}
	return index;
}

inline void Compiler::emit(Op op) {
	int stack_effect = op_stack_effect(op);
	m_stack_size += stack_effect;
	if (m_stack_size > m_codeblock->max_stack_size) {
		m_codeblock->max_stack_size = m_stack_size;
	}

	emit(op, token);
}

inline void Compiler::emit(Op op, const Token& token) {
	THIS_BLOCK.add_instruction(op, token.location.line);
}

inline void Compiler::emit_arg(u8 operand) {
	THIS_BLOCK.add_instruction(static_cast<Op>(operand), token.location.line);
}

inline void Compiler::emit_with_arg(Op op, u8 arg) {
	emit(op);
	emit_arg(arg);
}

inline void Compiler::emit(Op a, Op b) {
	emit(a, token);
	emit(b, token);
}

Op Compiler::toktype_to_op(TT toktype) const noexcept {
	switch (toktype) {
	case TT::Plus:
	case TT::PlusEq: return Op::add;
	case TT::Minus:
	case TT::MinusEq: return Op::sub;
	case TT::Div:
	case TT::DivEq: return Op::div;
	case TT::Mult:
	case TT::MultEq: return Op::mult;
	case TT::Mod:
	case TT::ModEq: return Op::mod;
	case TT::EqEq: return Op::eq;
	case TT::BangEq: return Op::neq;
	case TT::Concat: return Op::concat;
	case TT::BitLShift: return Op::lshift;
	case TT::BitRShift: return Op::rshift;
	case TT::BitAnd: return Op::band;
	case TT::BitOr: return Op::bor;
	case TT::Gt: return Op::gt;
	case TT::Lt: return Op::lt;
	case TT::GtEq: return Op::gte;
	case TT::LtEq: return Op::lte;
	default: VYSE_UNREACHABLE();
	}
}

#define CHECK_ARITY(x, y) ((x) >= (Op_##y##_operands_start) and ((x) <= (Op_##y##_operands_end)))
int Compiler::op_arity(u32 op_index) const noexcept {
	Op op = THIS_BLOCK.code[op_index];
	if (op == Op::make_func) {
		VYSE_ASSERT(op_index != THIS_BLOCK.op_count() - 1, "Op::make_func cannot be the last opcode");
		int n_upvals = int(THIS_BLOCK.code[op_index + 1]);
		return 1 + n_upvals * 2;
	}

	if (CHECK_ARITY(op, 0)) return 0;
	if (CHECK_ARITY(op, 1)) return 1;

	// Constant instructions take 1 operand: the index of the
	// constant in the constant pool.
	if (op >= Op_const_start and op <= Op_const_end) return 1;
	VYSE_ASSERT(CHECK_ARITY(op, 2), "Instructions other than make_func can have upto 2 operands.");
	return 2;
}
#undef CHECK_ARITY

int Compiler::op_stack_effect(Op op) const noexcept {
#define OP(_, __, stack_effect) stack_effect
	constexpr std::array<int, size_t(Op::no_op) + 1> stack_effects = {
#include "x_opcode.hpp"
	};
#undef OP

	return stack_effects[size_t(op)];
}

bool Compiler::is_assign_tok(TT type) const noexcept {
	return (type == TT::Eq or (type >= TT::ModEq and type <= TT::PlusEq));
}

// We need two overloads for Compiler::new_variable.
// On one hand, it's nice to be able to call new_variable(token)
// and not extract the name and length of the name at the call site.
// But sometimes we will want to add dummy variables (like for-loop limits).
// And we don't have a token for those to take the name from.

int Compiler::new_variable(const Token& varname, bool is_const) {
	const char* name = varname.raw_cstr(*m_source);
	const u32 length = varname.length();
	return new_variable(name, length, is_const);
}

int Compiler::new_variable(const char* name, u32 length, bool is_const) {
	// check of a variable with this name already exists in the current scope.
	if (m_symtable.find_in_current_scope(name, length) != -1) {
		ERROR("Attempt to redeclare existing variable '{}'.", std::string_view(name, length));
		return -1;
	}

	return m_symtable.add(name, length, is_const);
}

// -- LocalVar Table --

int SymbolTable::add(const char* name, u32 length, bool is_const = false) {
	m_symbols[m_num_symbols] = LocalVar{name, length, u8(m_scope_depth), is_const};
	return m_num_symbols++;
}

static bool names_equal(const char* a, int len_a, const char* b, int len_b) {
	if (len_a != len_b) return false;
	return std::memcmp(a, b, len_a) == 0;
}

int SymbolTable::find(const char* name, int length) const {
	// start looking from the innermost scope, and work our way
	// outwards.
	for (int i = m_num_symbols - 1; i >= 0; i--) {
		const LocalVar& symbol = m_symbols[i];
		if (names_equal(name, length, symbol.name, symbol.length)) return i;
	}
	return -1;
}

int SymbolTable::find_in_current_scope(const char* name, int length) const {
	for (int i = m_num_symbols - 1; i >= 0; i--) {
		const LocalVar& symbol = m_symbols[i];
		if (symbol.depth < m_scope_depth) return -1; // we've reached an outer scope
		if (names_equal(name, length, symbol.name, symbol.length)) return i;
	}
	return -1;
}

const LocalVar* SymbolTable::find_by_slot(const u8 index) const {
	return &m_symbols[index];
}

int SymbolTable::add_upvalue(int index, bool is_local, bool is_const) {
	// If the upvalue has already been captured, then return the stored value.
	for (int i = 0; i < m_num_upvals; ++i) {
		UpvalDesc& upval = m_upvals[i];
		if (upval.index == index and upval.is_local == is_local) {
			return i;
		}
	}

	m_upvals[m_num_upvals] = UpvalDesc{index, is_const, is_local};
	return m_num_upvals++;
}

} // namespace vyse
