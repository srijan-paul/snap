#include <block.hpp>
#include <cassert>
#include <compiler.hpp>
#include <cstring>
#include <scanner.hpp>
#include <token.hpp>
#include <vm.hpp>

// if the condition `cond` is false then execute the `body` before exiting with a c style assert.
#define ASSERT_BODY(cond, body)                                                                    \
	{                                                                                              \
		if (!(cond)) {                                                                             \
			body;                                                                                  \
			abort();                                                                               \
		}                                                                                          \
	}

#define ASSERT_LOG(cond, message) ASSERT_BODY(cond, std::cout << message << "\n")

// if the condition is false then log out the message
inline void xassert(bool condition, std::string&& message) {
	if (!(condition)) {
		std::cout << message << std::endl;
		abort();
	}
}

#define VAL_INT_EQ(v, i) SNAP_IS_INT(v) && SNAP_AS_INT(v) == i

#define EXPECT_VAL_EQ(a, b)                                                                        \
	ASSERT_BODY(snap::Value::are_equal(a, b), {                                                    \
		std::printf("Expected values to be equal: ");                                              \
		print_value(a);                                                                            \
		std::printf("\n");                                                                         \
		print_value(b);                                                                            \
	})

#define EXPECT_STR_EQ(s1, s2)                                                                      \
	strlen(s1) == strlen(s2) && (std::memcmp(SNAP_AS_CSTRING(s1), s2, strlen(s1)) == 0)

#define EXPECT_VAL_STREQ(value, str)                                                               \
	ASSERT_LOG(value.is_string() && EXPECT_STR_EQ(SNAP_AS_CSTRING(value), str),                    \
			   "Mismatched string value. Expected: "                                               \
				   << "'abcdef' "                                                                  \
				   << "Got: '" << SNAP_AS_CSTRING(value) << "'");
