#pragma once
#include <string>

#include "window.hpp"
#include "findwindowiterator.hpp"

class Window::FindEnum {
 private:
	 std::wstring m_class, m_name;
	 Window m_parent;
 public:
	 inline FindEnum(std::wstring className = L"", std::wstring windowName = L"", Window parent = Window::NullWindow) :
		 m_class(std::move(className)),
		 m_name(std::move(windowName)),
		 m_parent(parent)
	 { }

	 inline FindWindowIterator begin()
	 {
		 return { &m_class, &m_name, m_parent };
	 }

	 inline FindWindowIterator end()
	 {
		 return { nullptr, nullptr, Window::NullWindow };
	 }
};