/*
* MainWindow.xaml.h
* - 설명: 애플리케이션의 윈도우에 관한 설정을 담당함
* -     : 특히 타이틀바와 네비게이션뷰와 같은 UI를 담당함
* - 취급: 건들지 말 것
*/
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
