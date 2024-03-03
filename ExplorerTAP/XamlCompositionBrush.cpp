#include "XamlCompositionBrush.h"

std::atomic_int XamlCompositionBrush::m_connectionCounter = 0;

XamlCompositionBrush::XamlCompositionBrush(wuc::CompositionBrush brush, bool disposeBrush) : m_brush(brush), m_disposeBrush(disposeBrush) { }

void XamlCompositionBrush::OnConnected()
{
	CompositionBrush(m_brush);

	if (m_disposeBrush)
	{
		++m_connectionCounter;
		OnConnectionChanged();
	}
}

void XamlCompositionBrush::OnDisconnected()
{
	if (m_disposeBrush)
	{
		--m_connectionCounter;
		OnConnectionChanged();
	}
}

void XamlCompositionBrush::OnConnectionChanged()
{
	if (!m_connectionCounter)
	{
		CompositionBrush(nullptr);
		m_brush.Close();
	}
}
