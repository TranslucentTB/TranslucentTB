#pragma once
#include "arch.h"
#include <string>
#include <windef.h>

class Window; // Forward declare to prevent circular deps

class FindWindowIterator {
private:
	const std::wstring *const m_class;
	const std::wstring *const m_name;
	const HWND m_parent;

	HWND m_currentWindow;

	FindWindowIterator();
	void MoveNext();

public:
	static const FindWindowIterator EndIterator;

	FindWindowIterator(const std::wstring *const className, const std::wstring *const windowName, const HWND parent);

	inline FindWindowIterator &operator ++()
	{
		MoveNext();
		return *this;
	}

	inline bool operator ==(const FindWindowIterator &right) const
	{
		return m_currentWindow == right.m_currentWindow;
	}

	inline bool operator !=(const FindWindowIterator &right) const
	{
		return !operator==(right);
	}

	Window operator *() const;
};