#pragma once

#include "CheatPage.g.h"
#include <dxgi1_2.h>
#include <d3d11.h>
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
        void InitializeComponent();
        void CheatSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void LogSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void SearchProgramBtn_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        std::atomic<bool> m_isAIOn = false;
        std::atomic<bool> m_isLogOn = false;
        std::atomic<bool> m_isItemExist = false;

        winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
        //winrt::Windows::Graphics::SizeInt32 m_lastSize;

        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
        winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
        winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };

        std::atomic<bool> m_closed = false;
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrived;

        winrt::fire_and_forget OpenWindowList();
        winrt::fire_and_forget ShowErrorMsg();
        Windows::Foundation::IAsyncAction BackgroundTask();
        // BackgroundTask();
        // --ShowWindowImage();
        // --ProcessWithYOLO();
        // --ControlProgram();
        // PrintOutLog();
    };
}

namespace winrt::ExtraVisionApp1::factory_implementation
{
    struct CheatPage : CheatPageT<CheatPage, implementation::CheatPage>
    {
    };
}
