#include <algorithm>
#include <cassert>
#include <cstring>
#include <string.hpp>

namespace snap {

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

String::String(const char* chrs, std::size_t len) : Obj(ObjType::string), m_length{len} {
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	m_hash = hash_cstring(buf, m_length);
	m_chars = buf;
}

String::String(const char* chrs, size_t len, size_t hash) : Obj{OT::string}, m_length{len} {
	assert(hash == hash_cstring(chrs, len));

	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	m_hash = hash;
	m_chars = buf;
}

String::~String() {
	delete[] m_chars;
}

String* String::concatenate(const String* left, const String* right) {
	std::size_t length = left->m_length + right->m_length;

	char* buf = new char[length + 1];
	buf[length] = '\0';
	std::memcpy(buf, left->m_chars, left->m_length);
	std::memcpy(buf + left->m_length, right->m_chars, right->m_length);
	return new String(buf, length);
}

bool operator==(const String& a, const String& b) {
	if (&a == &b) return true;
	size_t alen = a.len(), blen = b.len();
	if (alen != blen or alen != blen) return false;
	return std::memcmp(a.c_str(), b.c_str(), alen) == 0;
}

const char* String::c_str() const {
	return m_chars;
}

char String::at(number index) const {
	assert(index > 0 and index < m_length);
	return m_chars[std::size_t(index)];
}

char String::operator[](size_t index) const {
	return at(index);
}

size_t String::hash() const {
	return m_hash;
}

size_t String::len() const {
	return m_length;
}

void String::trace(GC& gc) {
}

} // namespace snap