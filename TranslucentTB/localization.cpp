#include "localization.hpp"
#include <WinBase.h>
#include <WinUser.h>

#include "log/ttberror.hpp"

std::wstring_view Localization::LoadLocalizedString(uint16_t resource, WORD lang, HINSTANCE hInst)
{
	HRSRC src = FindResourceEx(hInst, RT_STRING, MAKEINTRESOURCE(resource / 16 + 1), lang);
	if (!src)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to fing string resource.");
		return FAILED_LOADING_RESOURCE;
	}

	HGLOBAL res = LoadResource(hInst, src);
	if (!src)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to load string resource.");
		return FAILED_LOADING_RESOURCE;
	}

	const wchar_t *str = reinterpret_cast<const wchar_t *>(LockResource(res));
	if (!str)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to lock string resource.");
		return FAILED_LOADING_RESOURCE;
	}

	for (int i = 0; i < (resource & 15); i++)
	{
		str += 1 + static_cast<uint16_t>(*str);
	}

	return { str + 1, static_cast<uint16_t>(*str) };
}
