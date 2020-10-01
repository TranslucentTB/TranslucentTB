#pragma once
#include <Shobjidl.h>

#include "FramelessPage.h"
#include "Pages/FramelessPageWithMessageDialog.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct FramelessPageWithMessageDialog : FramelessPageWithMessageDialogT<FramelessPageWithMessageDialog, FramelessPage, IInitializeWithWindow>
	{
		FramelessPageWithMessageDialog(const hstring &content);
		FramelessPageWithMessageDialog(const hstring &content, const hstring &title);

		Windows::UI::Popups::MessageDialog Dialog() noexcept;

		IFACEMETHOD(Initialize)(HWND hwnd) noexcept override;

	private:
		Windows::UI::Popups::MessageDialog m_Dialog;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct FramelessPageWithMessageDialog : FramelessPageWithMessageDialogT<FramelessPageWithMessageDialog, implementation::FramelessPageWithMessageDialog>
	{
	};
}
