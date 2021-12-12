#include "assert.hpp"
#include "util/args.hpp"
#include "value.hpp"
#include <userdata.hpp>
#include <vector>
#include <vm.hpp>

using namespace vy;

// const v = cppVector()
// v:push_back({ foo: "bar" })
// assert(v[0].foo == "bar")
// assert(v:size() == 1)
// v:pop_back(); assert(v:size() == 0)

using ValueVector = std::vector<Value>;

Value vec_push(VM& vm, int argc) {
	util::Args args(vm, "Vector:push", 2, argc);
	auto& uvector = args.next<UserData>();
	Value value = args.next_arg();
	auto vec = static_cast<ValueVector*>(uvector.m_data);
	vec->push_back(value);
	return VYSE_NUM(vec->size());
}

void trace_cpp_vector(GC& gc, void* vector_) {
	auto vector = static_cast<ValueVector*>(vector_);
	for (Value& v : *vector) {
		gc.mark_value(v);
	}
}

void delete_cpp_vector(void* vector) {
	auto* vector_ptr = static_cast<ValueVector*>(vector);
	delete vector_ptr;
}

Value vec_new(VM& vm, int argc) {
	util::Args args(vm, "Vector:new", 1, argc);
	UserData& vec = vm.make<UserData>(new std::vector<Value>());
	vec.m_proto = &args.next<Table>();
	vec.delete_fn = delete_cpp_vector;
	vec.trace_fn = trace_cpp_vector;
	return VYSE_OBJECT(&vec);
}

Value vec_size(VM& vm, int argc) {
	util::Args args(vm, "Vector:size", 1, argc);
	auto& uvector = args.next<UserData>();
	return VYSE_NUM(static_cast<ValueVector*>(uvector.m_data)->size());
}

Value vec_pop(VM& vm, int argc) {
	util::Args args(vm, "Vector:pop", 1, argc);
	auto& uvector = args.next<UserData>();
	auto vec = static_cast<ValueVector*>(uvector.m_data);
	if (vec->size() == 0) return VYSE_NIL;
	Value last = vec->back();
	vec->pop_back();
	return last;
}

// tests

void udata_test() {
	VM vm;

#define S(s) VYSE_OBJECT(&vm.make_string(s))
#define F(f) VYSE_OBJECT(&vm.make<CClosure>(f))

	Table& vector_class = vm.make<Table>();
	vm.gc_off();
	vector_class.set(S("new"), F(vec_new));
	vector_class.set(S("push"), F(vec_push));
	vector_class.set(S("size"), F(vec_size));
	vector_class.set(S("pop"), F(vec_pop));
	vm.set_global("Vector", VYSE_OBJECT(&vector_class));
	vm.gc_on();

#undef S
#undef F

	vm.load_stdlib();
	ExitCode ec = vm.runcode(R"(
		const vec = Vector:new()
		vec:push(123)
		assert(vec:size() == 1)
	)");

	ASSERT(ec == ExitCode::Success, "Vyse C++ vector wrapper test failed!");
}

int main() {
	udata_test();
	printf("[userdata tests passed]\n");
	return 0;
}
