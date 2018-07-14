#include "ccolourpicker.hpp"

#include "gui.hpp"

HRESULT CColourPicker::CreateColourPicker()
{
	return GUI::CreateGUI(this, Value, hParent);
}