#pragma once
#include "rendercontext.hpp"

class PickerCircleContext : public RenderContext {
public:
	using RenderContext::RenderContext;

	HRESULT Draw(HWND hDlg, const SColourF &col, const SColourF &) override;
};