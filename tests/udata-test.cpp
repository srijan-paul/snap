#include "assert.hpp"
#include "userdata.hpp"
#include "util/args.hpp"
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
	ValueVector* const vec = uvector.get<ValueVector>();
	vec->push_back(value);
	return VYSE_NUM(vec->size());
}

Value vec_new(VM& vm, int argc) {
	util::Args args(vm, "Vector:new", 1, argc);
	auto& vec = vm.make_udata<ValueVector>(new std::vector<Value>());
	vec.m_proto = &args.next<Table>();

	vec.m_deleter = [](void* vector) { delete static_cast<ValueVector*>(vector); };
	vec.m_tracer = [](GC& gc, void* vector_) {
		auto vector = static_cast<ValueVector*>(vector_);
		for (Value& v : *vector) {
			gc.mark_value(v);
		}
	};

	return VYSE_OBJECT(&vec);
}

Value vec_get(VM& vm, int argc) {
	util::Args args(vm, "Vector:get", 2, argc);
	auto vec = args.next_udata_arg<ValueVector>();
	number index = args.next_number();

	if (vec == nullptr or index >= vec->size() or index < 0 or index != size_t(index)) {
		return VYSE_NIL;
	}

	return vec->at(size_t(index));
}

Value vec_size(VM& vm, int argc) {
	util::Args args(vm, "Vector:size", 1, argc);
	auto& uvector = args.next<UserData>();
	const auto vec = uvector.get<ValueVector>();
	return VYSE_NUM(vec->size());
}

Value vec_pop(VM& vm, int argc) {
	util::Args args(vm, "Vector:pop", 1, argc);
	auto& uvector = args.next<UserData>();
	auto vec = uvector.get<ValueVector>();
	if (vec->size() == 0) return VYSE_NIL;
	Value last = vec->back();
	vec->pop_back();
	return last;
}

// tests
/// TODO: finish this test suite.
// Test the `[]` operator
void test_udata_indexing() {
	VM vm;

	struct Counter {
		int value = 0;
	};

	auto counter_new = [](VM& vm, int argc) -> Value {
		util::Args args{vm, "Counter:new", 1, argc};
		Table& counter_class = args.next<Table>();
		auto ctrptr = new Counter{};

		UserData& udata = vm.make_udata<Counter>(ctrptr, &counter_class);
		udata.m_deleter = [](void* ctr) -> void { delete static_cast<Counter*>(ctr); };
		return VYSE_OBJECT(&udata);
	};

	auto counter_inc = [](VM& vm, int argc) -> Value {
		util::Args args{vm, "Counter:inc", 1, argc};
		Counter* counter = args.next_udata_arg<Counter>();
		if (counter != nullptr) {
			counter->value += 1;
		}
		return VYSE_NIL;
	};

	auto counter_get = [](VM& vm, int argc) -> Value {
		util::Args args{vm, "Counter:inc", 1, argc};
		Counter* counter = args.next_udata_arg<Counter>();
		if (counter == nullptr) return VYSE_NIL;
		return VYSE_NUM(counter->value);
	};

#define S(s) VYSE_OBJECT(&vm.make_string(s))
#define F(f) VYSE_OBJECT(&vm.make<CClosure>(f))

	Table& counter_class = vm.make<Table>();
	vm.gc_off();
	counter_class.set(S("new"), F(counter_new));
	counter_class.set(S("inc"), F(counter_inc));
	counter_class.set(S("get"), F(counter_get));
	vm.set_global("Counter", VYSE_OBJECT(&counter_class));
	vm.gc_on();

	vm.load_stdlib();
	auto res = vm.runcode(R"(
		const ctr = Counter:new()
		assert(ctr:get() == 0)
		ctr:inc(); ctr:inc(); ctr:inc()
		assert(ctr:get() == 3)
		ctr.get = 'hello'
		assert(ctr.get == 'hello')
		ctr['get'] = 2 
		assert(ctr.get == 2 && ctr['ge'..'t'] == 2)
	)");

	assert(res == ExitCode::Success);
#undef S
#undef F

}

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
	vector_class.set(S("get"), F(vec_get));
	vm.set_global("Vector", VYSE_OBJECT(&vector_class));
	vm.gc_on();

#undef S
#undef F

	vm.load_stdlib();
	ExitCode ec = vm.runcode(R"(
		const vec = Vector:new()
		Vector:new() --> this should get garbage collected on stress mode.
		vec:push(Vector:new())
		vec:push(Vector:new())
		assert(vec:size() == 2)
	)");

	ASSERT(ec == ExitCode::Success, "Vyse C++ vector wrapper test failed!");
}

int main() {
	udata_test();
	test_udata_indexing();
	printf("[userdata tests passed]\n");
	return 0;
}
