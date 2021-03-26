#pragma once
#include <tuple>

static constexpr wchar_t numbers[] = {
	L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9'
};

static constexpr std::tuple<wchar_t, wchar_t> alphabet[] = {
	{ L'A', L'a'}, { L'B', L'b'}, { L'C', L'c'}, { L'D', L'd'},
	{ L'E', L'e'}, { L'F', L'f'}, { L'G', L'g'}, { L'H', L'h'},
	{ L'I', L'i'}, { L'J', L'j'}, { L'K', L'k'}, { L'L', L'l'},
	{ L'M', L'm'}, { L'N', L'n'}, { L'O', L'o'}, { L'P', L'p'},
	{ L'Q', L'q'}, { L'R', L'r'}, { L'S', L's'}, { L'T', L't'},
	{ L'U', L'u'}, { L'V', L'v'}, { L'W', L'w'}, { L'X', L'x'},
	{ L'Y', L'y'}, { L'Z', L'z'}
};

static constexpr wchar_t specialCharacters[] = {
	L' ', L'!', L'"', L'#', L'$', L'%', L'&', L'\'', L'(',
	L')', L'*', L'+', L',', L'-', L'.', L'/', L':', L';',
	L'<', L'=', L'>', L'?', L'@', L'[', L'\\', L']', L'^',
	L'_', L'`', L'{', L'|', L'}', L'~'
};
