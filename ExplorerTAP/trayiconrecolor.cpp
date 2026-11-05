#include "trayiconrecolor.hpp"
#include <algorithm>
#include <thread>
#include <vector>
#include <windows.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <wil/resource.h>

#include "constants.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")

namespace
{
	// what Shell_NotifyIcon sends to the taskbar through WM_COPYDATA.
	// only the leading fields matter here, and those are the same in every NOTIFYICONDATA version.
	struct TRAYNOTIFYDATAHEADER
	{
		DWORD dwSignature;
		DWORD dwMessage;
		DWORD cbSize;
		DWORD hWnd;
		UINT uID;
		UINT uFlags;
		UINT uCallbackMessage;
		DWORD hIcon;
	};

	constexpr ULONG_PTR TRAY_COPYDATA_NOTIFYICON = 1;
	constexpr DWORD TRAY_NOTIFYDATA_SIGNATURE = 0x34753423;
	constexpr UINT_PTR SUBCLASS_ID = 0x54544243; // TTBC

	constexpr std::chrono::seconds REFRESH_WINDOW { 10 };
	constexpr std::chrono::seconds OLD_ICON_LIFETIME { 30 };

	// pixels more transparent than this are ignored when deciding if an icon is monochrome.
	constexpr uint8_t VISIBLE_ALPHA = 64;
	// maximum difference between channels for a pixel to count as gray.
	constexpr int GRAY_TOLERANCE = 48;

	constexpr int Luminance(int r, int g, int b) noexcept
	{
		return (r * 2126 + g * 7152 + b * 722) / 10000;
	}
}

bool TrayIconRecolor::SetColor(uint32_t color, DWORD skipProcess)
{
	if (color >> 24 == 0)
	{
		color = 0;
	}

	if (color == m_Color)
	{
		return true;
	}

	if (!m_Tray)
	{
		if (color == 0)
		{
			return true; // nothing was ever recolored
		}

		const HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
		DWORD pid = 0;
		if (!tray || GetWindowThreadProcessId(tray, &pid) != GetCurrentThreadId() || pid != GetCurrentProcessId())
		{
			return false; // subclassing only works from the thread owning the window
		}

		if (!SetWindowSubclass(tray, SubclassProc, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)))
		{
			return false;
		}

		m_Tray = tray;
	}

	m_Color = color;
	RequestIconRefresh(skipProcess);
	return true;
}

TrayIconRecolor::~TrayIconRecolor()
{
	if (m_Tray)
	{
		RemoveWindowSubclass(m_Tray, SubclassProc, SUBCLASS_ID);
		m_Tray = nullptr;
	}

	ReleaseOldIcons(true);
}

LRESULT TrayIconRecolor::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData) noexcept
{
	const auto self = reinterpret_cast<TrayIconRecolor *>(dwRefData);
	if (uMsg == WM_COPYDATA)
	{
		return self->OnCopyData(hWnd, uMsg, wParam, lParam);
	}
	else if (uMsg == WM_NCDESTROY)
	{
		RemoveWindowSubclass(hWnd, SubclassProc, SUBCLASS_ID);
		self->m_Tray = nullptr;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT TrayIconRecolor::OnCopyData(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	const auto cds = reinterpret_cast<COPYDATASTRUCT *>(lParam);
	if (!cds || cds->dwData != TRAY_COPYDATA_NOTIFYICON || cds->cbData < sizeof(TRAYNOTIFYDATAHEADER) || !cds->lpData)
	{
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	const auto data = static_cast<TRAYNOTIFYDATAHEADER *>(cds->lpData);
	if (data->dwSignature != TRAY_NOTIFYDATA_SIGNATURE || (data->dwMessage != NIM_ADD && data->dwMessage != NIM_MODIFY))
	{
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	HICON recolored = nullptr;
	if (m_Color != 0 && (data->uFlags & NIF_ICON) && data->hIcon)
	{
		recolored = Recolor(static_cast<HICON>(LongToHandle(static_cast<LONG>(data->hIcon))));
		if (recolored)
		{
			data->hIcon = HandleToLong(recolored);
		}
	}

	LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

	// when we asked apps to send their icons again, most of them will try adding
	// an icon that already exists, which fails. update the existing one instead.
	if (!result && data->dwMessage == NIM_ADD && std::chrono::steady_clock::now() < m_RefreshDeadline)
	{
		data->dwMessage = NIM_MODIFY;
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	if (recolored)
	{
		m_OldIcons.emplace_back(std::chrono::steady_clock::now(), recolored);
	}

	ReleaseOldIcons(false);
	return result;
}

HICON TrayIconRecolor::Recolor(HICON icon) const noexcept
{
	ICONINFO info {};
	if (!GetIconInfo(icon, &info))
	{
		return nullptr;
	}

	wil::unique_hbitmap color(info.hbmColor);
	wil::unique_hbitmap mask(info.hbmMask);
	if (!color || !mask)
	{
		return nullptr; // monochrome cursor-style icon, leave it alone
	}

	BITMAP bm {};
	if (!GetObjectW(color.get(), sizeof(bm), &bm) || bm.bmWidth <= 0 || bm.bmHeight <= 0 || bm.bmWidth > 256 || bm.bmHeight > 256)
	{
		return nullptr;
	}

	const int width = bm.bmWidth, height = bm.bmHeight;

	BITMAPINFO bi {};
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height; // top-down
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	const wil::unique_hdc dc(CreateCompatibleDC(nullptr));
	if (!dc)
	{
		return nullptr;
	}

	std::vector<uint32_t> pixels(static_cast<std::size_t>(width) * height);
	if (!GetDIBits(dc.get(), color.get(), 0, height, pixels.data(), &bi, DIB_RGB_COLORS))
	{
		return nullptr;
	}

	// old style icons have no alpha channel, derive it from the mask.
	if (std::ranges::all_of(pixels, [](uint32_t px) { return (px >> 24) == 0; }))
	{
		std::vector<uint32_t> maskPixels(pixels.size());
		if (!GetDIBits(dc.get(), mask.get(), 0, height, maskPixels.data(), &bi, DIB_RGB_COLORS))
		{
			return nullptr;
		}

		for (std::size_t i = 0; i < pixels.size(); ++i)
		{
			const uint32_t alpha = (maskPixels[i] & 0x00FFFFFF) ? 0 : 0xFF;
			pixels[i] = (pixels[i] & 0x00FFFFFF) | (alpha << 24);
		}
	}

	const int targetR = m_Color & 0xFF, targetG = (m_Color >> 8) & 0xFF, targetB = (m_Color >> 16) & 0xFF;
	const bool lightTarget = Luminance(targetR, targetG, targetB) > 127;

	// only touch icons that are (nearly) monochrome and have the opposite tone to the target,
	// colorful icons and icons that already contrast fine are left as is.
	std::size_t visible = 0, gray = 0;
	uint64_t luminanceSum = 0;
	for (const uint32_t px : pixels)
	{
		const int a = px >> 24, r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
		if (a >= VISIBLE_ALPHA)
		{
			++visible;
			if (std::max({ r, g, b }) - std::min({ r, g, b }) <= GRAY_TOLERANCE)
			{
				++gray;
				luminanceSum += Luminance(r, g, b);
			}
		}
	}

	if (visible == 0 || gray * 10 < visible * 9)
	{
		return nullptr;
	}

	const auto averageLuminance = luminanceSum / gray;
	if (lightTarget ? averageLuminance >= 128 : averageLuminance <= 127)
	{
		return nullptr;
	}

	for (uint32_t &px : pixels)
	{
		const int a = px >> 24, r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
		if (a == 0 || std::max({ r, g, b }) - std::min({ r, g, b }) > GRAY_TOLERANCE)
		{
			continue;
		}

		// the strongest parts of the glyph get the target color, details in the
		// opposite tone get the opposite treatment so they stay visible.
		const int luminance = Luminance(r, g, b);
		const int weight = (std::min)(255, 2 * (lightTarget ? 255 - luminance : luminance));
		px = (static_cast<uint32_t>(a) << 24) |
			(static_cast<uint32_t>(targetR * weight / 255) << 16) |
			(static_cast<uint32_t>(targetG * weight / 255) << 8) |
			static_cast<uint32_t>(targetB * weight / 255);
	}

	void *bits = nullptr;
	const wil::unique_hbitmap newColor(CreateDIBSection(dc.get(), &bi, DIB_RGB_COLORS, &bits, nullptr, 0));
	if (!newColor || !bits)
	{
		return nullptr;
	}

	std::ranges::copy(pixels, static_cast<uint32_t *>(bits));
	GdiFlush();

	ICONINFO newInfo {};
	newInfo.fIcon = TRUE;
	newInfo.hbmMask = mask.get();
	newInfo.hbmColor = newColor.get();
	return CreateIconIndirect(&newInfo);
}

void TrayIconRecolor::ReleaseOldIcons(bool all) noexcept
{
	const auto now = std::chrono::steady_clock::now();
	while (!m_OldIcons.empty() && (all || m_OldIcons.size() > 256 || now - m_OldIcons.front().first > OLD_ICON_LIFETIME))
	{
		DestroyIcon(m_OldIcons.front().second);
		m_OldIcons.pop_front();
	}
}

void TrayIconRecolor::RequestIconRefresh(DWORD skipProcess) noexcept
{
	m_RefreshDeadline = std::chrono::steady_clock::now() + REFRESH_WINDOW;

	// the same message the shell sends when explorer restarts, it makes apps send their icons again.
	// not broadcasted, because TranslucentTB fully resets itself when it gets it.
	struct EnumData
	{
		UINT message;
		DWORD skipProcess;
	} enumData { RegisterWindowMessageW(WM_TASKBARCREATED.c_str()), skipProcess };

	EnumWindows([](HWND hwnd, LPARAM lParam) noexcept -> BOOL
	{
		const auto &data = *reinterpret_cast<const EnumData *>(lParam);

		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid != data.skipProcess && pid != GetCurrentProcessId())
		{
			PostMessageW(hwnd, data.message, 0, 0);
		}

		return TRUE;
	}, reinterpret_cast<LPARAM>(&enumData));

	// theme aware apps often only check that their icon still exists on TaskbarCreated,
	// but they do send a new one when they're told the theme changed. the theme itself stays the same.
	// done on another thread because SendMessageTimeout blocks until every window answered.
	// explorer itself is skipped: it would reset the taskbar theme we just changed.
	std::thread([]
	{
		EnumWindows([](HWND hwnd, LPARAM) noexcept -> BOOL
		{
			DWORD pid = 0;
			GetWindowThreadProcessId(hwnd, &pid);
			if (pid != GetCurrentProcessId())
			{
				DWORD_PTR result = 0;
				SendMessageTimeoutW(hwnd, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"ImmersiveColorSet"), SMTO_ABORTIFHUNG, 200, &result);
			}

			return TRUE;
		}, 0);
	}).detach();
}
