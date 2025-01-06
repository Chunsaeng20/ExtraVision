#include "pch.h"
#include "CheatPage.xaml.h"
#if __has_include("CheatPage.g.cpp")
#include "CheatPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ExtraVisionApp1::implementation
{
	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// AI ON
			}
			else
			{
				// AI OFF
			}
		}
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::LogSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// Log ON

				//toggleSwitch.IsOn(false);
			}
			else
			{
				// Log OFF
			}
		}
	}
}
