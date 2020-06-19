#pragma once
#include "../windows/messagewindow.hpp"
#include <optional>
#include <shellapi.h>
#include <windef.h>
#include <wil/resource.h>

class TrayIcon : public MessageWindow {
private:
	NOTIFYICONDATA m_IconData;

	const wchar_t *m_whiteIconResource;
	const wchar_t *m_darkIconResource;
	wil::unique_hicon m_Icon;

	// to know if we should restore the icon when explorer restarts,
	// may have initially failed to show if TTB started before explorer.
	bool m_ShowPreference;
	// to avoid redundant or assured failure calls
	bool m_CurrentlyShowing;

	std::optional<UINT> m_TaskbarCreatedMessage;

	void LoadThemedIcon();
	bool Notify(DWORD message, bool ignoreError = false);

protected:
	static constexpr UINT TRAY_CALLBACK = 0xBEEF;

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	inline void ReturnFocus()
	{
		// Focus may have been automatically returned if the tray icon pops up a context menu.
		// Ignore error in this case.
		Notify(NIM_SETFOCUS, true);
	}

public:
	TrayIcon(const GUID &iconId, Util::null_terminated_wstring_view className,
		Util::null_terminated_wstring_view windowName, const wchar_t *whiteIconResource,
		const wchar_t *darkIconResource, HINSTANCE hInstance);

	void Show();
	void Hide();

	~TrayIcon();

	inline TrayIcon(const TrayIcon &) = delete;
	inline TrayIcon &operator =(const TrayIcon &) = delete;
};
