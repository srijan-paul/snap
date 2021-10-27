#include <algorithm>
#include <cassert>
#include <cstring>
#include <string.hpp>

namespace vyse {

using OT = ObjType;

String::String(const char* chrs, std::size_t len) : Obj(ObjType::string), m_length{len} {
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	m_hash = hash_cstring(buf, m_length);
	m_chars = buf;
}

String::String(const char* chrs, size_t len, size_t hash) : Obj{OT::string}, m_length{len} {
	VYSE_ASSERT(hash == hash_cstring(chrs, len), "Incorrect cstring hash.");
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	m_hash = hash;
	m_chars = buf;
}

bool operator==(const String& a, const String& b) {
	if (&a == &b) return true;
	size_t alen = a.len(), blen = b.len();
	if (alen != blen or a.hash() != b.hash()) return false;
	return std::memcmp(a.c_str(), b.c_str(), alen) == 0;
}

void String::trace([[maybe_unused]] GC& gc) {}

} // namespace vyse