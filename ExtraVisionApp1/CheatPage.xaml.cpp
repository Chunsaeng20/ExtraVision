/*
* CheatPage.xaml.cpp
* - ����: ���ø����̼��� ���� ������ �����
* -     : ���α׷� Ž��, YOLO �� ����, ���α׷� ����, �α� ����� ��
* - ���: UI ������ �����ϰ� �����Ӱ� ���� ����
* -     : �ڵ带 �и��ϰ� �ʹٸ� ���ο� ������ �ۼ� �� CheatPage.xaml.cpp�� ���� include �� ��
* -     : ������ ����ų� �ʿ��� ��쿡�� CheatPage.xaml.h�� include �� ��
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

// --------------------- �� �������� include �� �� ---------------------

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
		// Ŭ���� �ʱ�ȭ
		CheatPageT::InitializeComponent();

		// Direct3D �ʱ�ȭ
		m_d3dDevice = CreateD3DDevice();
		m_d3dDevice->GetImmediateContext(m_d3dContext.put());
		m_dxgiDevice = m_d3dDevice.as<IDXGIDevice>();
		m_device = CreateDirect3DDevice(m_dxgiDevice.get());
	}

	// AI ��� ��ư �̺�Ʈ �ڵ鷯
	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// ��� �� AI �۵�
				std::thread TurnOnAI([this]() {
					// 3�� ���� ����
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));

					m_isAIOn.store(true);
					});
				TurnOnAI.detach();

				auto dispatcherQueue = this->DispatcherQueue();

				// ��� �� AI ����
				std::thread TurnOffAI([this, dispatcherQueue]() {
					// 63�� ���� ����
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
				// AI ��� ����
				this->m_isAIOn.store(false);
			}
		}
	}

	// ���α׷� �˻� ��ư �̺�Ʈ �ڵ鷯
	void winrt::ExtraVisionApp1::implementation::CheatPage::SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		if (GraphicsCaptureSession::IsSupported())
		{
			// �����찡 GraphicsCapture�� ������ ���� ����
			OpenWindowList();
		}
		else
		{
			// �������� ������ ���� �޽���
			ShowErrorMsg();
		}
	}

	// ������ ��� ����
	winrt::fire_and_forget CheatPage::OpenWindowList()
	{
		// ������ �ڵ� ��������
		HWND hWnd = MainWindow::GetWindowHandle();

		// GraphicsCapturePicker �ʱ�ȭ
		auto picker = GraphicsCapturePicker();
		picker.as<IInitializeWithWindow>()->Initialize(hWnd);

		// ������ ��������
		auto& item = co_await picker.PickSingleItemAsync();
		if (item != nullptr)
		{
			// OnFrameArrived ������ ����
			{
				// ȭ�� ĸó ����
				Close();

				// ������ ���������� �ʱ�ȭ
				m_item = item;
				m_lastSize = m_item.Size();
				m_swapChain = CreateDXGISwapChain(m_d3dDevice, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height), static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized), 2);
				m_framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
				m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &CheatPage::OnFrameArrived });
				m_session = m_framePool.CreateCaptureSession(m_item);

				// �̹��� �������� ���� ü�� ����
				auto panelNative{ ImageFrame().as<ISwapChainPanelNative>() };
				HRESULT hr = panelNative->SetSwapChain(m_swapChain.get());
				if (FAILED(hr)) co_return;

				// �̹��� ������ ũ�� ����
				m_imageFrameWidth = static_cast<int>(ImageFrame().ActualWidth());
				m_imageFrameHeight = static_cast<int>(ImageFrame().ActualHeight());
				m_imageFrameRatio = (float)m_imageFrameWidth / m_imageFrameHeight;

				// Frame Per Second ������ ���� �ʱ�ȭ
				m_fps = 0.0;
				m_frameCount = 0;
				m_startTime = std::chrono::high_resolution_clock::now();
				m_prevFrameTime = std::chrono::high_resolution_clock::now();

				// ����ȭ �÷��� �ѱ�
				m_isItemLoaded.store(true);

				// ȭ�� ĸó ����
				m_session.StartCapture();
			}

			// SelectAndTrackObject ������ ����
			{
				// �۾� ���� �����尡 ������ �����Ŵ
				if (m_consumerThread.joinable())
				{
					// �۾� ���� �����带 �����ϰ� �����Ŵ
					m_shouldExit.store(true);
					m_consumerThread.join();

					// ���� ��ȣ �÷��� �ʱ�ȭ
					m_shouldExit.store(false);
				}

				// ���ο� �۾� ������ ����
				m_consumerThread = std::thread(&CheatPage::SelectAndTrackObject, this);
			}
		}
	}

	// �����찡 GraphicsCapture�� �������� ���� ���� ���� �޽���
	winrt::fire_and_forget CheatPage::ShowErrorMsg()
	{
		ContentDialog dialog{};
		dialog.XamlRoot(this->Content().XamlRoot());
		dialog.Title(box_value(L"����"));
		dialog.Content(box_value(L"�� ���� ȭ�� ĸó�� �������� �ʽ��ϴ�."));
		dialog.CloseButtonText(L"�ݱ�");
		co_await dialog.ShowAsync();
	}

	// Windows Graphics Capture API �� �Һ��� ������ ���� �Լ�
	void CheatPage::Close()
	{
		auto expected = true;
		if (m_isItemLoaded.compare_exchange_strong(expected, false))
		{
			// ȭ�� ĸó ������ ����
			m_frameArrived.revoke();

			// ȭ�� ĸó�� ������ ������ ���
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] { return m_isLock == 0; });

			// Direct3D ���ҽ� ����
			m_framePool.Close();
			m_session.Close();
			m_swapChain = nullptr;
			m_framePool = nullptr;
			m_session = nullptr;
			m_item = nullptr;

			// ����ȭ �÷��� �ʱ�ȭ
			m_isItemLoaded.store(false);
		}

		// �۾� ���� �����尡 ������ �����Ŵ
		if (m_consumerThread.joinable())
		{
			// �۾� ���� �����带 �����ϰ� �����Ŵ
			m_shouldExit.store(true);
			m_consumerThread.join();

			// ���� ��ȣ �÷��� �ʱ�ȭ
			m_shouldExit.store(false);
		}
	}

	// ĸó�� �������� ������ Ǯ�� ����� �� �߻��ϴ� �̺�Ʈ �ڵ鷯
	// ĸó ������ �����ϴ� ��׶��� ������
	void CheatPage::OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const&)
	{
		// Direct3D ���ҽ��� ������ ������ ���� ������ ��
		m_isLock++;

		// ---------------------------------------------------------------------------------------------------------------
		// 1. ������ ��������
		//
		auto frame = sender.TryGetNextFrame();
		auto frameContentSize = frame.ContentSize();

		// ĸó�� �������� ����� ����Ǿ��� ���
		auto newSize = false;
		if (frameContentSize.Width != m_lastSize.Width || frameContentSize.Height != m_lastSize.Height)
		{
			// ������ �缳��
			newSize = true;
			m_lastSize = frameContentSize;
			m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height), static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized), 0);
		}

		// Direct3D11CaptureFrame�� ID3D11Texture2D�� ��ȯ
		auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

		// �������� ������ ��������
		D3D11_TEXTURE2D_DESC srcDesc = {};
		frameSurface->GetDesc(&srcDesc);

		// CPU���� ��밡���� �ؽ�ó�� �����ϱ� ���� ���� ����
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

		// CPU���� ��밡���� �ؽ�ó ����
		winrt::com_ptr<ID3D11Texture2D> IDestImage;
		HRESULT hr = m_d3dDevice->CreateTexture2D(&desc, NULL, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// CPU���� ��밡���� �ؽ�ó�� ����
		m_d3dContext->CopyResource(IDestImage.get(), frameSurface.get());

		// CPU�� ������ ����
		D3D11_MAPPED_SUBRESOURCE resource;
		UINT subresource = D3D11CalcSubresource(0, 0, 0);
		hr = m_d3dContext->Map(IDestImage.get(), subresource, D3D11_MAP_READ_WRITE, 0, &resource);
		if (FAILED(hr)) return;

		// �̹��� ������ ���� �غ�
		int imageHeight = static_cast<int>(desc.Height);
		int imageRowPitch = static_cast<int>(resource.RowPitch);
		int imageWidth = imageRowPitch / 4;
		int imageSize = imageRowPitch * imageHeight;
		float imageRatio = (float)imageWidth / imageHeight;

		// ĸó�ϴ� ȭ�� ũ�� ����
		m_imageWidth.store(imageWidth);
		m_imageHeight.store(imageHeight);

		// �̹��� ������ ����
		std::vector<BYTE> imageData;
		imageData.resize(imageSize);
		BYTE* srcData = reinterpret_cast<BYTE*>(resource.pData);
		BYTE* destData = imageData.data();
		memcpy(destData, srcData, imageSize);

		// �ؽ�ó ���
		m_d3dContext->Unmap(IDestImage.get(), 0);

		// ����� �������� ����� �ݿ�
		if (newSize) m_framePool.Recreate(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);

		// OpenCV Mat Ÿ������ ��ȯ
		// - ID3D11Texture2D�� BGRA 4ä��
		// - Row Major�� Column Major�� ��ȯ
		cv::Mat image(imageHeight, imageWidth, CV_8UC4, imageData.data());

		// ---------------------------------------------------------------------------------------------------------------
		// 2. AI �𵨿� �ֱ�
		//
		// ��ü Ž��
		std::vector<Detection> detections = m_detector.detect(image);

		// ---------------------------------------------------------------------------------------------------------------
		// 3. ť�� ��ü Ž�� ��� �ֱ�
		//
		{
			std::lock_guard<std::mutex> lock(m_detectQueueMutex);
			m_detectQueue.push(detections);
		}

		// ---------------------------------------------------------------------------------------------------------------
		// 4. UI ����
		//
		// Frame Per Second�� Frame Time ����
		m_frameCount++;
		auto current_time = std::chrono::high_resolution_clock::now();

		// Frame Time ����
		std::chrono::duration<double> elapsed_seconds = current_time - m_prevFrameTime;
		double frame_time_ms = elapsed_seconds.count() * 1000.0;
		m_prevFrameTime = current_time;

		// 1�ʸ��� FPS�� ���
		elapsed_seconds = current_time - m_startTime;
		if (elapsed_seconds.count() >= 1.0)
		{
			m_fps = m_frameCount / elapsed_seconds.count();
			m_startTime = current_time;
			m_frameCount = 0;
		}

		// FPS���� ���ڿ��� ��ȯ
		std::stringstream ss;
		ss << std::fixed << std::setprecision(2);
		ss << "FPS: " << m_fps;
		std::string fps_text = ss.str();

		// Frame Time ���� ���ڿ��� ��ȯ
		ss.str("");
		ss << "Frame Time: " << frame_time_ms << "ms";
		std::string frameTime_text = ss.str();

		// Ž���� ��ü�� �̹����� ǥ��
		cv::Mat boundingImage = image.clone();
		m_detector.drawBoundingBoxMask(boundingImage, detections);

		// cv::Mat ũ�⸦ ������ ���̿� �°� ����
		cv::Mat imageUI;
		imageHeight = m_imageFrameHeight;
		imageWidth = m_imageFrameHeight * imageRatio;
		cv::resize(boundingImage, imageUI, cv::Size(imageWidth, imageHeight), 0, 0, cv::INTER_LINEAR);

		// �̹��� �����ӿ� ������ �̹��� ����
		int horizontalPadding = (m_imageFrameWidth - imageWidth) > 0 ? m_imageFrameWidth - imageWidth : 0;
		int left = horizontalPadding / 2;
		int right = horizontalPadding - left;
		cv::Mat expandedImageUI;
		cv::copyMakeBorder(imageUI, expandedImageUI, 0, 0, left, right, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255, 255));

		// �̹����� ��� �ؽ�Ʈ �߰�
		cv::putText(expandedImageUI, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0, 255), 2, cv::LINE_AA);
		cv::putText(expandedImageUI, frameTime_text, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0, 255), 2, cv::LINE_AA);

		// cv::Mat�� GPU���� ��밡���� �ؽ�ó�� ��ȯ�ϱ� ���� �غ�
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

		// cv::Mat�� GPU���� ��밡���� �ؽ�ó�� ��ȯ
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = expandedImageUI.data;
		initData.SysMemPitch = expandedImageUI.cols * 4;
		initData.SysMemSlicePitch = expandedImageUI.cols * expandedImageUI.rows * 4;
		hr = m_d3dDevice->CreateTexture2D(&desc, &initData, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// SwapChain�� BackBuffer�� ������
		winrt::com_ptr<ID3D11Texture2D> backBuffer;
		hr = m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void());
		if (FAILED(hr)) return;

		// ���� Ÿ�� �� ����
		winrt::com_ptr<ID3D11RenderTargetView> rtv;
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		hr = m_d3dDevice->CreateRenderTargetView(backBuffer.get(), &rtvDesc, rtv.put());
		if (FAILED(hr)) return;

		// ȭ�� Ŭ����
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_d3dContext->OMSetRenderTargets(1, rtv.put(), nullptr);
		m_d3dContext->ClearRenderTargetView(rtv.get(), clearColor);

		// �̹����� ������ ȭ���� ũ�� ����
		D3D11_BOX region = {};
		region.left = static_cast<uint32_t>(0);
		region.top = static_cast<uint32_t>(0);
		region.right = static_cast<uint32_t>(m_imageFrameWidth);
		region.bottom = static_cast<uint32_t>(m_imageFrameHeight);
		region.back = 1;

		// BackBuffer�� �ؽ�ó�� ����
		m_d3dContext->CopySubresourceRegion(backBuffer.get(), 0, 0, 0, 0, IDestImage.get(), 0, &region);

		// ȭ�� ���
		DXGI_PRESENT_PARAMETERS parameters = { 0 };
		m_swapChain->Present1(1, 0, &parameters);

		// ---------------------------------------------------------------------------------------------------------------

		// ������ �� ����
		m_isLock--;
		cv.notify_all();
	}

	// OnFrameArrived���� ������ ��ü�� ���� ������ �Һ�
	// ��ü�� �����ϰ� �����ϴ� ������ �����ϴ� ��׶��� ������
	void CheatPage::SelectAndTrackObject()
	{
		// ����׿� �ڵ�
		std::wstringstream ss;
		int whatHappend = 0;

		std::vector<Detection> lastDetectedObjects; // ���������� ������ ��ü ����

		float sensitivity = 0.04f;					// �ΰ���
		float deadZoneRadius = 8.0f;				// ������ �ݰ�
		float smoothing = 0.8f;						// �������� �ε巴�� ����
		int prevMouseMoveX = 0;						// ���� ���콺 X��ǥ
		int prevMouseMoveY = 0;						// ���� ���콺 Y��ǥ
		int maxMovementDistance = 300;				// �� �����ӿ� �̵��� �ִ� �̵���
		float verticalOffset = 0.275f;				// ��ü�� ��� �κ��� ��������
		float predictionOffset = 2.0f;				// �󸶳� �ָ� ��������

		// --- '���'�� ���� ���� �߰� ---
		bool isCurrentlyTracking = false;			// ���� Ư�� ��ü�� ���� ������ ����
		Detection currentTarget;					// ���� ���� ���� ��ü�� ����
		Detection previousTarget;
		double currentTargetDistance = 1.0E10;		// ���� ���� ���� ��ü�� �Ÿ�

		// Ÿ���� ���ƴ��� �Ǵ��ϱ� ���� ������
		const float trackingLossDistanceThreshold = 100.0f; // ���� ��ġ���� �� �Ÿ�(�ȼ�) �̻� ����� �ٸ� ��ü�� �Ǵ�
		const int frameTolerance = 3;						// Ÿ���� ������ �� �� �����ӱ��� ��ġ�� ��������
		int frameToleranceCount = 0;

		// ��� ������
		const int fireDelay = 4;
		int fireDelayCount = 0;

		while (m_shouldExit.load() == false)
		{
			bool isNewDataExist = false;			// ť�� ����� ���� �÷���

			// ť���� ������ �������� �õ�
			{
				std::lock_guard<std::mutex> lock(m_detectQueueMutex);
				if (!m_detectQueue.empty())
				{
					// ť�� ������� �ʴٸ� �����͸� ������
					lastDetectedObjects = m_detectQueue.front();
					m_detectQueue.pop();
					isNewDataExist = true;
				}
			}

			// ���ο� ������ ������ �ϴ� ������ ����
			if (!isNewDataExist) continue;
			// ������ Ž�� ����� ����ִٸ� �ϴ� ������ ����
			else if (lastDetectedObjects.empty()) continue;

			// --- ��ü ���� ---
			// ���� ����ϴ� ��ü ���� ����
			// 1. ���� �����ӿ��� ���� ���� ��ü�� �ִٸ� �� ��ü�� ��� ����
			// 2. ���� ���� ��ü�� �Ҿ���ȴٸ� ȭ�� �߾ӿ��� ���� ����� ��ü�� ����
			// Ž���� ��ü�� ���α׷� �߾ӱ����� �Ÿ� ����
			int imageWidth = m_imageWidth.load();
			int imageHeight = m_imageHeight.load();
			int centerWidth = imageWidth / 2;
			int centerHeight = imageHeight / 2;

			Detection trackingTarget;					// ���� ������ ���� ���� ��ü�� ����
			Detection closestTarget;					// ���� ����� ��ü�� ����
			double minDistanceToLastTarget = 1.0E10;	// ���� ���� ��ü���� ���� ����� �Ÿ�
			double distanceTargetToCenter = 1.0E10;		// ���� ���� ��ü�� ȭ�� �߾ӱ����� �Ÿ�
			double closestItemDistance = 1.0E10;		// ȭ�� �߾Ӱ� ���� ����� ��ü�� �Ÿ�

			// ������ Ž�� ��� Ȯ��
			{
				for (auto& item : lastDetectedObjects)
				{
					// �� ��ü�� ����
					if (item.box.x < 730 && item.box.x > 710 && item.box.y < 620 && item.box.y > 600)
					{
						// �� ��ü�� ��ǥ�� 1080p ��üȭ�鿡�� 720, 612
						continue;
					}

					// ���� Ÿ�� ��ü�� �߽� ��ǥ
					int lastTargetCenterX = currentTarget.box.x + currentTarget.box.width / 2;
					int lastTargetCenterY = currentTarget.box.y + currentTarget.box.height / 2;

					// ���� ��ü�� �߽� ��ǥ
					int currentItemCenterX = item.box.x + item.box.width / 2;
					int currentItemCenterY = item.box.y + item.box.height / 2;

					// ���� Ÿ�� ��ü�� ���� ��ü�� ��Ŭ���� �Ÿ��� ���
					double distanceToTarget = std::sqrt(std::pow(lastTargetCenterX - currentItemCenterX, 2) + std::pow(lastTargetCenterY - currentItemCenterY, 2));

					// ȭ�� �߾Ӱ� ���� ��ü�� ��Ŭ���� �Ÿ��� ���
					double distanceToCenter = std::sqrt(std::pow(centerWidth - currentItemCenterX, 2) + std::pow(centerHeight - currentItemCenterY, 2));

					// ���� ���� ��ü ã��
					if (distanceToTarget < minDistanceToLastTarget)
					{
						// ���� ������ �ĺ��� �ϴ� ����
						minDistanceToLastTarget = distanceToTarget;
						distanceTargetToCenter = distanceToCenter;
						trackingTarget = item;
					}

					// ȭ�� �߾Ӱ� ���� ����� ��ü ã��
					if (distanceToCenter < closestItemDistance)
					{
						closestItemDistance = distanceToCenter;
						closestTarget = item;
					}
				}

				// ��ü�� ���� ���̾���, ���� ���� �ݰ� �ȿ� ���� Ÿ�� ��ü�� �����Ѵٸ� ������ ������
				if (isCurrentlyTracking && minDistanceToLastTarget < trackingLossDistanceThreshold)
				{
					previousTarget = currentTarget;
					currentTarget = trackingTarget; // Ÿ�� ���� ����
					currentTargetDistance = distanceTargetToCenter;
					closestItemDistance = currentTargetDistance;
					frameToleranceCount = 0;

					// ����׿� �ڵ�
					whatHappend = 1;
				}
				// ��ü�� ���� ������ �ʴٸ�
				else if (!isCurrentlyTracking)
				{
					previousTarget = closestTarget;
					currentTarget = closestTarget;	// ���� ����� ��ü�� ����
					currentTargetDistance = closestItemDistance;
					isCurrentlyTracking = true;

					// ����׿� �ڵ�
					whatHappend = 2;
				}
				// Ÿ���� ���ƴٸ�
				else
				{
					// ��� ������ ������ ��ٸ�
					frameToleranceCount++;
					if (frameToleranceCount < frameTolerance)
					{
						// ���� Ÿ���� ������ �״�� ���
						closestItemDistance = currentTargetDistance;

						// ����׿� �ڵ�
						whatHappend = 3;
					}
					// ���ġ�� �Ѿ�� ���� ����� ��ü�� ����
					else
					{
						frameToleranceCount = 0;
						previousTarget = closestTarget;
						currentTarget = closestTarget;
						currentTargetDistance = closestItemDistance;
						isCurrentlyTracking = false;

						// ����׿� �ڵ�
						whatHappend = 4;
					}
				}
			}

			// ��ǥ ��ü������ �ȼ� �Ÿ�
			int currentCenterX = currentTarget.box.x + currentTarget.box.width / 2;
			int currentCenterY = currentTarget.box.y + currentTarget.box.height / 2;

			int prevCenterX = previousTarget.box.x + previousTarget.box.width / 2;
			int prevCenterY = previousTarget.box.y + previousTarget.box.height / 2;

			int velocityX = currentCenterX - prevCenterX;
			int velocityY = currentCenterY - prevCenterY;

			// ���� ������ ���� �����̴� ��ü�� ��ġ�� ����
			int predictedCenterX = currentCenterX + static_cast<int>(velocityX * predictionOffset);
			int predictedCenterY = currentCenterY + static_cast<int>(velocityY * predictionOffset);

			int targetDx = (predictedCenterX - centerWidth);
			int targetDy = (predictedCenterY - centerHeight) - static_cast<int>(currentTarget.box.height * verticalOffset);

			// ���콺�� �̵��� ���� ��� ��ǥ
			int mouseMoveX = 0;
			int mouseMoveY = 0;

			// ������ �ۿ� ��ü�� ���� ���� �̵��� ���
			if (closestItemDistance > deadZoneRadius && closestItemDistance != 1.0E10)
			{
				// �ΰ��� ����
				float moveX = static_cast<float>(targetDx) * sensitivity;
				float moveY = static_cast<float>(targetDy) * sensitivity;

				// ������ ����
				moveX = prevMouseMoveX + (moveX - prevMouseMoveX) * smoothing;
				moveY = prevMouseMoveY + (moveY - prevMouseMoveY) * smoothing;

				// �̵��� ���� (Clamping)
				moveX = std::max(static_cast<float>(-maxMovementDistance), std::min(moveX, static_cast<float>(maxMovementDistance)));
				moveY = std::max(static_cast<float>(-maxMovementDistance), std::min(moveY, static_cast<float>(maxMovementDistance)));

				// ���� �̵���
				mouseMoveX = static_cast<int>(moveX);
				mouseMoveY = static_cast<int>(moveY);
			}

			// ���� �����ӿ��� ���콺 �̵��� ����
			prevMouseMoveX = mouseMoveX;
			prevMouseMoveY = mouseMoveY;

			// --- ��ü ���� ---
			// SendInput �Լ��� ������ �������� ���� �̺�Ʈ�� �߻���Ŵ
			// ���� ���� ��Ŀ���� �����쿡�� �Է��� �߻���
			// �̶� ���Ἲ ������ ���ų� ���� ���α׷����� ��ȿ�� �Է��� �߻���
			// 
			// AI�� ���������� ��ǻ�͸� ����
			if (m_isAIOn.load())
			{
				// �̵��� Ŭ���� ���� INPUT ����ü �迭
				INPUT inputs[3] = { 0 }; // 0: Move, 1: LeftDown, 2: LeftUp
				int inputCount = 0;

				// ���콺 �̵� ����
				inputs[inputCount].type = INPUT_MOUSE;
				inputs[inputCount].mi.dx = mouseMoveX;
				inputs[inputCount].mi.dy = mouseMoveY;
				inputs[inputCount].mi.dwFlags = MOUSEEVENTF_MOVE;
				inputCount++;

				// ������ �����Ǿ���, Ÿ���� ������ �� ���
				bool isAimStable = (std::abs(mouseMoveX) + std::abs(mouseMoveY) < 5);
				if (isAimStable && closestItemDistance != 1.0E10 && fireDelayCount > fireDelay)
				{
					ss << L"���\n";
					// ���콺 ���� ��ư Ŭ�� �̺�Ʈ (����)
					inputs[inputCount].type = INPUT_MOUSE;
					inputs[inputCount].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					inputCount++;

					// ���콺 ���� ��ư Ŭ�� �̺�Ʈ (��)
					inputs[inputCount].type = INPUT_MOUSE;
					inputs[inputCount].mi.dwFlags = MOUSEEVENTF_LEFTUP;
					inputCount++;

					fireDelayCount = 0;
				}
				else
				{
					fireDelayCount++;
				}

				// �غ�� �Էµ��� �ѹ��� ����
				if (inputCount > 0)
				{
					SendInput(inputCount, inputs, sizeof(INPUT));
				}
			}

			// ����׿� �ڵ�
			ss << L"���� �̵���: " << mouseMoveX << L", " << mouseMoveY << "\n";
			switch (whatHappend)
			{
			case 1: ss << L"��� ����\n"; break;
			case 2: ss << L"���� ����\n"; break;
			case 3: ss << L"��ħ\n"; break;
			case 4: ss << L"���ļ� ���� ����\n"; break;
			}
			OutputDebugString(ss.str().c_str());
			whatHappend = 0;
		}
	}
}
