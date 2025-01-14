# ExtraVisionApp1

## Prerequisite
### Visual Studio
- Visual Studio Installer에서 "C++를 사용한 데스크톱 개발" 중 "Windows 앱 SDK C++ 템플릿" 설치
- ".NET 데스크톱 개발"도 설치하는 것을 권장
### OpenCV
- OpenCV 4.11.0 버전을 글로벌 세팅으로 설치
1. https://github.com/opencv/opencv/releases/tag/4.11.0 에서 opencv-4.11.0-windows.exe 를 다운로드
2. 실행하여 OpenCV 파일을 다운로드
3. build/include 폴더 안에 opencv2 폴더는 Visual Studio가 설치된 폴더로 이동하여 2022/Community/VC/Tools/MSVC/{사용하는 버전}/include 폴더로 복사
4. build/x64/vc16/bin 폴더 안에 파일들은 2022/Community/VC/Tools/MSVC/{사용하는 버전}/bin/Hostx64/x64 폴더로 복사
5. opencv_world4110.dll과 opencv_world4110d.dll 파일은 C://Windows//System32와 C://Windows//SysWOW64 폴더로도 복사
6. build/x64/vc16/lib 폴더 안에 파일들은 2022/Community/VC/Tools/MSVC/{사용하는 버전}/lib/x64 폴더로 복사
7. lib 파일 관련 오류가 발생할 경우에는 프로젝트 속성->구성 속성->링커->추가 종속성에 opencv_world4110.lib나 opencv_world4110d.lib를 추가할 것

## Workflow
- App : 애플리케이션 전체 설정
  - 타이틀바 관련 UI 설정
- MainWindow : 애플리케이션 레이아웃 설정
  - 타이틀바 및 네비게이션뷰 관련 UI 설정
- HomePage : 애플리케이션을 실행했을 때의 홈페이지
  - 소개 화면
- CheatPage : 주요 로직을 실행하는 페이지
  - 화면 캡처
  - AI 적용
  - 윈도우 제어
  - 로그 남기기
