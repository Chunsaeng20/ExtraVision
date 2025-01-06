#include "pch.h"
#include "CheatPage.xaml.h"
#if __has_include("CheatPage.g.cpp")
#include "CheatPage.g.cpp"
#endif
#include <winrt/Windows.Graphics.Capture.h>
#include <shobjidl_core.h>
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace winrt::Windows::Graphics::Capture;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ExtraVisionApp1::implementation
{
	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// AI 토글 버튼 이벤트 핸들러
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// AI ON
				this->m_isAIOn.store(true);
			}
			else
			{
				// AI OFF
				this->m_isAIOn.store(false);
			}
		}
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::LogSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// Log 토글 버튼 이벤트 핸들러
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// Log ON
				this->m_isLogOn.store(true);
			}
			else
			{
				// Log OFF
				this->m_isLogOn.store(false);
			}
		}
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// 윈도우가 GraphicsCapture를 지원할 때만 가능
		if (GraphicsCaptureSession::IsSupported())
		{
			OpenWindowList();
		}
		else
		{
			ShowErrorMsg();
		}
	}

	winrt::fire_and_forget CheatPage::OpenWindowList()
	{
		// 윈도우 핸들 가져오기
		HWND hWnd = MainWindow::GetWindowHandle();

		// GraphicsCapturePicker 초기화
		auto picker = GraphicsCapturePicker();
		picker.as<IInitializeWithWindow>()->Initialize(hWnd);

		// 아이템 가져오기
		auto item = co_await picker.PickSingleItemAsync();
		if (item != nullptr)
		{
			m_item = item;
		}
	}

	winrt::fire_and_forget CheatPage::ShowErrorMsg()
	{
		ContentDialog dialog{};
		dialog.XamlRoot(this->Content().XamlRoot());
		dialog.Title(box_value(L"오류"));
		dialog.Content(box_value(L"이 기기는 화면 캡쳐가 지원되지 않습니다."));
		dialog.CloseButtonText(L"닫기");
		co_await dialog.ShowAsync();
	}
}
