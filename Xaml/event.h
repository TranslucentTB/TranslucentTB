#pragma once
#include "winrt.hpp"

#define DECL_EVENT_FUNCS(TYPE, NAME, FIELD) \
	winrt::event_token NAME(const TYPE &handler_) \
	{ \
		return FIELD.add(handler_); \
	}\
	\
	void NAME(const winrt::event_token &token_) \
	{ \
		FIELD.remove(token_); \
	}
