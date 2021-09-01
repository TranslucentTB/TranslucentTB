#pragma once
#include "../arch.h"
#include <guiddef.h>
#include <rpcndr.h>
#include <Unknwn.h>
#include <windef.h>
#include <winerror.h>

static constexpr CLSID CLSID_ImmersiveShell = { 0xC2F03A33, 0x21F5, 0x47FA, { 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 } };
static constexpr GUID SID_MultitaskingViewVisibilityService = { 0x785702DD, 0xB8EF, 0x469F, { 0x8C, 0x19, 0xE9, 0x1B, 0x5F, 0x4C, 0xA5, 0x64 } };
static constexpr wchar_t SEH_Cortana[] = L"Windows.Internal.ShellExperience.Cortana";
static constexpr wchar_t SEH_SearchApp[] = L"Windows.Internal.ShellExperience.SearchApp";

enum MULTITASKING_VIEW_TYPES : INT {
	MVT_NONE = 0x0,
	MVT_ALT_TAB_VIEW = 0x1,
	MVT_ALL_UP_VIEW = 0x2,
	MVT_SNAP_ASSIST_VIEW = 0x4,
	MVT_PPI_ALL_UP_VIEW = 0x8, // this is used by Windows 10 Team, so not of interest to us
	MVT_ANY = 0xF
};

MIDL_INTERFACE("c59a7a3c-0676-4526-8192-5d0bf9b89b95")
IMultitaskingViewVisibilityNotification : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE MultitaskingViewShown(MULTITASKING_VIEW_TYPES flags) = 0;
	virtual HRESULT STDMETHODCALLTYPE MultitaskingViewDismissed(MULTITASKING_VIEW_TYPES flags) = 0;
};

MIDL_INTERFACE("ac11cda3-1601-4ad7-a40e-fe2ced187307")
IMultitaskingViewVisibilityService : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE IsViewVisible(MULTITASKING_VIEW_TYPES flags, MULTITASKING_VIEW_TYPES *retFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE Register(IMultitaskingViewVisibilityNotification *pVisibilityNotification, DWORD *pdwCookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD dwCookie) = 0;
};
