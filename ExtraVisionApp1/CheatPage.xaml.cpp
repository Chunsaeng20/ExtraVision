/*
* CheatPage.xaml.cpp
* - 설명: 애플리케이션의 메인 로직을 담당함
* -     : 프로그램 탐지, YOLO 모델 적용, 프로그램 제어, 로그 남기기 등
* - 취급: UI 로직을 제외하고 자유롭게 수정 가능
* -     : 코드를 분리하고 싶다면 새로운 파일을 작성 후 CheatPage.xaml.cpp로 직접 include 할 것
* -     : 오류가 생기거나 필요한 경우에만 CheatPage.xaml.h에 include 할 것
*/
#include "pch.h"
#include "CheatPage.xaml.h"
#include "MainWindow.xaml.h"
#if __has_include("CheatPage.g.cpp")
#include "CheatPage.g.cpp"
#endif
#include <shobjidl_core.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <microsoft.ui.xaml.media.dxinterop.h>
struct __declspec(uuid("905a0fef-bc53-11df-8c49-001e4fc686da")) IBufferByteAccess : ::IUnknown
{
	virtual HRESULT __stdcall Buffer(uint8_t** value) = 0;
};

// --------------------- 이 위쪽으로 include 할 것 ---------------------

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
		m_d3dDevice = CreateD3DDevice();
		m_d3dDevice->GetImmediateContext(m_d3dContext.put());
		m_dxgiDevice = m_d3dDevice.as<IDXGIDevice>();
		m_device = CreateDirect3DDevice(m_dxgiDevice.get());
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// AI 토글 버튼 이벤트 핸들러
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// 잠시 후 AI 작동
				std::thread TurnOnAI([this]() {
					// 3초 동안 잠들기
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));

					m_isAIOn.store(true);
					});
				TurnOnAI.detach();

				auto dispatcherQueue = this->DispatcherQueue();

				// 잠시 후 AI 정지
				std::thread TurnOffAI([this, dispatcherQueue]() {
					// 15초 동안 잠들기
					std::this_thread::sleep_for(std::chrono::milliseconds(15000));

					m_isAIOn.store(false);

					dispatcherQueue.TryEnqueue([this]()
						{
							CheatSwitch().IsOn(false);
						});
					});
				TurnOffAI.detach();
			}
			else
			{
				// AI 즉시 정지
				this->m_isAIOn.store(false);
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
			m_swapChain = CreateDXGISwapChain(m_d3dDevice, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height), static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized), 2);
			m_framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
			m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &CheatPage::OnFrameArrived });
			m_session = m_framePool.CreateCaptureSession(m_item);

			auto panelNative{ ImageFrame().as<ISwapChainPanelNative>() };
			HRESULT hr = panelNative->SetSwapChain(m_swapChain.get());
			if (FAILED(hr)) co_return;

			m_imageFrameWidth = static_cast<int>(ImageFrame().ActualWidth());
			m_imageFrameHeight = static_cast<int>(ImageFrame().ActualHeight());
			m_imageFrameRatio = (float)m_imageFrameWidth / m_imageFrameHeight;

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
		dialog.Content(box_value(L"이 기기는 화면 캡처를 지원하지 않습니다."));
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

			// 화면 캡처가 중지될 때까지 대기
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] { return m_isLock == 0; });

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
		// 스레드 락
		m_isLock++;

		// Frame Per Second 측정
		auto start_time = std::chrono::high_resolution_clock::now();

		// 캡처된 프레임이 프레임 풀에 저장될 때 발생하는 이벤트 핸들러
		// 주요 로직을 실행하는 백그라운드 스레드
		// ---------------------------------------------------------------------------------------------------------------
		// 1. 프레임 가져오기
		//
		auto frame = sender.TryGetNextFrame();
		auto frameContentSize = frame.ContentSize();

		// 캡처할 윈도우의 사이즈가 변경되었을 경우
		auto newSize = false;
		if (frameContentSize.Width != m_lastSize.Width || frameContentSize.Height != m_lastSize.Height)
		{
			// 사이즈 재설정
			newSize = true;
			m_lastSize = frameContentSize;
			m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height), static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized), 0);
		}

		// Direct3D11CaptureFrame을 ID3D11Texture2D로 변환
		auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

		// ID3D11Texture2D에서 CPU로 데이터 추출
		D3D11_TEXTURE2D_DESC srcDesc = {};
		frameSurface->GetDesc(&srcDesc);

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = srcDesc.Width;
		desc.Height = srcDesc.Height;
		desc.Format = srcDesc.Format;
		desc.ArraySize = 1;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_STAGING;

		// CPU에서 사용가능한 변수 생성
		winrt::com_ptr<ID3D11Texture2D> IDestImage;
		HRESULT hr = m_d3dDevice->CreateTexture2D(&desc, NULL, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// CPU에서 사용가능한 변수로 복사
		m_d3dContext->CopyResource(IDestImage.get(), frameSurface.get());

		// CPU로 데이터 추출
		D3D11_MAPPED_SUBRESOURCE resource;
		UINT subresource = D3D11CalcSubresource(0, 0, 0);
		hr = m_d3dContext->Map(IDestImage.get(), subresource, D3D11_MAP_READ_WRITE, 0, &resource);
		if (FAILED(hr)) return;

		// 이미지 데이터 추출 준비
		int imageHeight = static_cast<int>(desc.Height);
		int imageRowPitch = static_cast<int>(resource.RowPitch);
		int imageWidth = imageRowPitch / 4;
		int imageSize = imageRowPitch * imageHeight;

		// 이미지 데이터 추출
		std::vector<BYTE> imageData;
		imageData.resize(imageSize);
		BYTE* srcData = reinterpret_cast<BYTE*>(resource.pData);
		BYTE* destData = imageData.data();
		memcpy(destData, srcData, imageSize);

		// 텍스처 언맵
		m_d3dContext->Unmap(IDestImage.get(), 0);

		// 변경된 윈도우의 사이즈를 반영
		if (newSize)
		{
			m_framePool.Recreate(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
		}

		// ---------------------------------------------------------------------------------------------------------------
		// 2. AI 모델에 넣기
		// 
		// OpenCV Mat 타입으로 변환
		// - ID3D11Texture2D은 BGRA 4채널
		// - Row Major를 Column Major로 변환
		cv::Mat image(imageHeight, imageWidth, CV_8UC4, imageData.data());

		// 객체 탐지
		std::vector<Detection> detections = m_detector.detect(image);

		// 탐지된 객체를 이미지에 표시
		cv::Mat boundingImage = image.clone();
		m_detector.drawBoundingBoxMask(boundingImage, detections);

		// ---------------------------------------------------------------------------------------------------------------
		// 3. 컴퓨터 제어
		// 현재 사용하는 제어 로직
		// -> 탐지된 객체 중 프로그램 중앙(조준점)과 가장 가까운 객체로 마우스 이동 및 사격
		// 
		// 탐지된 객체와 프로그램 중앙까지의 거리 측정
		int centerWidth = imageWidth / 2;
		int centerHeight = imageHeight / 2;
		double closestItemDistance = 1.0E10;

		// 마우스가 이동할 상대 좌표
		int mouseMoveX = 0;
		int mouseMoveY = 0;
		for (auto& item : detections)
		{
			// 원하는 객체가 아니면 스킵
			//if (item.classId != 0) continue;

			// 맨해튼 거리 계산
			int centerX = item.box.x + item.box.width / 2;
			int centerY = item.box.y + item.box.height / 2;
			int dx = centerWidth - centerX;
			int dy = centerHeight - centerY;

			double manhattanDistance = std::abs(dx) + std::abs(dy);

			// 최소 거리인 객체를 추출
			if (manhattanDistance < closestItemDistance)
			{
				closestItemDistance = manhattanDistance;
				mouseMoveX = dx / 2 - item.box.width / 6;
				mouseMoveY = dy / 2 + item.box.height / 6;
			}
		}

		// 마우스가 이동할 상대 좌표 보정
		// 2차원 픽셀 변화량은 3차원 좌표계에서의 변화량과 다르므로
		// 오차가 발생함. 따라서 보정이 필요함
		float imageRatio = (float)imageWidth / imageHeight;
		float horizontalFOV = 60.0f;
		float verticalFOV = 2 * atan(tan(horizontalFOV / 2.0f) * imageRatio);

		// 객체의 상대 좌표 정규화 [-0.5, 0.5]
		float normalizedX = (float)mouseMoveX / imageWidth;
		float normalizedY = (float)mouseMoveY / imageHeight;

		// 정규화된 좌표를 각도 변화량으로 변환
		float angleX = normalizedX * horizontalFOV / 2;
		float angleY = normalizedY * verticalFOV / 2;

		// 각도 변화량을 픽셀 변화량으로 변환 및 마우스 민감도 적용
		float mouseSensitivity = 1.0f;
		mouseMoveX = static_cast<int>((angleX * imageWidth / horizontalFOV) * mouseSensitivity);
		mouseMoveY = static_cast<int>((angleY * imageHeight / verticalFOV) * mouseSensitivity);

		// 프레임 기반 보정
		// 목표에 도달하는 프레임 수를 설정
		static int targetFrames = 1;
		mouseMoveX = static_cast<int>(mouseMoveX / targetFrames);
		mouseMoveY = static_cast<int>(mouseMoveY / targetFrames);

		// SendInput 함수는 윈도우 전역으로 가상 이벤트를 발생시킴
		// 따라서 현재 포커스된 윈도우에만 입력이 발생함
		// 이때 무결성 수준이 낮거나 같은 프로그램에만 유효한 입력이 발생함
		INPUT input = { 0 };
		input.type = INPUT_MOUSE;
		input.mi.dx = -mouseMoveX;
		input.mi.dy = -mouseMoveY;

		// AI가 켜져있으면
		if (m_isAIOn.load())
		{
			// AI 제어 방식 (1: 완전 제어, 2: 부분 제어)
			int method = 1;
			switch (method)
			{
			case 1:
				// AI가 컴퓨터를 완전히 제어
				// 화면 중앙과 객체의 위치가 충분히 가까우면
				if (abs(mouseMoveX) + abs(mouseMoveY) < 10 && closestItemDistance != 1.0E10)
				{
					// 마우스 왼쪽 버튼 클릭 이벤트 (누름)
					input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					SendInput(1, &input, sizeof(INPUT));

					// 마우스 누름 이벤트의 확실한 작동을 위한 딜레이
					for(int i = 0; i < 10; i++)
						SendInput(1, &input, sizeof(INPUT));
				}

				// 마우스 이동
				input.mi.dwFlags = MOUSEEVENTF_MOVE;
				SendInput(1, &input, sizeof(INPUT));

				// 화면 중앙과 객체의 위치가 충분히 멀면
				if (abs(mouseMoveX) + abs(mouseMoveY) > 10)
				{
					// 마우스 왼쪽 버튼 클릭 이벤트 (뗌)
					input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(1, &input, sizeof(INPUT));
				}
				break;

			case 2:
				// AI가 컴퓨터를 부분적으로 제어
				// 화면 중앙과 객체의 위치가 충분히 가까우면
				if (abs(mouseMoveX) + abs(mouseMoveY) < 10 && closestItemDistance != 1.0E10)
				{
					// 마우스 왼쪽 버튼 클릭 이벤트 (누름)
					input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					SendInput(1, &input, sizeof(INPUT));

					// 마우스 누름 이벤트의 확실한 작동을 위한 딜레이
					for (int i = 0; i < 10; i++)
						SendInput(1, &input, sizeof(INPUT));
				}

				// 화면 중앙과 객체의 위치가 충분히 멀면
				if (abs(mouseMoveX) + abs(mouseMoveY) > 10)
				{
					// 마우스 왼쪽 버튼 클릭 이벤트 (뗌)
					input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(1, &input, sizeof(INPUT));
				}
				break;
			}
		}

		// ---------------------------------------------------------------------------------------------------------------
		// 4. UI 제어
		// # 화면 크기를 조절할 경우 이전 프레임의 잔상이 테두리에 남는 버그가 있으나 치명적이지 않아 놔둠
		// 
		// cv::Mat 크기를 프레임 높이에 맞게 조절
		cv::Mat imageUI;
		imageHeight = m_imageFrameHeight;
		imageWidth = m_imageFrameHeight * imageRatio;
		cv::resize(boundingImage, imageUI, cv::Size(imageWidth, imageHeight), 0, 0, cv::INTER_LINEAR);

		// cv::Mat을 ID3D11Texture2D로 변환
		IDestImage = nullptr;
		desc = {};
		desc.Width = imageUI.cols;
		desc.Height = imageUI.rows;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.ArraySize = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;

		// ID3D11Texture2D 텍스처 생성
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = imageUI.data;
		initData.SysMemPitch = imageUI.cols * 4;
		initData.SysMemSlicePitch = imageUI.cols * imageUI.rows * 4;
		hr = m_d3dDevice->CreateTexture2D(&desc, &initData, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// SwapChain의 BackBuffer에 텍스처를 복사하고 화면에 출력
		winrt::com_ptr<ID3D11Texture2D> backBuffer;
		hr = m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void());
		if (FAILED(hr)) return;

		// 렌더 타겟 뷰 생성
		winrt::com_ptr<ID3D11RenderTargetView> rtv;
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		hr = m_d3dDevice->CreateRenderTargetView(backBuffer.get(), &rtvDesc, rtv.put());
		if (FAILED(hr)) return;

		// 화면 클리어
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_d3dContext->OMSetRenderTargets(1, rtv.put(), nullptr);		
		m_d3dContext->ClearRenderTargetView(rtv.get(), clearColor);

		// 복사할 화면 대비 출력할 화면의 여백 측정
		int left = (m_imageFrameWidth - imageWidth) / 2;

		// 복사할 화면 영역 크기 측정
		D3D11_BOX region = {};
		region.left = static_cast<uint32_t>(0);
		region.top = static_cast<uint32_t>(0);
		region.right = static_cast<uint32_t>(imageWidth);
		region.bottom = static_cast<uint32_t>(imageHeight);
		region.back = 1;

		// 화면 복사
		m_d3dContext->CopySubresourceRegion(backBuffer.get(), 0, static_cast<uint32_t>(left), 0, 0, IDestImage.get(), 0, &region);

		// 화면 출력
		DXGI_PRESENT_PARAMETERS parameters = { 0 };
		m_swapChain->Present1(1, 0, &parameters);

		// ---------------------------------------------------------------------------------------------------------------

		// Frame Per Second 측정
		auto end_time = std::chrono::high_resolution_clock::now();
		auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
		double fps = 1000000.0 / frame_duration;
		OutputDebugStringW((to_hstring(fps) + L"\n").c_str());

		// 스레드 락 해제
		m_isLock--;
		cv.notify_all();
	}
}
