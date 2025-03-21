/*
* MainWindow.xaml.cpp
* - 설명: 애플리케이션의 윈도우에 관한 설정을 담당함
* -     : 특히 타이틀바와 네비게이션뷰와 같은 UI를 담당함
* - 취급: 건들지 말 것
*/
#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ExtraVisionApp1::implementation
{
	// 정적 변수 초기화
	HWND MainWindow::m_hWnd{ 0 };

	void MainWindow::InitializeComponent()
	{
		// 클래스 초기화
		MainWindowT::InitializeComponent();
		ExtendsContentIntoTitleBar(true);
		SetTitleBar(TitleBarArea());
		SetWindowHandle();
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::NavView_Loaded(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// 초기 화면 설정
		NavView().SelectedItem(NavView().MenuItems().GetAt(0));
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::NavView_SelectionChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const&, winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args)
	{
		// NavigationView 이벤트 핸들러
		auto selectedItem = unbox_value<hstring>(args.SelectedItemContainer().Content());
		if (selectedItem == L"Home")
		{
			OpenHomePage();
		}
		else if (selectedItem == L"Cheat")
		{
			OpenCheatPage();
		}
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::NavView_DisplayModeChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewDisplayModeChangedEventArgs const&)
	{
		// 반응형 타이틀바
		auto top = unbox_value<Grid>(Content());
		auto titleBar = unbox_value<Grid>(top.FindName(L"TitleBarArea"));

		Thickness thickness;
		thickness.Left = sender.CompactPaneLength() * (sender.DisplayMode() == NavigationViewDisplayMode::Minimal ? 2 : 1);
		thickness.Top = titleBar.Margin().Top;
		thickness.Right = titleBar.Margin().Right;
		thickness.Bottom = titleBar.Margin().Bottom;

		titleBar.Margin(thickness);
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::NavView_BackRequested(winrt::Microsoft::UI::Xaml::Controls::NavigationView const&, winrt::Microsoft::UI::Xaml::Controls::NavigationViewBackRequestedEventArgs const&)
	{
		// NavigationView Back 버튼 이벤트 핸들러
		MainFrame().GoBack();
		SetNavigationViewHeader();
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::MainFrame_Navigated(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&)
	{
		// NavigationView Back 버튼 활성화
		NavView().IsBackEnabled(MainFrame().CanGoBack());
	}

	void MainWindow::OpenHomePage()
	{
		// Home 페이지 열기
		MainFrame().Navigate(xaml_typename<HomePage>());
		SetNavigationViewHeader();
	}

	void MainWindow::OpenCheatPage()
	{
		// Cheat 페이지 열기
		MainFrame().Navigate(xaml_typename<CheatPage>());
		SetNavigationViewHeader();
	}

	void MainWindow::SetNavigationViewHeader()
	{
		// NavigationView 헤더 설정
		auto currentPage = MainFrame().CurrentSourcePageType().Name;

		if (currentPage == xaml_typename<HomePage>().Name)
		{
			NavView().Header(box_value(L"Home"));
		}
		else if (currentPage == xaml_typename<CheatPage>().Name)
		{
			NavView().Header(box_value(L"Cheat"));
		}
	}

	void MainWindow::SetWindowHandle()
	{
		// 윈도우 핸들 설정하기
		auto windowNative{ this->m_inner.as<IWindowNative>() };
		windowNative->get_WindowHandle(&m_hWnd);
	}

	HWND MainWindow::GetWindowHandle()
	{
		// 윈도우 핸들 가져오기
		return m_hWnd;
	}
}
