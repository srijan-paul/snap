#include <cassert>
#include <compiler.hpp>
#include <cstring>
#include <iostream>
#include <token.hpp>
#include <vm.hpp>

// if the condition `cond` is false then execute the `body` before exiting.
#define ASSERT_BODY(cond, body)                                                                    \
	{                                                                                              \
		if (!(cond)) {                                                                             \
			body;                                                                                  \
			exit(1);                                                                               \
		}                                                                                          \
	}

#define ASSERT(cond, message)                                                                      \
	ASSERT_BODY(cond, fprintf(stderr, "[%s - %d]: ", __func__, __LINE__);                          \
				std::cout << message << '\n';)

// if the condition is false then log out the message
inline void xassert(bool condition, std::string&& message) {
	if (!(condition)) {
		fprintf(stderr, "[%s - %d]: %s\n", __func__, __LINE__, message.c_str());
		abort();
	}
}

#define VAL_NUM_EQ(v, i) SNAP_AS_NUM(v) && SNAP_AS_NUM(v) == i

#define EXPECT_VAL_EQ(a, b)                                                                        \
	ASSERT_BODY(a == b, {                                                                          \
		std::printf("Expected values to be equal: ");                                              \
		print_value(a);                                                                            \
		std::printf("\t");                                                                         \
		print_value(b);                                                                            \
	})

#define EXPECT_STR_EQ(s1, s2)                                                                      \
	strlen(s1) == strlen(s2) && (std::memcmp(SNAP_AS_CSTRING(s1), s2, strlen(s1)) == 0)

#define EXPECT(cond, message)                                                                      \
	do {                                                                                           \
		if (!(cond)) {                                                                             \
			std::cout << "[FAILED] " << message << std::endl;                                      \
			abort();                                                                               \
		}                                                                                          \
	} while (false)

#define EXPECT_VAL_STREQ(value, str)                                                               \
	ASSERT_LOG(value.is_string() && EXPECT_STR_EQ(SNAP_AS_CSTRING(value), str),                    \
			   "Mismatched string value. Expected: "                                               \
				   << "'abcdef' "                                                                  \
				   << "Got: '" << SNAP_AS_CSTRING(value) << "'");
