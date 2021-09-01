#pragma once
#include "../windows/messagewindow.hpp"
#include <optional>
#include <shellapi.h>
#include <windef.h>
#include <wil/resource.h>

#include "undoc/uxtheme.hpp"

class TrayIcon : public virtual MessageWindow {
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

	PFN_SHOULD_SYSTEM_USE_DARK_MODE m_Ssudm;

	void LoadThemedIcon();
	bool Notify(DWORD message, NOTIFYICONDATA *data = nullptr);

protected:
	static constexpr UINT TRAY_CALLBACK = 0xBEEF;

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	inline void ReturnFocus() noexcept
	{
		// Focus may have been automatically returned if the tray icon pops up a context menu.
		// Ignore error in this case.
		Shell_NotifyIcon(NIM_SETFOCUS, &m_IconData);
	}

	std::optional<RECT> GetTrayRect();

public:
	TrayIcon(const GUID &iconId, const wchar_t *whiteIconResource,
		const wchar_t *darkIconResource, PFN_SHOULD_SYSTEM_USE_DARK_MODE ssudm);

	void Show();
	void Hide();

	void SendNotification(uint16_t textResource, DWORD infoFlags = NIIF_INFO);

	virtual ~TrayIcon() noexcept(false) = 0;
};
