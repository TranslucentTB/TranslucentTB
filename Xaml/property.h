#pragma once
#include <utility>
#include <type_traits>

#define PROPERTY_FIELD(NAME) m_ ## NAME ## Property_
#define DECL_PROPERTY_FUNCS(TYPE, NAME, FIELD) \
	TYPE NAME() const noexcept(std::is_nothrow_copy_constructible_v<TYPE>) \
	{ \
		return FIELD; \
	}\
	\
	void NAME(TYPE value) noexcept(std::is_nothrow_move_assignable_v<TYPE>) \
	{ \
		FIELD = std::move(value); \
	}

#define DECL_PROPERTY_PROP(TYPE, NAME) \
private: \
	TYPE PROPERTY_FIELD(NAME); \
	\
public: \
	DECL_PROPERTY_FUNCS(TYPE, NAME, PROPERTY_FIELD(NAME))
