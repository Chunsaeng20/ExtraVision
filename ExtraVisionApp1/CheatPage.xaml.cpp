#include "pch.h"
#include "CheatPage.xaml.h"
#include "MainWindow.xaml.h"
#if __has_include("CheatPage.g.cpp")
#include "CheatPage.g.cpp"
#endif
#include <shobjidl_core.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Storage.Streams.h>

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

	void winrt::ExtraVisionApp1::implementation::CheatPage::LogSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
	{
		// Log ��� ��ư �̺�Ʈ �ڵ鷯
		auto toggleSwitch = sender.as<ToggleSwitch>();
		if (toggleSwitch != NULL)
		{
			if (toggleSwitch.IsOn())
			{
				// Log ��ȭ ����
			}
			else
			{
				// Log ��ȭ ����
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
			m_framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
			m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &CheatPage::OnFrameArrived });
			m_session = m_framePool.CreateCaptureSession(m_item);

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
		dialog.Content(box_value(L"�� ���� ȭ�� ĸ�ĸ� �������� �ʽ��ϴ�."));
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
		// ĸó�� �������� ������ Ǯ�� ����� �� �߻��ϴ� �̺�Ʈ �ڵ鷯
		// �ֿ� ������ �����ϴ� ��׶��� ������
		// ------------------------------------------------------------
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
		hr = m_d3dContext->Map(IDestImage.get(), 0, D3D11_MAP_READ, 0, &resource);
		if (FAILED(hr)) return;

		// �̹��� ������ ����
		BYTE* imageData = reinterpret_cast<BYTE*>(resource.pData);
		UINT imageSize = resource.RowPitch * m_lastSize.Height;

		// �ؽ�ó ���
		m_d3dContext->Unmap(IDestImage.get(), 0);

		// ���ۿ� ���
		winrt::Windows::Storage::Streams::Buffer buffer(imageSize);
		memcpy(buffer.data(), imageData, imageSize);

		// Bitmap�� UI�� ���
		ShowWindowImage(buffer);

		// ����� �������� ����� �ݿ�
		if (newSize)
		{
			m_framePool.Recreate(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
		}

		// ------------------------------------------------------------
		// 2. AI �𵨿� �ֱ� (���� �ʿ�)

		// ------------------------------------------------------------
		// 3. ��ǻ�� ���� (���� �ʿ�)
	}

	winrt::fire_and_forget CheatPage::ShowWindowImage(winrt::Windows::Storage::Streams::Buffer buffer)
	{
		// ��Ʈ������ ��ȯ
		Windows::Storage::Streams::InMemoryRandomAccessStream stream;
		co_await stream.WriteAsync(buffer);

		// �̹��� ����
		//Bmp().SetSourceAsync(stream);
		co_return;
	}

	Windows::Foundation::IAsyncAction CheatPage::BackgroundTask()
	{
		// �ֿ� ������ �����ϴ� ��׶��� ������
		while (true)
		{
			// ��׶��� �����忡�� ����
			co_await winrt::resume_background();
		}
	}
}
