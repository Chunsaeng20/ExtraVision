/*
* HomePage.xaml.h
* - ����: ���ø����̼��� ó�� �������� ���� Ȩ�������� �����
* - ���: �Ұ��� ������ �����ϰ� �ǵ��� �� ��
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
