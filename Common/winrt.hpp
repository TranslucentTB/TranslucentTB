#pragma once
#ifndef WINRT_BASE_H
# include <Unknwn.h>
# include <winrt/base.h>
#else
# ifndef __IUnknown_INTERFACE_DEFINED__
#  error "<winrt/base.h> has been previously included without COM support"
# endif
#endif

namespace winrt::Windows::Foundation {}
namespace winrt::Windows::UI::Xaml {}
namespace winrt::Windows::UI::Xaml::Controls {}
namespace winrt::Windows::UI::Xaml::Hosting {}

namespace wf = winrt::Windows::Foundation;
namespace wux = winrt::Windows::UI::Xaml;
namespace wuxc = wux::Controls;
namespace wuxh = wux::Hosting;
