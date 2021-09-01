#pragma once
#include <guiddef.h>
#include <Unknwn.h>
#include <winrt/base.h>

// forward declare namespaces we alias
namespace winrt {
	namespace Microsoft::UI::Xaml::Controls {}
	namespace TranslucentTB::Xaml::Models::Primitives {}
	namespace Windows {
		namespace Foundation::Collections {}
		namespace UI::Xaml {
			namespace Controls {}
			namespace Hosting {}
		}
	}
}

// alias some long namespaces for convenience
namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;
namespace txmp = winrt::TranslucentTB::Xaml::Models::Primitives;
namespace wf = winrt::Windows::Foundation;
namespace wfc = wf::Collections;
namespace wux = winrt::Windows::UI::Xaml;
namespace wuxc = wux::Controls;
namespace wuxh = wux::Hosting;
