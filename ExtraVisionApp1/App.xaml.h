/*
* App.xaml.h
* - 설명: 애플리케이션 전반에 관한 설정을 담당함
* - 취급: 건들지 말 것
*/
#pragma once

#include "App.xaml.g.h"

namespace winrt::ExtraVisionApp1::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
