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

	void winrt::ExtraVisionApp1::implementation::CheatPage::CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// AI ��� ��ư �̺�Ʈ �ڵ鷯
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// AI �۵�
				this->m_isAIOn.store(true);
			}
			else
			{
				// AI ����
				this->m_isAIOn.store(false);
			}
		}
	}

	void winrt::ExtraVisionApp1::implementation::CheatPage::SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// ���α׷� �˻� ��ư �̺�Ʈ �ڵ鷯
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
			// ȭ�� ĸó ����
			Close();

			// ������ ���������� �ʱ�ȭ
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

			// ����ȭ
			m_isItemLoaded.store(true);

			// ȭ�� ĸó ����
			m_session.StartCapture();
		}
	}

	winrt::fire_and_forget CheatPage::ShowErrorMsg()
	{
		// �����찡 GraphicsCapture�� �������� ���� ���� ���� �޽���
		ContentDialog dialog{};
		dialog.XamlRoot(this->Content().XamlRoot());
		dialog.Title(box_value(L"����"));
		dialog.Content(box_value(L"�� ���� ȭ�� ĸó�� �������� �ʽ��ϴ�."));
		dialog.CloseButtonText(L"�ݱ�");
		co_await dialog.ShowAsync();
	}

	void CheatPage::Close()
	{
		// Windows Graphics Capture API ���� �Լ�
		auto expected = true;
		if (m_isItemLoaded.compare_exchange_strong(expected, false))
		{
			m_frameArrived.revoke();

			// ȭ�� ĸó�� ������ ������ ���
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
		// ������ ��
		m_isLock++;

		// ĸó�� �������� ������ Ǯ�� ����� �� �߻��ϴ� �̺�Ʈ �ڵ鷯
		// �ֿ� ������ �����ϴ� ��׶��� ������
		// ---------------------------------------------------------------------------------------------------------------
		// 1. ������ ��������
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

		// ID3D11Texture2D���� CPU�� ������ ����
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

		// CPU���� ��밡���� ���� ����
		winrt::com_ptr<ID3D11Texture2D> IDestImage;
		HRESULT hr = m_d3dDevice->CreateTexture2D(&desc, NULL, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// CPU���� ��밡���� ������ ����
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

		// �̹��� ������ ����
		std::vector<BYTE> imageData;
		imageData.resize(imageSize);
		BYTE* srcData = reinterpret_cast<BYTE*>(resource.pData);
		BYTE* destData = imageData.data();
		memcpy(destData, srcData, imageSize);

		// �ؽ�ó ���
		m_d3dContext->Unmap(IDestImage.get(), 0);

		// ����� �������� ����� �ݿ�
		if (newSize)
		{
			m_framePool.Recreate(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
		}

		// ---------------------------------------------------------------------------------------------------------------
		// 2. AI �𵨿� �ֱ�
		// OpenCV Mat Ÿ������ ��ȯ
		// - ID3D11Texture2D�� BGRA 4ä��
		// - Row Major�� Column Major�� ��ȯ
		cv::Mat image(imageHeight, imageWidth, CV_8UC4, imageData.data());

		// ��ü Ž��
		std::vector<Detection> detections = m_detector.detect(image);

		// Ž���� ��ü�� �̹����� ǥ��
		cv::Mat boundingImage = image.clone();
		m_detector.drawBoundingBoxMask(boundingImage, detections);

		// ---------------------------------------------------------------------------------------------------------------
		// 3. ��ǻ�� ����
		// ���� ����ϴ� ���� ����
		// -> Ž���� ��ü �� ���α׷� �߾�(������)�� ���� ����� ��ü�� ���콺 �̵� �� ���
		// Ž���� ��ü�� ���α׷� �߾ӱ����� �Ÿ� ����
		int centerWidth = imageWidth / 2;
		int centerHeight = imageHeight / 2;
		float closestItemDistance = 1.0E10;
		BoundingBox* closestItem = nullptr;
		for (auto& item : detections)
		{
			// ���ϴ� ��ü�� �ƴϸ� ��ŵ
			//if (item.classId != 0) continue;
			double distance = std::pow(centerWidth - item.box.x, 2) + std::pow(centerHeight - item.box.y, 2);
			// �ּ� �Ÿ��� ��ü�� ����
			if (distance < closestItemDistance)
			{
				closestItemDistance = distance;
				closestItem = &(item.box);
			}
		}

		// ���콺�� �̵��� ��ǥ ���
		if (closestItem)
		{
			int mouseMoveX = centerWidth - closestItem->x;
			int mouseMoveY = centerHeight - closestItem->y;

			// AI ��� ��ư�� ON�� ���� ��ǻ�͸� ����
			if (this->m_isAIOn.load())
			{
				// SendInput �Լ��� ������ �������� ���� �̺�Ʈ�� �߻���Ŵ
				// ���� ���� ��Ŀ���� �����쿡�� �Է��� �߻���
				// �̶� ���Ἲ ������ ���ų� ���� ���α׷����� ��ȿ�� �Է��� �߻���
				INPUT input = { 0 };
				input.type = INPUT_MOUSE;
				input.mi.dx = (-mouseMoveX * 65536) / GetSystemMetrics(SM_CXSCREEN);  // x ��ǥ (��ũ���� ���� ��ǥ)
				input.mi.dy = (-mouseMoveY * 65536) / GetSystemMetrics(SM_CYSCREEN);  // y ��ǥ (��ũ���� ���� ��ǥ)
				input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
				SendInput(1, &input, sizeof(INPUT));  // ���콺 �̵�

				// ���콺 ���� ��ư Ŭ�� �̺�Ʈ (����)
				input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				SendInput(1, &input, sizeof(INPUT));

				// ���콺 ���� ��ư Ŭ�� �̺�Ʈ (����)
				input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &input, sizeof(INPUT));
			}
		}
		
		// ---------------------------------------------------------------------------------------------------------------
		// 4. UI ����
		// cv::Mat ũ�� ����
		cv::Mat imageUI;
		float imageRatio = (float)imageWidth / imageHeight;
		imageHeight = m_imageFrameHeight;
		imageWidth = m_imageFrameHeight * imageRatio;
		cv::resize(boundingImage, imageUI, cv::Size(imageWidth, imageHeight), 0, 0, cv::INTER_LINEAR);

		// cv::Mat�� ID3D11Texture2D�� ��ȯ
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

		// ID3D11Texture2D �ؽ�ó ����
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = imageUI.data;
		initData.SysMemPitch = imageUI.cols * 4;
		initData.SysMemSlicePitch = imageUI.cols * imageUI.rows * 4;
		hr = m_d3dDevice->CreateTexture2D(&desc, &initData, IDestImage.put());
		if (FAILED(hr)) return;
		if (IDestImage == nullptr) return;

		// SwapChain�� BackBuffer�� �ؽ�ó�� �����ϰ� ȭ�鿡 ���
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
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_d3dContext->OMSetRenderTargets(1, rtv.put(), nullptr);		
		m_d3dContext->ClearRenderTargetView(rtv.get(), clearColor);

		// ����� ȭ�� ũ�� ����
		int left = (m_imageFrameWidth - imageWidth) / 2;
		int top = 0;

		// ������ ȭ�� ���� ����
		D3D11_BOX region = {};
		region.left = static_cast<uint32_t>(0);
		region.top = static_cast<uint32_t>(0);
		region.right = static_cast<uint32_t>(imageWidth);
		region.bottom = static_cast<uint32_t>(imageHeight);
		region.back = 1;

		// ȭ�� ����
		m_d3dContext->CopySubresourceRegion(backBuffer.get(), 0, static_cast<uint32_t>(left), static_cast<uint32_t>(top), 0, IDestImage.get(), 0, &region);

		// ȭ�� ���
		DXGI_PRESENT_PARAMETERS parameters = { 0 };
		m_swapChain->Present1(1, 0, &parameters);

		// ������ �� ����
		m_isLock--;
		cv.notify_all();
	}
}
