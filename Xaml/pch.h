#pragma once
#include "winrt.hpp"

// Remove intrusive macro from Windows headers
#include "undefgetcurrenttime.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Data.h>
#include <winrt/Windows.UI.Xaml.Documents.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.Xaml.Markup.h>
#include <winrt/Windows.UI.Xaml.Navigation.h>
#include <winrt/Microsoft.Toolkit.Win32.UI.XamlHost.h>
#include <winrt/TranslucentTB.Xaml.h>
#include <winrt/TranslucentTB.Xaml.Controls.h>
#include <winrt/TranslucentTB.Xaml.Models.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "redefgetcurrenttime.h"
