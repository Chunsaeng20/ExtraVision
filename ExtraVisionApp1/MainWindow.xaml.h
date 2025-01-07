#pragma once

#include "MainWindow.g.h"

namespace winrt::ExtraVisionApp1::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }
        void InitializeComponent();
        void NavView_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void NavView_SelectionChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);
        void NavView_DisplayModeChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewDisplayModeChangedEventArgs const& args);
        void NavView_BackRequested(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewBackRequestedEventArgs const& args);
        void MainFrame_Navigated(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);
        static HWND GetWindowHandle();

    private:
        static HWND m_hWnd;

    private:
        void OpenHomePage();
        void OpenCheatPage();
        void SetNavigationViewHeader();
        void SetWindowHandle();
    };
}

namespace winrt::ExtraVisionApp1::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
