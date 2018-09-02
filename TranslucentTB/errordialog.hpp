#pragma once
#include "taskdialog.hpp"
#include "ttberror.hpp"

class ErrorDialog : public TTBTaskDialog {
public:
	inline ErrorDialog(const std::wstring &message, HRESULT error, bool isFatal) :
		TTBTaskDialog(message, Error::ExceptionFromHRESULT(error), [](...) { }, isFatal ? TD_ERROR_ICON : TD_WARNING_ICON)
	{ }

	inline HRESULT Run()
	{
		int a, b, c;
		return TTBTaskDialog::Run(a, b, c);
	}
};