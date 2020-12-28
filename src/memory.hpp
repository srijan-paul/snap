#pragma once
#include <common.hpp>
#include <stdlib.h>

template <typename T>
void* allocate(T* ptr, size_t new_size) {
	if (new_size == 0) {
		delete ptr;
		return nullptr;
	}
	return realloc(ptr, new_size);
}