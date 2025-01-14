/*
* HomePage.xaml.h
* - 설명: 애플리케이션을 처음 실행했을 때의 홈페이지를 담당함
* - 취급: 소개문 내용을 제외하고 건들지 말 것
*/
#pragma once

#include "HomePage.g.h"

namespace winrt::ExtraVisionApp1::implementation
{
    struct HomePage : HomePageT<HomePage>
    {
        HomePage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }
    };
}

namespace winrt::ExtraVisionApp1::factory_implementation
{
    struct HomePage : HomePageT<HomePage, implementation::HomePage>
    {
    };
}
