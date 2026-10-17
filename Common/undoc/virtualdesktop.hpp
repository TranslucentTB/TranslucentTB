#pragma once
#include <windows.h>
#include <objectarray.h>
#include <servprov.h>
#include <winstring.h>

// Undocumented Windows 11 24H2/25H2 shell ABI. Keep this separate from the
// documented IVirtualDesktopManager used for window membership checks.
// Interface GUIDs and vtable layouts:
// https://github.com/Ciantic/VirtualDesktopAccessor/blob/7ff9ef827bab9a081421ebb204339dd96475ec1a/src/interfaces.rs
namespace VirtualDesktopAbi {

// Later builds must be checked before using this undocumented vtable layout.
inline constexpr bool IsSupportedBuild(DWORD build, DWORD revision) noexcept
{
	return build == 26200 || (build == 26100 && revision >= 2605);
}

struct __declspec(uuid("3F07F4BE-B107-441A-AF0F-39D82529072C")) Desktop : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE IsViewVisible(IUnknown *view, BOOL *visible) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetId(GUID *id) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetName(HSTRING *name) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWallpaper(HSTRING *wallpaper) = 0;
};

// Only the prefix that we use is declared; never call subsequent vtable slots.
struct __declspec(uuid("53F5CA0B-158F-4124-900C-057158060B27")) Manager : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE GetCount(UINT *count) = 0;
	virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(IUnknown *view, Desktop *desktop) = 0;
	virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(IUnknown *view, BOOL *canMove) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(Desktop **desktop) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDesktops(IObjectArray **desktops) = 0;
};

struct __declspec(uuid("B9E5E94D-233E-49AB-AF5C-2B4541C3AADE")) Notification : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE DesktopCreated(Desktop *desktop) = 0;
	virtual HRESULT STDMETHODCALLTYPE DesktopDestroyBegin(Desktop *desktop, Desktop *fallback) = 0;
	virtual HRESULT STDMETHODCALLTYPE DesktopDestroyFailed(Desktop *desktop, Desktop *fallback) = 0;
	virtual HRESULT STDMETHODCALLTYPE DesktopDestroyed(Desktop *desktop, Desktop *fallback) = 0;
	virtual HRESULT STDMETHODCALLTYPE DesktopMoved(Desktop *desktop, INT64 oldIndex, INT64 newIndex) = 0;
	virtual HRESULT STDMETHODCALLTYPE DesktopNameChanged(Desktop *desktop, HSTRING name) = 0;
	virtual HRESULT STDMETHODCALLTYPE ViewDesktopChanged(IUnknown *view) = 0;
	virtual HRESULT STDMETHODCALLTYPE CurrentDesktopChanged(Desktop *oldDesktop, Desktop *newDesktop) = 0;
	virtual HRESULT STDMETHODCALLTYPE DesktopWallpaperChanged(Desktop *desktop, HSTRING wallpaper) = 0;
	virtual HRESULT STDMETHODCALLTYPE DesktopSwitched(Desktop *desktop) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoteDesktopConnected(Desktop *desktop) = 0;
};

struct __declspec(uuid("0CD45E71-D927-4F15-8B0A-8FEF525337BF")) NotificationService : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Register(Notification *notification, DWORD *cookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD cookie) = 0;
};

inline constexpr GUID ImmersiveShell = { 0xC2F03A33, 0x21F5, 0x47FA, { 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 } };
inline constexpr GUID ManagerService = { 0xC5E0CDCA, 0x7B6E, 0x41B2, { 0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B } };
inline constexpr GUID NotificationsService = { 0xA501FDEC, 0x4A09, 0x464C, { 0xAE, 0x4E, 0x1B, 0x9C, 0x21, 0xB8, 0x49, 0x18 } };

}
