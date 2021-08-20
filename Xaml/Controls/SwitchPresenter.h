#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/SwitchPresenter.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct SwitchPresenter : SwitchPresenterT<SwitchPresenter>
	{
	private:
		// make DECL_DEPENDENCY_PROPERTY_WITH_METADATA below work
		static void OnValueChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &args);

		// This one manually implemented to not expose the setter
		DECL_DEPENDENCY_PROPERTY_FIELD(Controls::Case, CurrentCase, nullptr);

	public:
		SwitchPresenter();

		static wux::DependencyProperty CurrentCaseProperty() noexcept
		{
			return DEPENDENCY_PROPERTY_FIELD(CurrentCase);
		}

		Controls::Case CurrentCase()
		{
			return GetValue(DEPENDENCY_PROPERTY_FIELD(CurrentCase)).as<Controls::Case>();
		}

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(IInspectable, Value, wux::PropertyMetadata(nullptr, OnValueChanged));

		wfc::IObservableVector<Controls::Case> SwitchCases() noexcept
		{
			return m_Cases;
		}

		void OnApplyTemplate();

		~SwitchPresenter();

	private:
		void EvaluateCases();

		void OnCasesChanged(const wfc::IObservableVector<Controls::Case> &sender, const wfc::IVectorChangedEventArgs &event);
		event_token m_CasesChangedToken;
		wfc::IObservableVector<Controls::Case> m_Cases = single_threaded_observable_vector<Controls::Case>();

		void OnLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args);
		event_token m_LoadedToken;

		static bool CompareValues(const IInspectable &compare, const IInspectable &value);
		static hstring ValueToString(const IInspectable &value);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, SwitchPresenter);
