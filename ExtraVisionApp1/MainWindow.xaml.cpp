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
	void MainWindow::InitializeComponent()
	{
		MainWindowT::InitializeComponent();
		ExtendsContentIntoTitleBar(true);
		SetTitleBar(TitleBarArea());
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::NavView_Loaded(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// �ʱ� ȭ�� ����
		NavView().SelectedItem(NavView().MenuItems().GetAt(0));
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::NavView_SelectionChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const&, winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args)
	{
		// NavigationView �̺�Ʈ �ڵ鷯
		auto selectedItem = unbox_value<hstring>(args.SelectedItemContainer().Content());
		if (selectedItem == L"Home")
		{
			openHomePage();
		}
		else if (selectedItem == L"Cheat")
		{
			openCheatPage();
		}
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::NavView_DisplayModeChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewDisplayModeChangedEventArgs const&)
	{
		// ������ Ÿ��Ʋ��
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
		// NavigationView Back ��ư �̺�Ʈ �ڵ鷯
		MainFrame().GoBack();
		setNavigationViewHeader();
	}

	void winrt::ExtraVisionApp1::implementation::MainWindow::MainFrame_Navigated(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&)
	{
		// NavigationView Back ��ư Ȱ��ȭ
		NavView().IsBackEnabled(MainFrame().CanGoBack());
	}

	void MainWindow::openHomePage()
	{
		// Home ������ ����
		MainFrame().Navigate(xaml_typename<HomePage>());
		setNavigationViewHeader();
	}

	void MainWindow::openCheatPage()
	{
		// Cheat ������ ����
		MainFrame().Navigate(xaml_typename<CheatPage>());
		setNavigationViewHeader();
	}

	void MainWindow::setNavigationViewHeader()
	{
		// NavigationView ��� ����
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
}
