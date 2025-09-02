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

	// AI 토글 버튼 이벤트 핸들러
	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
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
					// 63초 동안 잠들기
					std::this_thread::sleep_for(std::chrono::milliseconds(63000));

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

	// 프로그램 검색 버튼 이벤트 핸들러
	void winrt::ExtraVisionApp1::implementation::CheatPage::SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
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

	// 윈도우 목록 열기
	winrt::fire_and_forget CheatPage::OpenWindowList()
	{
		// 윈도우 핸들 가져오기
		HWND hWnd = MainWindow::GetWindowHandle();

		// GraphicsCapturePicker 초기화
		auto picker = GraphicsCapturePicker();
		picker.as<IInitializeWithWindow>()->Initialize(hWnd);

		// 아이템 가져오기
		auto& item = co_await picker.PickSingleItemAsync();
		if (item != nullptr)
		{
			// OnFrameArrived 스레드 설정
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

				// 이미지 프레임의 스왑 체인 설정
				auto panelNative{ ImageFrame().as<ISwapChainPanelNative>() };
				HRESULT hr = panelNative->SetSwapChain(m_swapChain.get());
				if (FAILED(hr)) co_return;

				// 이미지 프레임 크기 설정
				m_imageFrameWidth = static_cast<int>(ImageFrame().ActualWidth());
				m_imageFrameHeight = static_cast<int>(ImageFrame().ActualHeight());
				m_imageFrameRatio = (float)m_imageFrameWidth / m_imageFrameHeight;

				// Frame Per Second 측정용 변수 초기화
				m_fps = 0.0;
				m_frameCount = 0;
				m_startTime = std::chrono::high_resolution_clock::now();
				m_prevFrameTime = std::chrono::high_resolution_clock::now();

				// 동기화 플래그 켜기
				m_isItemLoaded.store(true);

				// 화면 캡처 시작
				m_session.StartCapture();
			}

			// SelectAndTrackObject 스레드 설정
			{
				// 작업 중인 스레드가 있으면 종료시킴
				if (m_consumerThread.joinable())
				{
					// 작업 중인 스레드를 안전하게 종료시킴
					m_shouldExit.store(true);
					m_consumerThread.join();

					// 종료 신호 플래그 초기화
					m_shouldExit.store(false);
				}

				// 새로운 작업 스레드 시작
				m_consumerThread = std::thread(&CheatPage::SelectAndTrackObject, this);
			}
		}
	}

	// 윈도우가 GraphicsCapture를 지원하지 않을 때의 에러 메시지
	winrt::fire_and_forget CheatPage::ShowErrorMsg()
	{
		ContentDialog dialog{};
		dialog.XamlRoot(this->Content().XamlRoot());
		dialog.Title(box_value(L"오류"));
		dialog.Content(box_value(L"이 기기는 화면 캡처를 지원하지 않습니다."));
		dialog.CloseButtonText(L"닫기");
		co_await dialog.ShowAsync();
	}

	// Windows Graphics Capture API 및 소비자 스레드 정리 함수
	void CheatPage::Close()
	{
		auto expected = true;
		if (m_isItemLoaded.compare_exchange_strong(expected, false))
		{
			// 화면 캡처 세션을 중지
			m_frameArrived.revoke();

			// 화면 캡처가 중지될 때까지 대기
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] { return m_isLock == 0; });

			// Direct3D 리소스 정리
			m_framePool.Close();
			m_session.Close();
			m_swapChain = nullptr;
			m_framePool = nullptr;
			m_session = nullptr;
			m_item = nullptr;

			// 동기화 플래그 초기화
			m_isItemLoaded.store(false);
		}

		// 작업 중인 스레드가 있으면 종료시킴
		if (m_consumerThread.joinable())
		{
			// 작업 중인 스레드를 안전하게 종료시킴
			m_shouldExit.store(true);
			m_consumerThread.join();

			// 종료 신호 플래그 초기화
			m_shouldExit.store(false);
		}
	}

	// 캡처된 프레임이 프레임 풀에 저장될 때 발생하는 이벤트 핸들러
	// 캡처 로직을 실행하는 백그라운드 스레드
	void CheatPage::OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const&)
	{
		// Direct3D 리소스의 안전한 접근을 위해 스레드 락
		m_isLock++;

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

		// 프레임의 포맷을 가져오기
		D3D11_TEXTURE2D_DESC srcDesc = {};
		frameSurface->GetDesc(&srcDesc);

		// CPU에서 사용가능한 텍스처를 생성하기 위한 포맷 설정
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

		// CPU에서 사용가능한 텍스처 생성
		winrt::com_ptr<ID3D11Texture2D> IDestImage;
		HRESULT hr = m_d3dDevice->CreateTexture2D(&desc, NULL, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// CPU에서 사용가능한 텍스처로 복사
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
		float imageRatio = (float)imageWidth / imageHeight;

		// 캡처하는 화면 크기 저장
		m_imageWidth.store(imageWidth);
		m_imageHeight.store(imageHeight);

		// 이미지 데이터 추출
		std::vector<BYTE> imageData;
		imageData.resize(imageSize);
		BYTE* srcData = reinterpret_cast<BYTE*>(resource.pData);
		BYTE* destData = imageData.data();
		memcpy(destData, srcData, imageSize);

		// 텍스처 언맵
		m_d3dContext->Unmap(IDestImage.get(), 0);

		// 변경된 윈도우의 사이즈를 반영
		if (newSize) m_framePool.Recreate(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);

		// OpenCV Mat 타입으로 변환
		// - ID3D11Texture2D은 BGRA 4채널
		// - Row Major를 Column Major로 변환
		cv::Mat image(imageHeight, imageWidth, CV_8UC4, imageData.data());

		// ---------------------------------------------------------------------------------------------------------------
		// 2. AI 모델에 넣기
		//
		// 객체 탐지
		std::vector<Detection> detections = m_detector.detect(image);

		// ---------------------------------------------------------------------------------------------------------------
		// 3. 큐에 객체 탐지 결과 넣기
		//
		{
			std::lock_guard<std::mutex> lock(m_detectQueueMutex);
			m_detectQueue.push(detections);
		}

		// ---------------------------------------------------------------------------------------------------------------
		// 4. UI 제어
		//
		// Frame Per Second와 Frame Time 측정
		m_frameCount++;
		auto current_time = std::chrono::high_resolution_clock::now();

		// Frame Time 측정
		std::chrono::duration<double> elapsed_seconds = current_time - m_prevFrameTime;
		double frame_time_ms = elapsed_seconds.count() * 1000.0;
		m_prevFrameTime = current_time;

		// 1초마다 FPS를 계산
		elapsed_seconds = current_time - m_startTime;
		if (elapsed_seconds.count() >= 1.0)
		{
			m_fps = m_frameCount / elapsed_seconds.count();
			m_startTime = current_time;
			m_frameCount = 0;
		}

		// FPS값을 문자열로 변환
		std::stringstream ss;
		ss << std::fixed << std::setprecision(2);
		ss << "FPS: " << m_fps;
		std::string fps_text = ss.str();

		// Frame Time 값을 문자열로 변환
		ss.str("");
		ss << "Frame Time: " << frame_time_ms << "ms";
		std::string frameTime_text = ss.str();

		// 탐지된 객체를 이미지에 표시
		cv::Mat boundingImage = image.clone();
		m_detector.drawBoundingBoxMask(boundingImage, detections);

		// cv::Mat 크기를 프레임 높이에 맞게 조절
		cv::Mat imageUI;
		imageHeight = m_imageFrameHeight;
		imageWidth = m_imageFrameHeight * imageRatio;
		cv::resize(boundingImage, imageUI, cv::Size(imageWidth, imageHeight), 0, 0, cv::INTER_LINEAR);

		// 이미지 프레임에 꽉차게 이미지 조절
		int horizontalPadding = (m_imageFrameWidth - imageWidth) > 0 ? m_imageFrameWidth - imageWidth : 0;
		int left = horizontalPadding / 2;
		int right = horizontalPadding - left;
		cv::Mat expandedImageUI;
		cv::copyMakeBorder(imageUI, expandedImageUI, 0, 0, left, right, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255, 255));

		// 이미지에 통계 텍스트 추가
		cv::putText(expandedImageUI, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0, 255), 2, cv::LINE_AA);
		cv::putText(expandedImageUI, frameTime_text, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0, 255), 2, cv::LINE_AA);

		// cv::Mat을 GPU에서 사용가능한 텍스처로 변환하기 위해 준비
		IDestImage = nullptr;
		desc = {};
		desc.Width = expandedImageUI.cols;
		desc.Height = expandedImageUI.rows;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.ArraySize = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;

		// cv::Mat을 GPU에서 사용가능한 텍스처로 변환
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = expandedImageUI.data;
		initData.SysMemPitch = expandedImageUI.cols * 4;
		initData.SysMemSlicePitch = expandedImageUI.cols * expandedImageUI.rows * 4;
		hr = m_d3dDevice->CreateTexture2D(&desc, &initData, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// SwapChain의 BackBuffer를 가져옴
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

		// 이미지를 복사할 화면의 크기 측정
		D3D11_BOX region = {};
		region.left = static_cast<uint32_t>(0);
		region.top = static_cast<uint32_t>(0);
		region.right = static_cast<uint32_t>(m_imageFrameWidth);
		region.bottom = static_cast<uint32_t>(m_imageFrameHeight);
		region.back = 1;

		// BackBuffer에 텍스처를 복사
		m_d3dContext->CopySubresourceRegion(backBuffer.get(), 0, 0, 0, 0, IDestImage.get(), 0, &region);

		// 화면 출력
		DXGI_PRESENT_PARAMETERS parameters = { 0 };
		m_swapChain->Present1(1, 0, &parameters);

		// ---------------------------------------------------------------------------------------------------------------

		// 스레드 락 해제
		m_isLock--;
		cv.notify_all();
	}

	// OnFrameArrived에서 생성한 객체에 대한 정보를 소비
	// 객체를 선택하고 추적하는 로직을 실행하는 백그라운드 스레드
	void CheatPage::SelectAndTrackObject()
	{
		// 디버그용 코드
		std::wstringstream ss;
		int whatHappend = 0;

		std::vector<Detection> lastDetectedObjects; // 마지막으로 가져온 객체 정보

		float sensitivity = 0.04f;					// 민감도
		float deadZoneRadius = 8.0f;				// 데드존 반경
		float smoothing = 0.8f;						// 움직임을 부드럽게 만듬
		int prevMouseMoveX = 0;						// 이전 마우스 X좌표
		int prevMouseMoveY = 0;						// 이전 마우스 Y좌표
		int maxMovementDistance = 300;				// 한 프레임에 이동할 최대 이동량
		float verticalOffset = 0.275f;				// 객체의 어느 부분을 조준할지
		float predictionOffset = 2.0f;				// 얼마나 멀리 예측할지

		// --- '기억'을 위한 변수 추가 ---
		bool isCurrentlyTracking = false;			// 현재 특정 객체를 추적 중인지 여부
		Detection currentTarget;					// 현재 추적 중인 객체의 정보
		Detection previousTarget;
		double currentTargetDistance = 1.0E10;		// 현재 추적 중인 객체의 거리

		// 타겟을 놓쳤는지 판단하기 위한 설정값
		const float trackingLossDistanceThreshold = 100.0f; // 이전 위치에서 이 거리(픽셀) 이상 벗어나면 다른 객체로 판단
		const int frameTolerance = 3;						// 타겟을 놓쳤을 때 몇 프레임까지 위치를 유지할지
		int frameToleranceCount = 0;

		// 사격 딜레이
		const int fireDelay = 4;
		int fireDelayCount = 0;

		while (m_shouldExit.load() == false)
		{
			bool isNewDataExist = false;			// 큐가 비었을 때의 플래그

			// 큐에서 데이터 가져오기 시도
			{
				std::lock_guard<std::mutex> lock(m_detectQueueMutex);
				if (!m_detectQueue.empty())
				{
					// 큐가 비어있지 않다면 데이터를 가져옴
					lastDetectedObjects = m_detectQueue.front();
					m_detectQueue.pop();
					isNewDataExist = true;
				}
			}

			// 새로운 정보가 없으면 일단 가만히 있음
			if (!isNewDataExist) continue;
			// 가져온 탐지 결과가 비어있다면 일단 가만히 있음
			else if (lastDetectedObjects.empty()) continue;

			// --- 객체 선택 ---
			// 현재 사용하는 객체 선택 로직
			// 1. 현재 프레임에서 추적 중인 객체가 있다면 그 객체를 계속 추적
			// 2. 추적 중인 객체를 잃어버렸다면 화면 중앙에서 가장 가까운 객체를 추적
			// 탐지된 객체와 프로그램 중앙까지의 거리 측정
			int imageWidth = m_imageWidth.load();
			int imageHeight = m_imageHeight.load();
			int centerWidth = imageWidth / 2;
			int centerHeight = imageHeight / 2;

			Detection trackingTarget;					// 가장 유력한 추적 중인 객체의 정보
			Detection closestTarget;					// 가장 가까운 객체의 정보
			double minDistanceToLastTarget = 1.0E10;	// 추적 중인 객체와의 가장 가까운 거리
			double distanceTargetToCenter = 1.0E10;		// 추적 중인 객체의 화면 중앙까지의 거리
			double closestItemDistance = 1.0E10;		// 화면 중앙과 가장 가까운 객체의 거리

			// 가져온 탐지 결과 확인
			{
				for (auto& item : lastDetectedObjects)
				{
					// 손 객체는 무시
					if (item.box.x < 730 && item.box.x > 710 && item.box.y < 620 && item.box.y > 600)
					{
						// 손 객체의 좌표는 1080p 전체화면에서 720, 612
						continue;
					}

					// 이전 타겟 객체의 중심 좌표
					int lastTargetCenterX = currentTarget.box.x + currentTarget.box.width / 2;
					int lastTargetCenterY = currentTarget.box.y + currentTarget.box.height / 2;

					// 현재 객체의 중심 좌표
					int currentItemCenterX = item.box.x + item.box.width / 2;
					int currentItemCenterY = item.box.y + item.box.height / 2;

					// 이전 타겟 객체와 현재 객체의 유클리드 거리를 계산
					double distanceToTarget = std::sqrt(std::pow(lastTargetCenterX - currentItemCenterX, 2) + std::pow(lastTargetCenterY - currentItemCenterY, 2));

					// 화면 중앙과 현재 객체의 유클리드 거리를 계산
					double distanceToCenter = std::sqrt(std::pow(centerWidth - currentItemCenterX, 2) + std::pow(centerHeight - currentItemCenterY, 2));

					// 추적 중인 객체 찾기
					if (distanceToTarget < minDistanceToLastTarget)
					{
						// 가장 유력한 후보로 일단 저장
						minDistanceToLastTarget = distanceToTarget;
						distanceTargetToCenter = distanceToCenter;
						trackingTarget = item;
					}

					// 화면 중앙과 가장 가까운 객체 찾기
					if (distanceToCenter < closestItemDistance)
					{
						closestItemDistance = distanceToCenter;
						closestTarget = item;
					}
				}

				// 객체를 추적 중이었고, 추적 유지 반경 안에 이전 타겟 객체가 존재한다면 추적을 유지함
				if (isCurrentlyTracking && minDistanceToLastTarget < trackingLossDistanceThreshold)
				{
					previousTarget = currentTarget;
					currentTarget = trackingTarget; // 타겟 정보 갱신
					currentTargetDistance = distanceTargetToCenter;
					closestItemDistance = currentTargetDistance;
					frameToleranceCount = 0;

					// 디버그용 코드
					whatHappend = 1;
				}
				// 객체를 추적 중이지 않다면
				else if (!isCurrentlyTracking)
				{
					previousTarget = closestTarget;
					currentTarget = closestTarget;	// 가장 가까운 객체를 추적
					currentTargetDistance = closestItemDistance;
					isCurrentlyTracking = true;

					// 디버그용 코드
					whatHappend = 2;
				}
				// 타겟을 놓쳤다면
				else
				{
					// 허용 프레임 수까지 기다림
					frameToleranceCount++;
					if (frameToleranceCount < frameTolerance)
					{
						// 이전 타겟의 정보를 그대로 사용
						closestItemDistance = currentTargetDistance;

						// 디버그용 코드
						whatHappend = 3;
					}
					// 허용치를 넘어서면 가장 가까운 객체를 추적
					else
					{
						frameToleranceCount = 0;
						previousTarget = closestTarget;
						currentTarget = closestTarget;
						currentTargetDistance = closestItemDistance;
						isCurrentlyTracking = false;

						// 디버그용 코드
						whatHappend = 4;
					}
				}
			}

			// 목표 객체까지의 픽셀 거리
			int currentCenterX = currentTarget.box.x + currentTarget.box.width / 2;
			int currentCenterY = currentTarget.box.y + currentTarget.box.height / 2;

			int prevCenterX = previousTarget.box.x + previousTarget.box.width / 2;
			int prevCenterY = previousTarget.box.y + previousTarget.box.height / 2;

			int velocityX = currentCenterX - prevCenterX;
			int velocityY = currentCenterY - prevCenterY;

			// 선형 예측을 통해 움직이는 객체의 위치를 예측
			int predictedCenterX = currentCenterX + static_cast<int>(velocityX * predictionOffset);
			int predictedCenterY = currentCenterY + static_cast<int>(velocityY * predictionOffset);

			int targetDx = (predictedCenterX - centerWidth);
			int targetDy = (predictedCenterY - centerHeight) - static_cast<int>(currentTarget.box.height * verticalOffset);

			// 마우스가 이동할 최종 상대 좌표
			int mouseMoveX = 0;
			int mouseMoveY = 0;

			// 데드존 밖에 객체가 있을 때만 이동량 계산
			if (closestItemDistance > deadZoneRadius && closestItemDistance != 1.0E10)
			{
				// 민감도 적용
				float moveX = static_cast<float>(targetDx) * sensitivity;
				float moveY = static_cast<float>(targetDy) * sensitivity;

				// 스무딩 적용
				moveX = prevMouseMoveX + (moveX - prevMouseMoveX) * smoothing;
				moveY = prevMouseMoveY + (moveY - prevMouseMoveY) * smoothing;

				// 이동량 제한 (Clamping)
				moveX = std::max(static_cast<float>(-maxMovementDistance), std::min(moveX, static_cast<float>(maxMovementDistance)));
				moveY = std::max(static_cast<float>(-maxMovementDistance), std::min(moveY, static_cast<float>(maxMovementDistance)));

				// 최종 이동량
				mouseMoveX = static_cast<int>(moveX);
				mouseMoveY = static_cast<int>(moveY);
			}

			// 현재 프레임에서 마우스 이동량 저장
			prevMouseMoveX = mouseMoveX;
			prevMouseMoveY = mouseMoveY;

			// --- 객체 추적 ---
			// SendInput 함수는 윈도우 전역으로 가상 이벤트를 발생시킴
			// 따라서 현재 포커스된 윈도우에만 입력이 발생함
			// 이때 무결성 수준이 낮거나 같은 프로그램에만 유효한 입력이 발생함
			// 
			// AI가 켜져있으면 컴퓨터를 제어
			if (m_isAIOn.load())
			{
				// 이동과 클릭을 위한 INPUT 구조체 배열
				INPUT inputs[3] = { 0 }; // 0: Move, 1: LeftDown, 2: LeftUp
				int inputCount = 0;

				// 마우스 이동 설정
				inputs[inputCount].type = INPUT_MOUSE;
				inputs[inputCount].mi.dx = mouseMoveX;
				inputs[inputCount].mi.dy = mouseMoveY;
				inputs[inputCount].mi.dwFlags = MOUSEEVENTF_MOVE;
				inputCount++;

				// 조준이 안정되었고, 타겟이 존재할 때 사격
				bool isAimStable = (std::abs(mouseMoveX) + std::abs(mouseMoveY) < 5);
				if (isAimStable && closestItemDistance != 1.0E10 && fireDelayCount > fireDelay)
				{
					ss << L"사격\n";
					// 마우스 왼쪽 버튼 클릭 이벤트 (누름)
					inputs[inputCount].type = INPUT_MOUSE;
					inputs[inputCount].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					inputCount++;

					// 마우스 왼쪽 버튼 클릭 이벤트 (뗌)
					inputs[inputCount].type = INPUT_MOUSE;
					inputs[inputCount].mi.dwFlags = MOUSEEVENTF_LEFTUP;
					inputCount++;

					fireDelayCount = 0;
				}
				else
				{
					fireDelayCount++;
				}

				// 준비된 입력들을 한번에 전송
				if (inputCount > 0)
				{
					SendInput(inputCount, inputs, sizeof(INPUT));
				}
			}

			// 디버그용 코드
			ss << L"최종 이동량: " << mouseMoveX << L", " << mouseMoveY << "\n";
			switch (whatHappend)
			{
			case 1: ss << L"계속 추적\n"; break;
			case 2: ss << L"새로 추적\n"; break;
			case 3: ss << L"놓침\n"; break;
			case 4: ss << L"놓쳐서 새로 추적\n"; break;
			}
			OutputDebugString(ss.str().c_str());
			whatHappend = 0;
		}
	}
}
