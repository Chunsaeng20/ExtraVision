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

		// 캡처된 프레임이 프레임 풀에 저장될 때 발생하는 이벤트 핸들러
		// 주요 로직을 실행하는 백그라운드 스레드
		// ---------------------------------------------------------------------------------------------------------------
		// 1. 프레임 가져오기
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
		// 탐지된 객체와 프로그램 중앙까지의 거리 측정
		int centerWidth = imageWidth / 2;
		int centerHeight = imageHeight / 2;
		float closestItemDistance = 1.0E10;
		BoundingBox* closestItem = nullptr;
		for (auto& item : detections)
		{
			// 원하는 객체가 아니면 스킵
			//if (item.classId != 0) continue;
			double distance = std::pow(centerWidth - item.box.x, 2) + std::pow(centerHeight - item.box.y, 2);
			// 최소 거리인 객체를 추출
			if (distance < closestItemDistance)
			{
				closestItemDistance = distance;
				closestItem = &(item.box);
			}
		}

		// 마우스가 이동할 좌표 계산
		if (closestItem)
		{
			int mouseMoveX = centerWidth - closestItem->x;
			int mouseMoveY = centerHeight - closestItem->y;

			// AI 토글 버튼이 ON일 때만 컴퓨터를 제어
			if (this->m_isAIOn.load())
			{
				// SendInput 함수는 윈도우 전역으로 가상 이벤트를 발생시킴
				// 따라서 현재 포커스된 윈도우에만 입력이 발생함
				// 이때 무결성 수준이 낮거나 같은 프로그램에만 유효한 입력이 발생함
				INPUT input = { 0 };
				input.type = INPUT_MOUSE;
				input.mi.dx = (-mouseMoveX * 65536) / GetSystemMetrics(SM_CXSCREEN);  // x 좌표 (스크린의 절대 좌표)
				input.mi.dy = (-mouseMoveY * 65536) / GetSystemMetrics(SM_CYSCREEN);  // y 좌표 (스크린의 절대 좌표)
				input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
				SendInput(1, &input, sizeof(INPUT));  // 마우스 이동

				// 마우스 왼쪽 버튼 클릭 이벤트 (누름)
				input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				SendInput(1, &input, sizeof(INPUT));

				// 마우스 왼쪽 버튼 클릭 이벤트 (떼기)
				input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &input, sizeof(INPUT));
			}
		}
		
		// ---------------------------------------------------------------------------------------------------------------
		// 4. UI 제어
		// cv::Mat 크기 조절
		cv::Mat imageUI;
		float imageRatio = (float)imageWidth / imageHeight;
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
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_d3dContext->OMSetRenderTargets(1, rtv.put(), nullptr);		
		m_d3dContext->ClearRenderTargetView(rtv.get(), clearColor);

		// 출력할 화면 크기 조절
		int left = (m_imageFrameWidth - imageWidth) / 2;
		int top = 0;

		// 복사할 화면 영역 조절
		D3D11_BOX region = {};
		region.left = static_cast<uint32_t>(0);
		region.top = static_cast<uint32_t>(0);
		region.right = static_cast<uint32_t>(imageWidth);
		region.bottom = static_cast<uint32_t>(imageHeight);
		region.back = 1;

		// 화면 복사
		m_d3dContext->CopySubresourceRegion(backBuffer.get(), 0, static_cast<uint32_t>(left), static_cast<uint32_t>(top), 0, IDestImage.get(), 0, &region);

		// 화면 출력
		DXGI_PRESENT_PARAMETERS parameters = { 0 };
		m_swapChain->Present1(1, 0, &parameters);

		// 스레드 락 해제
		m_isLock--;
		cv.notify_all();
	}
}
