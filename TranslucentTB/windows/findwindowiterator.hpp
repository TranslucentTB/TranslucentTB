#pragma once
#include <string>

#include "window.hpp"

class FindWindowIterator {
private:
	const std::wstring *m_class;
	const std::wstring *m_name;
	Window m_parent, m_currentWindow;

	inline void MoveNext()
	{
		m_currentWindow = Window::Find(*m_class, *m_name, m_parent, m_currentWindow);
	}

	inline FindWindowIterator(const std::wstring *className, const std::wstring *windowName, Window parent) :
		m_class(className),
		m_name(windowName),
		m_parent(parent),
		m_currentWindow(Window::NullWindow)
	{
		MoveNext();
	}

	friend class Window::FindEnum;

public:
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

	inline Window operator *() const
	{
		return m_currentWindow;
	}
};