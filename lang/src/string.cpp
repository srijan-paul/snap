#include <algorithm>
#include <cassert>
#include <cstring>
#include <string.hpp>

namespace vyse {

// this hash function is from: https://craftinginterpreters.com/hash-tables.html
u32 hash_cstring(const char* key, int len) {
	// hash upto 32 characters max.
	len = std::min(len, 32);
	u32 hash = 2166136261u;

	for (int i = 0; i < len; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}

using OT = ObjType;

String::String(const char* chrs, std::size_t len) noexcept : Obj(ObjType::string), m_length{len} {
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	m_hash = hash_cstring(buf, m_length);
	m_chars = buf;
}

String::String(const char* chrs, size_t len, size_t hash) noexcept
		: Obj{OT::string}, m_length{len} {
	VYSE_ASSERT(hash == hash_cstring(chrs, len), "Incorrect cstring hash.");
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	m_hash = hash;
	m_chars = buf;
}

String::~String() {
	delete[] m_chars;
}

bool operator==(const String& a, const String& b) {
	if (&a == &b) return true;
	size_t alen = a.len(), blen = b.len();
	if (alen != blen or a.hash() != b.hash()) return false;
	return std::memcmp(a.c_str(), b.c_str(), alen) == 0;
}

void String::trace([[maybe_unused]] GC& gc){};

} // namespace vyse 