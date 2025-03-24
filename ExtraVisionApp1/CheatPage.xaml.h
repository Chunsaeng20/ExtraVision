/*
* CheatPage.xaml.h
* - ����: ���ø����̼��� ���� ������ �����
* -     : ���α׷� Ž��, YOLO �� ����, ���α׷� ����, �α� ����� ��
* - ���: UI ������ �����ϰ� �����Ӱ� ���� ����
* -     : �ڵ带 �и��ϰ� �ʹٸ� ���ο� ������ �ۼ� �� CheatPage.xaml.cpp�� ���� include �� ��
* -     : ������ ����ų� �ʿ��� ��쿡�� CheatPage.xaml.h�� include �� ��
*/
#pragma once

#include "CheatPage.g.h"
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <opencv2/opencv.hpp>
#include <YOLO11.hpp>
#include <filesystem>
#include <onnxruntime_cxx_api.h>

#define MODELPATH "models\\yolo11n.onnx"    // YOLO �� ��� ���
#define LABELSPATH "models\\coco.names"     // �� ��� ���

// ���� ���� ��θ� �������� �Լ�
inline std::string GetModelDirectory(const std::string& filePath, const std::string& modelPath)
{
    std::filesystem::path path(filePath);
	path = path.parent_path();
	return path.string() + "\\" + modelPath;
}

// GPU ��� ���� ���θ� Ȯ���ϴ� �Լ�
inline bool isGpuAvailable()
{
	// CUDA ���� ����
    std::vector<std::string> availableProviders = Ort::GetAvailableProviders();
    auto cudaAvailable = std::find(availableProviders.begin(), availableProviders.end(), "CUDAExecutionProvider");

    if (cudaAvailable != Ort::GetAvailableProviders().end()) return true;
	else return false;
}

namespace winrt::ExtraVisionApp1::implementation
{
    struct CheatPage : CheatPageT<CheatPage>
    {
    public:
        // UI ���� �޼���
        CheatPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }
        ~CheatPage()
        {
            Close();
        }
        void InitializeComponent();
        void CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        // ����Ͻ� ���� ����
        // YOLO ��
        YOLO11Detector m_detector{ GetModelDirectory(__FILE__, MODELPATH), GetModelDirectory(__FILE__, LABELSPATH), isGpuAvailable()};

        // ��Ʈ�� ����
        std::atomic<bool> m_isAIOn = false;
        std::atomic<bool> m_isItemLoaded = false;
        std::atomic<int> m_isLock = 0;
        std::condition_variable cv;
        std::mutex mtx;

        // Windows Graphics Capture API
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrived;
        winrt::Windows::Graphics::SizeInt32 m_lastSize{ 0 };

        // Direct3D API
        winrt::com_ptr<IDXGIDevice> m_dxgiDevice{ nullptr };
        winrt::com_ptr<ID3D11Device> m_d3dDevice{ nullptr };
        winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
        winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };

        // �̹��� ������ ũ��
		int m_imageFrameWidth = 0;
		int m_imageFrameHeight = 0;
		float m_imageFrameRatio = 0.0f;

    private:
        // ����Ͻ� ���� �޼���
        void Close();
        winrt::fire_and_forget OpenWindowList();
        winrt::fire_and_forget ShowErrorMsg();
        void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args);
    };
}

namespace winrt::ExtraVisionApp1::factory_implementation
{
    struct CheatPage : CheatPageT<CheatPage, implementation::CheatPage>
    {
    };
}

// -------------------------------------------------- DirectX ���� �޼��� ���� --------------------------------------------------

// DXGI �������̽��� �������� �޼���
template <typename T>
auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
    auto access = object.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

// IDirect3DDevice�� �����ϴ� �޼���
inline auto CreateDirect3DDevice(IDXGIDevice* dxgi_device)
{
    winrt::com_ptr<::IInspectable> d3d_device;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device, d3d_device.put()));
    return d3d_device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

// ID3D11Device�� �����ϴ� ���� �޼���
inline auto CreateD3DDevice(D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device)
{
    WINRT_ASSERT(!device);

    return D3D11CreateDevice(nullptr, type, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr);
}

// DXGISwapChain�� �����ϴ� ���� �޼���
inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device, const DXGI_SWAP_CHAIN_DESC1* desc)
{
    auto dxgiDevice = device.as<IDXGIDevice2>();
    winrt::com_ptr<IDXGIAdapter> adapter;
    winrt::check_hresult(dxgiDevice->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
    winrt::com_ptr<IDXGIFactory2> factory;
    winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

    winrt::com_ptr<IDXGISwapChain1> swapchain;
    winrt::check_hresult(factory->CreateSwapChainForComposition(device.get(), desc, nullptr, swapchain.put()));

    return swapchain;
}

// DXGISwapChain�� �����ϴ� �޼���
inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t bufferCount)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferCount = bufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    return CreateDXGISwapChain(device, &desc);
}

// ID3D11Device�� �����ϴ� �޼���
inline auto CreateD3DDevice()
{
    winrt::com_ptr<ID3D11Device> device;
    HRESULT hr = CreateD3DDevice(D3D_DRIVER_TYPE_HARDWARE, device);

    if (DXGI_ERROR_UNSUPPORTED == hr)
    {
        hr = CreateD3DDevice(D3D_DRIVER_TYPE_WARP, device);
    }

    winrt::check_hresult(hr);
    return device;
}

// -------------------------------------------------- DirectX ���� �޼��� ���� --------------------------------------------------
