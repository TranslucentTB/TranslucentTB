#pragma once
#include <atomic>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Composition.h>

class XamlCompositionBrush : public wux::Media::XamlCompositionBrushBaseT<XamlCompositionBrush>
{
public:
	XamlCompositionBrush(wuc::CompositionBrush brush, bool disposeBrush = true);

	void OnConnected();
	void OnDisconnected();
private:
	wuc::CompositionBrush m_brush;

	bool m_disposeBrush;
	static std::atomic_int m_connectionCounter;

	void OnConnectionChanged();
};
