#pragma once
#include "token.hpp"
#include "value.hpp"

namespace snap {

u32 hash_cstring(const char* chars, int len);
/// Strings in snap are heap allocated, and contain 3 important fields:
/// `m_chars`  -> Pointer to the C string on the heap (null terminated).
/// `m_length` -> Length of the string.
/// `m_hash`   -> Unlike other objects, a string's hash is computed by walking over it's
/// 						  characters. This is done to make sure that strings with the same
///               characters end up with the same hash.
class String final : public Obj {
	friend VM;

	SNAP_NO_DEFAULT_CONSTRUCT(String);

  public:
	/// @param len length of the string.
	String(const char* chrs, size_t len);

	const char* c_str() const;
	char at(number index) const;
	size_t len() const;
	size_t hash() const;
	char operator[](size_t index) const;
	static String* concatenate(const String* a, const String* b);
	~String();

  private:
	/// @brief creates a string by copying over the characters
	/// `chrs` and length `len`. Instead of computing the hash,
	/// this uses a precomputed hash value of `hash`.
	/// IMPORTANT: It is the caller's responsibilty to ensure
	/// that `hash` is the correct hash of the this string,
	/// having the same value has `hash_cstring(chrs, len)`.
	String(const char* chrs, size_t len, size_t hash);

	/// @brief creates a string that owns the (heap allocated) characters `chrs`.
	/// @param chrs Null terminated character buffer on the heap.
	/// @param hash The hash for this cstring.
	String(char* chrs, size_t hash)
		: Obj{ObjType::string}, m_chars{chrs}, m_length{strlen(chrs)}, m_hash{hash} {};

	/// @brief Creates a string that owns the characters `chrs`.
	/// @param chrs pointer to the character buffer. Must be null terminated.
	String(char* chrs)
		: Obj(ObjType::string), m_chars{chrs}, m_length{strlen(chrs)}, m_hash{hash_cstring(
																		   chrs, m_length)} {};

	const char* m_chars = nullptr;
	const size_t m_length;
	/// @brief The string's hash value. This is computed by calling
	/// `hash_cstring(cstr, length)`.
	size_t m_hash;
};

bool operator==(const String& a, const String& b);
} // namespace snap
