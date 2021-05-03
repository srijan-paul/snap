#pragma once
#include "token.hpp"
#include "value.hpp"

namespace vyse {

u32 hash_cstring(const char* chars, int len);
/// Strings in vyse are heap allocated, and contain 3 important fields:
/// `m_chars`  -> Pointer to the C string on the heap (null terminated).
/// `m_length` -> Length of the string.
/// `m_hash`   -> Unlike other objects, a string's hash is computed by walking over it's
/// 						  characters. This is done to make sure that strings with the same
///               characters end up with the same hash.
class String final : public Obj {
	friend VM;

	VYSE_NO_DEFAULT_CONSTRUCT(String);

public:
	/// @param len length of the string.
	explicit String(const char* chrs, size_t len) noexcept;

	inline constexpr const char* c_str() const {
		return m_chars;
	}

	char at(number index) const;

	constexpr size_t len() const {
		return m_length;
	}

	constexpr size_t hash() const {
		return m_hash;
	}

	char operator[](size_t index) const {
		return at(index);
	}

	size_t size() const override {
		return m_length * sizeof(char) + sizeof(String);
	}

	~String();

private:
	/// @brief creates a string by copying over the characters
	/// `chrs` and length `len`. Instead of computing the hash,
	/// this uses a precomputed hash value of `hash`.
	/// IMPORTANT: It is the caller's responsibilty to ensure
	/// that `hash` is the correct hash of the this string,
	/// having the same value has `hash_cstring(chrs, len)`.
	explicit String(const char* chrs, size_t len, size_t hash) noexcept;

	/// @brief creates a string that owns the (heap allocated) characters `chrs`.
	/// @param chrs Null terminated character buffer on the heap.
	/// @param hash The hash for this cstring.
	explicit String(char* chrs, size_t hash) noexcept
			: Obj{ObjType::string}, m_chars{chrs}, m_length{strlen(chrs)}, m_hash{hash} {};

	/// @brief Creates a string that owns the characters `chrs`.
	/// @param chrs pointer to the character buffer. Must be null terminated.
	explicit String(char* chrs) noexcept
			: Obj(ObjType::string), m_chars{chrs}, m_length{strlen(chrs)}, m_hash{hash_cstring(
																																				 chrs, m_length)} {};

	void trace(GC& gc) override;

	const char* m_chars;
	const size_t m_length;
	/// @brief The string's hash value. This is computed by calling
	/// `hash_cstring(cstr, length)`.
	size_t m_hash;
};

bool operator==(const String& a, const String& b);
} // namespace vyse 
