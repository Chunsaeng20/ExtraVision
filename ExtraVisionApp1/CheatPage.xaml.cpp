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
	void CheatPage::InitializeComponent()
	{
		// Ŭ���� �ʱ�ȭ
		CheatPageT::InitializeComponent();
		BackgroundTask();
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// AI ��� ��ư �̺�Ʈ �ڵ鷯
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
		// Log ��� ��ư �̺�Ʈ �ڵ鷯
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
		// �����찡 GraphicsCapture�� ������ ���� ����
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
		// ������ �ڵ� ��������
		HWND hWnd = MainWindow::GetWindowHandle();

		// GraphicsCapturePicker �ʱ�ȭ
		auto picker = GraphicsCapturePicker();
		picker.as<IInitializeWithWindow>()->Initialize(hWnd);

		// ������ ��������
		auto item = co_await picker.PickSingleItemAsync();
		if (item != nullptr)
		{
			m_item = item;
			m_isItemExist.store(true);
		}
	}

	winrt::fire_and_forget CheatPage::ShowErrorMsg()
	{
		// �����찡 GraphicsCapture�� �������� ���� ���� ���� �޽���
		ContentDialog dialog{};
		dialog.XamlRoot(this->Content().XamlRoot());
		dialog.Title(box_value(L"����"));
		dialog.Content(box_value(L"�� ���� ȭ�� ĸ�ĸ� �������� �ʽ��ϴ�."));
		dialog.CloseButtonText(L"�ݱ�");
		co_await dialog.ShowAsync();
	}

	Windows::Foundation::IAsyncAction CheatPage::BackgroundTask()
	{
		// �ֿ� ������ �����ϴ� ��׶��� ������
		while (true)
		{
			// ��׶��� �����忡�� ����
			co_await winrt::resume_background();

			// �������� ������ ���� ���� ���� ����
			if (m_isItemExist.load())
			{

			}
		}
	}
}
