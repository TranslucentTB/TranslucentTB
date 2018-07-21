#pragma once
#include "arch.h"
#include <string>
#include <windef.h>

class Window; // Forward declare to prevent circular deps

class FindWindowIterator {
private:
	static const std::wstring uselessEmptyString;
	const std::wstring &m_class;
	const std::wstring &m_name;
	const HWND m_parent;

	HWND m_currentWindow;

	FindWindowIterator();
	void MoveNext();

public:
	static const FindWindowIterator EndIterator;

	FindWindowIterator(const std::wstring &className, const std::wstring &windowName, const HWND parent);

	FindWindowIterator &operator ++();

	bool operator ==(const FindWindowIterator &right) const;
	inline bool operator !=(const FindWindowIterator &right) const
	{
		return !operator==(right);
	}

	Window operator *();
};