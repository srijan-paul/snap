#pragma once
#include <value.hpp>

namespace snap {

/// Strings in snap are heap allocated, and contain 3 important fields:
/// `chars`  -> Pointer to the C string on the heap (null terminated).
/// `length` -> Length of the string.
/// `hash` ->  Unlike other objects, a string's hash is computed by walking over it's characters.
class String : public Obj {
	const char* m_chars = nullptr;

  public:
	const size_t m_length = 0;
	/// @param len length of the string.
	String(const char* chrs, size_t len);
	// creates a string that owns the characters `chrs`.
	/// @param chrs pointer to the character buffer. Must be null terminated.
	String(char* chrs) : Obj(ObjType::string), m_chars{chrs} {};
	/// @brief concatenates [left] and [right] into a string
	String(const String* left, const String* right);

	s32 hash();
	const char* c_str() const;
	static String* concatenate(const String* a, const String* b);
	~String();
};

} // namespace snap
