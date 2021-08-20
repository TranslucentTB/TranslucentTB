#include "pch.h"

#include "Controls/SwitchPresenter.h"
#if __has_include("Controls/SwitchPresenter.g.cpp")
#include "Controls/SwitchPresenter.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	void SwitchPresenter::OnValueChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &)
	{
		if (const auto that = sender.try_as<SwitchPresenter>())
		{
			that->EvaluateCases();
		}
	}

	SwitchPresenter::SwitchPresenter()
	{
		m_LoadedToken = Loaded({ get_weak(), &SwitchPresenter::OnLoaded });
		m_CasesChangedToken = m_Cases.VectorChanged({ get_weak(), &SwitchPresenter::OnCasesChanged });
	}

	void SwitchPresenter::OnApplyTemplate()
	{
		base_type::OnApplyTemplate();

		EvaluateCases();
	}

	SwitchPresenter::~SwitchPresenter()
	{
		m_Cases.VectorChanged(m_CasesChangedToken);
		Loaded(m_LoadedToken);
	}

	void SwitchPresenter::EvaluateCases()
	{
		const auto current = CurrentCase();
		if (m_Cases.Size() == 0)
		{
			// If we have no cases, then we can't match anything.
			if (current)
			{
				// Only bother clearing our actual content if we had something before.
				Content(nullptr);
				SetValue(CurrentCaseProperty(), nullptr);
			}

			return;
		}
		else if (current)
		{
			const auto currentValue = current.Value();
			if (currentValue && currentValue == Value())
			{
				// If the current case we're on already matches our current value,
				// then we don't have any work to do.
				return;
			}
		}

		Case xdefault(nullptr);
		Case newcase(nullptr);

		const auto value = Value();
		for (const auto &xcase : m_Cases)
		{
			if (xcase.IsDefault())
			{
				// If there are multiple default cases provided, this will override and just grab the last one, the developer will have to fix this in their XAML. We call this out in the case comments.
				xdefault = xcase;
				continue;
			}

			if (CompareValues(value, xcase.Value()))
			{
				newcase = xcase;
				break;
			}
		}

		if (!newcase && xdefault)
		{
			// Inject default if we found one without matching anything
			newcase = xdefault;
		}

		// Only bother changing things around if we actually have a new case.
		if (newcase != current)
		{
			// If we don't have any cases or default, setting these to null is what we want to be blank again.
			Content(newcase ? newcase.Content() : nullptr);
			SetValue(CurrentCaseProperty(), newcase);
		}
	}

	void SwitchPresenter::OnCasesChanged(const wfc::IObservableVector<Case> &, const wfc::IVectorChangedEventArgs &)
	{
		EvaluateCases();
	}

	void SwitchPresenter::OnLoaded(const IInspectable &, const wux::RoutedEventArgs &)
	{
		EvaluateCases();
	}

	bool SwitchPresenter::CompareValues(const IInspectable &compare, const IInspectable &value)
	{
		if (compare == value)
		{
			return true;
		}

		if (!compare || !value)
		{
			return false;
		}

		return ValueToString(compare) == ValueToString(value);
	}

	hstring SwitchPresenter::ValueToString(const IInspectable &value)
	{
		if (const auto str = value.try_as<hstring>())
		{
			return *str;
		}
		else
		{
			return wux::Markup::XamlBindingHelper::ConvertValue(xaml_typename<hstring>(), value).as<hstring>();
		}
	}
}
