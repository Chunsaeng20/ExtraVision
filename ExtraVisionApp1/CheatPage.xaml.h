/*
* CheatPage.xaml.h
* - 설명: 애플리케이션의 메인 로직을 담당함
* -     : 프로그램 탐지, YOLO 모델 적용, 프로그램 제어, 로그 남기기 등
* - 취급: UI 로직을 제외하고 자유롭게 수정 가능
* -     : 코드를 분리하고 싶다면 새로운 파일을 작성 후 CheatPage.xaml.cpp로 직접 include 할 것
* -     : 오류가 생기거나 필요한 경우에만 CheatPage.xaml.h에 include 할 것
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

#define MODELPATH "models\\yolo11n.onnx"    // YOLO 모델 상대 경로
#define LABELSPATH "models\\coco.names"     // 라벨 상대 경로

// 모델의 절대 경로를 가져오는 함수
inline std::string GetModelDirectory(const std::string& filePath, const std::string& modelPath)
{
    std::filesystem::path path(filePath);
	path = path.parent_path();
	return path.string() + "\\" + modelPath;
}

// GPU 사용 가능 여부를 확인하는 함수
inline bool isGpuAvailable(bool useGpu = true)
{
    // GPU 사용을 원하지 않는 경우 false 반환
	if (!useGpu) return false;

	// CUDA 지원 여부
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
        // UI 로직 메서드
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
        // 비즈니스 로직 변수
        // YOLO 모델
        // # onnx runtime 관련 gpu inference 오류가 발생 <- 해결 필요
        YOLO11Detector m_detector{ GetModelDirectory(__FILE__, MODELPATH), GetModelDirectory(__FILE__, LABELSPATH), isGpuAvailable(false) };

        // 컨트롤 변수
		std::atomic<bool> m_isAIOn = false;         // AI 활성화 여부
		std::atomic<bool> m_isItemLoaded = false;   // 캡처 아이템이 로드되었는지 여부
		std::atomic<int> m_isLock = 0;              // 스레드 동기화를 위한 변수
		std::condition_variable cv;                 // 스레드 동기화를 위한 조건 변수
		std::mutex mtx;                             // 스레드 동기화를 위한 뮤텍스

        // Windows Graphics Capture API
		winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };                           // 캡처 아이템
		winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };                     // 캡처 세션
		winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };               // 프레임 풀
		winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrived; // 프레임 도착 이벤트 핸들러
		winrt::Windows::Graphics::SizeInt32 m_lastSize{ 0 };                                                // 마지막 캡처 크기

        // Direct3D API
		winrt::com_ptr<IDXGIDevice> m_dxgiDevice{ nullptr };                                                // DXGI 디바이스
		winrt::com_ptr<ID3D11Device> m_d3dDevice{ nullptr };                                                // D3D11 디바이스
		winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };                                             // DXGI 스왑 체인
		winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };                                        // D3D11 디바이스 컨텍스트
		winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };                 // D3D11 디바이스 래퍼

        // 이미지 프레임 크기
		int m_imageFrameWidth = 0;                  // 이미지 프레임 너비
		int m_imageFrameHeight = 0;                 // 이미지 프레임 높이
		float m_imageFrameRatio = 0.0f;             // 이미지 프레임 비율

		// Frame Per Second 측정용 변수
		double m_fps = 0.0;                                             // 초당 프레임 수
		long long m_frameCount = 0;                                     // 프레임 카운트
		std::chrono::high_resolution_clock::time_point m_startTime;     // 프레임 시작 시간

    private:
        // 비즈니스 로직 메서드
		// Windows Graphics Capture API 정리
        void Close();

        // 윈도우 목록 열기
        winrt::fire_and_forget OpenWindowList();

		// 윈도우가 GraphicsCapture를 지원하지 않을 때의 에러 메시지
        winrt::fire_and_forget ShowErrorMsg();        

		// 프레임 도착 이벤트 핸들러
        void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args);
    };
}

namespace winrt::ExtraVisionApp1::factory_implementation
{
    struct CheatPage : CheatPageT<CheatPage, implementation::CheatPage>
    {
    };
}

// -------------------------------------------------- DirectX 관련 메서드 시작 --------------------------------------------------

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

// ID3D11Device를 생성하는 원본 메서드
inline auto CreateD3DDevice(D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device)
{
    WINRT_ASSERT(!device);

    return D3D11CreateDevice(nullptr, type, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr);
}

// DXGISwapChain을 생성하는 원본 메서드
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

// DXGISwapChain을 생성하는 메서드
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

// -------------------------------------------------- DirectX 관련 메서드 종료 --------------------------------------------------
