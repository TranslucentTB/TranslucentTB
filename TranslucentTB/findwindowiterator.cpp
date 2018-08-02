#include "findwindowiterator.hpp"
#include "window.hpp"

const std::wstring FindWindowIterator::uselessEmptyString;
const FindWindowIterator FindWindowIterator::EndIterator;

FindWindowIterator::FindWindowIterator() :
	m_class(uselessEmptyString),
	m_name(uselessEmptyString),
	m_parent(Window::NullWindow),
	m_currentWindow(Window::NullWindow)
{ }

void FindWindowIterator::MoveNext()
{
	m_currentWindow = Window::Find(m_class, m_name, m_parent, m_currentWindow);
}

FindWindowIterator::FindWindowIterator(const std::wstring &className, const std::wstring &windowName, const HWND parent) :
	m_class(className),
	m_name(windowName),
	m_parent(parent),
	m_currentWindow(Window::NullWindow)
{
	MoveNext();
}

Window FindWindowIterator::operator *()
{
	return m_currentWindow;
}