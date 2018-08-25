#include "hooks.hpp"
#include "blacklist.hpp"

EventHook Hooks::m_ChangeHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, Hooks::HandleChangeEvent, WINEVENT_OUTOFCONTEXT);
EventHook Hooks::m_DestroyHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, Hooks::HandleDestroyEvent, WINEVENT_OUTOFCONTEXT);

void Hooks::HandleChangeEvent(const DWORD, const Window &window, ...)
{
	{
		std::lock_guard guard(Blacklist::m_CacheLock);
		Blacklist::m_Cache.erase(window);
	}

	{
		std::lock_guard guard(Window::m_TitlesLock);
		Window::m_Titles.erase(window);
	}
}

void Hooks::HandleDestroyEvent(const DWORD, const Window &window, ...)
{
	{
		std::lock_guard guard(Blacklist::m_CacheLock);
		Blacklist::m_Cache.erase(window);
	}

	{
		std::lock_guard guard(Window::m_TitlesLock);
		Window::m_Titles.erase(window);
	}
	{
		std::lock_guard guard(Window::m_ClassNamesLock);
		Window::m_ClassNames.erase(window);
	}
	{
		std::lock_guard guard(Window::m_FilenamesLock);
		Window::m_Filenames.erase(window);
	}
}
