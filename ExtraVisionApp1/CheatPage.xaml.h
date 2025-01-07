#pragma once

#include "CheatPage.g.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/Windows.Graphics.Capture.h>

namespace winrt::ExtraVisionApp1::implementation
{
    struct CheatPage : CheatPageT<CheatPage>
    {
    public:
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
        void LogSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        // 컨트롤 변수
        std::atomic<bool> m_isAIOn = false;
        std::atomic<bool> m_isItemLoaded = false;

        // Windows Graphics Capture API
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrived;
        winrt::Windows::Graphics::SizeInt32 m_lastSize{ 0 };

        // Direct3D API
        winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
        winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };

    private:
        // 메서드
        void Close();
        winrt::fire_and_forget OpenWindowList();
        winrt::fire_and_forget ShowErrorMsg();
        void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args);
        Windows::Foundation::IAsyncAction BackgroundTask();
    };
}

namespace winrt::ExtraVisionApp1::factory_implementation
{
    struct CheatPage : CheatPageT<CheatPage, implementation::CheatPage>
    {
    };
}

// DXGI 인터페이스를 가져오는 메서드
template <typename T>
auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
    auto access = object.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

// IDirect3DDevice를 생성하는 메서드
inline auto CreateDirect3DDevice(IDXGIDevice* dxgi_device)
{
    winrt::com_ptr<::IInspectable> d3d_device;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device, d3d_device.put()));
    return d3d_device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

// ID3D11Device를 생성하는 메서드
inline auto CreateD3DDevice(D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device)
{
    WINRT_ASSERT(!device);

    return D3D11CreateDevice(nullptr, type, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr);
}

// ID3D11Device를 생성하는 메서드
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
