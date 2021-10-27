#pragma once
#include "token.hpp"
#include "value.hpp"

namespace vy {

// this hash function is from: https://craftinginterpreters.com/hash-tables.html
constexpr u32 hash_cstring(const char* key, uint len) {
	// If this is a long string, then only the first 32 characters are
	// considered for hashing.
	len = std::min(len, 32u);
	u32 hash = 2166136261u;

	for (uint i = 0; i < len; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}
/// @brief Strings in vyse are heap allocated, and contain 3 important fields:
/// @property `m_chars`  -> Pointer to the C string on the heap (null terminated).
/// @property `m_length` -> Length of the string.
/// @property `m_hash`   -> Unlike other objects, a string's hash is computed by walking over it's
///                         characters. This is done to make sure that strings with the same
///                         characters end up with the same hash.
class String final : public Obj {
	friend VM;

	VYSE_NO_DEFAULT_CONSTRUCT(String);
	VYSE_NO_COPY(String);
	VYSE_NO_MOVE(String);

  public:
	/// @param len length of the string.
	explicit String(const char* chrs, size_t len);

	[[nodiscard]] inline constexpr const char* c_str() const noexcept {
		return m_chars;
	}

	[[nodiscard]] char at(int index) const {
		VYSE_ASSERT(index >= 0 and size_t(index) < m_length, "string index out of range.");
		return m_chars[size_t(index)];
	}

	[[nodiscard]] constexpr size_t len() const {
		return m_length;
	}

	[[nodiscard]] constexpr size_t hash() const {
		return m_hash;
	}

	[[nodiscard]] char operator[](s64 index) const {
		return at(index);
	}

	[[nodiscard]] size_t size() const override {
		return m_length * sizeof(char) + sizeof(String);
	}

	~String() {
		VYSE_ASSERT(m_chars != nullptr, "Malformed string object");
		delete[] m_chars;
	}

  private:
	/// @brief creates a string by copying over the characters `chrs` and length `len`. Instead of
	/// computing the hash, this uses a precomputed hash value of `hash`. IMPORTANT: It is the
	/// caller's responsibilty to ensure that `hash` is the correct hash of the this string, having
	/// the same value has `hash_cstring(chrs, len)`.
	explicit String(const char* chrs, size_t len, size_t hash);

	/// @brief creates a string that owns the (heap allocated) characters `chrs`.
	/// @param chrs Null terminated character buffer on the heap.
	/// @param hash The hash for this cstring.
	explicit String(char* chrs, size_t hash) noexcept
		: Obj{ObjType::string}, m_chars{chrs}, m_length{strlen(chrs)}, m_hash{hash} {
		VYSE_ASSERT(hash == hash_cstring(chrs, strlen(chrs)), "Incorrect hash");
	}

	/// @brief Creates a string that owns the characters `chrs`.
	/// @param chrs pointer to the character buffer. Must be null terminated.
	/// @param len length of the buffer. We could calculate this inside the constructor, but the VM
	/// usually has this information at hand when creating strings, so we reuse that.
	/// @param hash The strings hash. Correctness is to be verified by the caller.
	explicit String(char* chrs, size_t len, size_t hash) noexcept
		: Obj(ObjType::string), m_chars{chrs}, m_length{len}, m_hash{hash} {
		VYSE_ASSERT(hash == hash_cstring(chrs, len), "Incorrect hash");
	}

	void trace(GC& gc) override;

	const char* m_chars;
	const size_t m_length;
	/// @brief The string's hash value. This is computed by calling `hash_cstring(cstr, length)`.
	size_t m_hash;
};

bool operator==(const String& a, const String& b);
} // namespace vy
