#include <string.hpp>

// this hash function is from: https://craftinginterpreters.com/hash-tables.html
static uint32_t fnv1a(const char* key, int len) {
	uint32_t hash = 2166136261u;

	for (int i = 0; i < len; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}

namespace snap {

String::String(const char* chrs, std::size_t len) : Obj(ObjType::string), m_length{len} {
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	m_chars = buf;
}

String::String(const String* left, const String* right): Obj(ObjType::string), m_length(left->m_length + right->m_length) {
	char* buf = new char[m_length + 1];
	buf[m_length] = '\0';
	std::memcpy(buf, left->m_chars, left->m_length);
	std::memcpy(buf + left->m_length, right->m_chars, right->m_length);

	m_chars = buf;
}

s32 String::hash() {
	m_hash = fnv1a(m_chars, m_length);
	return m_hash;
}

String* String::concatenate(const String* left, const String* right) {
	std::size_t length = left->m_length + right->m_length;

	char* buf = new char[length + 1];
	buf[length] = '\0';
	std::memcpy(buf, left->m_chars, left->m_length);
	std::memcpy(buf + left->m_length, right->m_chars, right->m_length);
	return new String(buf, length);
}

const char* String::c_str() const {
	return m_chars;
}

String::~String() {
	delete[] m_chars;
}

}