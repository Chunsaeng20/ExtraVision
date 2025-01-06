#pragma once

#include "CheatPage.g.h"

namespace winrt::ExtraVisionApp1::implementation
{
    struct CheatPage : CheatPageT<CheatPage>
    {
        CheatPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }
        void CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void LogSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::ExtraVisionApp1::factory_implementation
{
    struct CheatPage : CheatPageT<CheatPage, implementation::CheatPage>
    {
    };
}
