#include "findwindowiterator.hpp"
#include "window.hpp"

const FindWindowIterator FindWindowIterator::EndIterator;

FindWindowIterator::FindWindowIterator() :
	m_class(nullptr),
	m_name(nullptr),
	m_parent(Window::NullWindow),
	m_currentWindow(Window::NullWindow)
{ }

void FindWindowIterator::MoveNext()
{
	m_currentWindow = Window::Find(*m_class, *m_name, m_parent, m_currentWindow);
}

FindWindowIterator::FindWindowIterator(const std::wstring *const className, const std::wstring *const windowName, const HWND parent) :
	m_class(className),
	m_name(windowName),
	m_parent(parent),
	m_currentWindow(Window::NullWindow)
{
	MoveNext();
}

Window FindWindowIterator::operator *() const
{
	return m_currentWindow;
}