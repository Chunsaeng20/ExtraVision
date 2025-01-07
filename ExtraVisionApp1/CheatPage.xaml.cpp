#include "pch.h"
#include "CheatPage.xaml.h"
#include "MainWindow.xaml.h"
#if __has_include("CheatPage.g.cpp")
#include "CheatPage.g.cpp"
#endif
#include <shobjidl_core.h>
#include <winrt/Windows.Graphics.Capture.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ExtraVisionApp1::implementation
{
	void CheatPage::InitializeComponent()
	{
		// 클래스 초기화
		CheatPageT::InitializeComponent();

		// Direct3D 초기화
		auto d3dDevice = CreateD3DDevice();
		auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
		m_device = CreateDirect3DDevice(dxgiDevice.get());
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// AI 토글 버튼 이벤트 핸들러
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// AI 작동
				this->m_isAIOn.store(true);
			}
			else
			{
				// AI 정지
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
				// Log 녹화 시작
			}
			else
			{
				// Log 녹화 종료
			}
		}
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// 프로그램 검색 버튼 이벤트 핸들러
		if (GraphicsCaptureSession::IsSupported())
		{
			// 윈도우가 GraphicsCapture를 지원할 때만 가능
			OpenWindowList();
		}
		else
		{
			// 지원하지 않으면 에러 메시지
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
			// 화면 캡처 정지
			Close();

			// 가져온 아이템으로 초기화
			m_item = item;
			m_lastSize = m_item.Size();
			m_framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
			m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &CheatPage::OnFrameArrived });
			m_session = m_framePool.CreateCaptureSession(m_item);

			// 동기화
			m_isItemLoaded.store(true);

			// 화면 캡처 시작
			m_session.StartCapture();
		}
	}

	winrt::fire_and_forget CheatPage::ShowErrorMsg()
	{
		// 윈도우가 GraphicsCapture를 지원하지 않을 때의 에러 메시지
		ContentDialog dialog{};
		dialog.XamlRoot(this->Content().XamlRoot());
		dialog.Title(box_value(L"오류"));
		dialog.Content(box_value(L"이 기기는 화면 캡쳐를 지원하지 않습니다."));
		dialog.CloseButtonText(L"닫기");
		co_await dialog.ShowAsync();
	}

	void CheatPage::Close()
	{
		// Windows Graphics Capture API 정리 함수
		auto expected = true;
		if (m_isItemLoaded.compare_exchange_strong(expected, false))
		{
			m_frameArrived.revoke();
			m_framePool.Close();
			m_session.Close();

			m_swapChain = nullptr;
			m_framePool = nullptr;
			m_session = nullptr;
			m_item = nullptr;

			m_isItemLoaded.store(false);
		}
	}

	void CheatPage::OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const&)
	{
		// 캡처된 프레임이 프레임 풀에 저장될 때 발생하는 이벤트 핸들러
		// 주요 로직을 실행하는 백그라운드 스레드

		// 프레임 가져오기
		auto frame = sender.TryGetNextFrame();
		auto frameContentSize = frame.ContentSize();

		// 캡처할 윈도우의 사이즈가 변경되었을 경우
		auto newSize = false;
		if (frameContentSize.Width != m_lastSize.Width || frameContentSize.Height != m_lastSize.Height)
		{
			// 사이즈 재설정
			newSize = true;
			m_lastSize = frameContentSize;
		}

		// Direct3D11CaptureFrame을 ID3D11Texture2D로 변환
		auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

		// ID3D11Texture2D를 Bitmap으로 변환

		// Bitmap을 UI에 띄움

		// 변경된 윈도우의 사이즈를 반영
		if (newSize)
		{
			m_framePool.Recreate(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
		}

		// AI 모델에 넣기 (구현 필요)

		// 컴퓨터 제어 (구현 필요)
	}

	Windows::Foundation::IAsyncAction CheatPage::BackgroundTask()
	{
		// 주요 로직을 실행하는 백그라운드 스레드
		while (true)
		{
			// 백그라운드 스레드에서 실행
			co_await winrt::resume_background();
		}
	}
}
