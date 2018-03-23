#include "windowhelper.hpp"
#include <atlbase.h>
#include <combaseapi.h>
#include <dwmapi.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <ShObjIdl.h>
#include <vector>
#include <winerror.h>
#include <WinUser.h>
#include <wrl/wrappers/corewrappers.h>

#include "ttberror.hpp"

std::wstring WindowHelper::GetWindowTitle(const HWND &hwnd)
{
	int titleSize = GetWindowTextLength(hwnd) + 1; // For the null terminator
	std::vector<wchar_t> windowTitleBuffer(titleSize);
	int result = GetWindowText(hwnd, windowTitleBuffer.data(), titleSize);

	if (!result)
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting title of a window failed.");
		return L"";
	}

	return windowTitleBuffer.data();
}

std::wstring WindowHelper::GetWindowClass(const HWND &hwnd)
{
	wchar_t className[256];
	int result = GetClassName(hwnd, className, 256);

	if (!result)
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting class name of a window failed.");
		return L"";
	}

	return className;
}

std::wstring WindowHelper::GetWindowFile(const HWND &hwnd)
{
	namespace wrap = Microsoft::WRL::Wrappers;

	DWORD ProcessId;
	GetWindowThreadProcessId(hwnd, &ProcessId);

	wrap::HandleT<wrap::HandleTraits::HANDLENullTraits> processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcessId));
	if (!processHandle.IsValid())
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting process handle of a window failed.");
		return L"";
	}

	DWORD path_Size = 33000;
	std::vector<wchar_t> exeName_path(33000);

	if (!QueryFullProcessImageName(processHandle.Get(), 0, exeName_path.data(), &path_Size))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting file name of a window failed.");
		return L"";
	}

	std::wstring exeName(exeName_path.data());
	return exeName.substr(exeName.find_last_of(L"/\\") + 1);
}

bool WindowHelper::IsWindowOnCurrentDesktop(const HWND &hWnd)
{
	static CComPtr<IVirtualDesktopManager> desktop_manager;
	static bool failed = false;

	if (failed)
	{
		return true;
	}
	else if (!desktop_manager)
	{
		failed = !ErrorHandle(desktop_manager.CoCreateInstance(CLSID_VirtualDesktopManager), Error::Level::Log, L"Initialization of IVirtualDesktopManager failed.");
		if (failed)
		{
			return true;
		}
	}

	BOOL on_current_desktop;  // This must be a BOOL not a bool because Windows and C89 are equally stupid
	HRESULT result = desktop_manager->IsWindowOnCurrentVirtualDesktop(hWnd, &on_current_desktop);
	if (FAILED(result))
	{
		ErrorHandle(result, Error::Level::Log, L"Verifying if a window is on the current virtual desktop failed.");
		return true;
	}

	return on_current_desktop;
}

bool WindowHelper::IsWindowMaximised(const HWND &hWnd)
{
	WINDOWPLACEMENT result;
	BOOL status = GetWindowPlacement(hWnd, &result);
	if (!status)
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting placement of a window failed.");
		return false;
	}
	return result.showCmd == SW_MAXIMIZE;
}

bool WindowHelper::IsWindowCloaked(const HWND &hWnd)
{
	BOOL cloaked;
	HRESULT status = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));

	if (FAILED(status))
	{
		ErrorHandle(status, Error::Level::Log, L"Getting cloaked status of a window failed.");
		return false;
	}

	return cloaked;
}