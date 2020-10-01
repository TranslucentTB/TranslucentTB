#include "pch.h"

#include "FramelessPageWithMessageDialog.h"
#if __has_include("Pages/FramelessPageWithMessageDialog.g.cpp")
#include "Pages/FramelessPageWithMessageDialog.g.cpp"
#endif

using namespace winrt;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	FramelessPageWithMessageDialog::FramelessPageWithMessageDialog(const hstring &content) :
		m_Dialog(content)
	{ }

	FramelessPageWithMessageDialog::FramelessPageWithMessageDialog(const hstring &content, const hstring &title) :
		m_Dialog(content, title)
	{ }

	Windows::UI::Popups::MessageDialog FramelessPageWithMessageDialog::Dialog() noexcept
	{
		return m_Dialog;
	}

	IFACEMETHODIMP FramelessPageWithMessageDialog::Initialize(HWND hwnd) noexcept
	{
		com_ptr<IInitializeWithWindow> initWithWindow;
		HRESULT hr = winrt::get_unknown(m_Dialog)->QueryInterface(initWithWindow.put());
		if (SUCCEEDED(hr))
		{
			hr = initWithWindow->Initialize(hwnd);
		}

		return hr;
	}
}
