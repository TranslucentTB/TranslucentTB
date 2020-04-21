#pragma once
#include "arch.h"
#include <ShObjIdl.h>
#include "winrt.hpp"

#include "wilx.hpp"

class AppVisibilitySink : public winrt::implements<AppVisibilitySink, IAppVisibilityEvents> {
public:
	using StartOpenedEventHandler = winrt::delegate<bool>;

private:
	winrt::event<StartOpenedEventHandler> m_startOpened;

	IFACEMETHODIMP LauncherVisibilityChange(BOOL currentVisibleState) override try
	{
		m_startOpened(currentVisibleState);
		return S_OK;
	}
	catch (...)
	{
		return winrt::to_hresult();
	}

	IFACEMETHODIMP AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY) override
	{
		return S_OK;
	}

	void STDMETHODCALLTYPE remove_StartOpened(int64_t token)
	{
		m_startOpened.remove({ token });
	}

public:
	using StartOpened_revoker = wilx::unique_com_token<&AppVisibilitySink::remove_StartOpened>;

	winrt::event_token StartOpened(const StartOpenedEventHandler &value)
	{
		return m_startOpened.add(value);
	}

	StartOpened_revoker StartOpened(winrt::auto_revoke_t, const StartOpenedEventHandler &value)
	{
		StartOpened_revoker revoker;
		revoker.associate(this);
		revoker.reset(StartOpened(value).value);

		return revoker;
	}

	void StartOpened(const winrt::event_token &token)
	{
		remove_StartOpened(token.value);
	}
};
