#include "AppDelegate.h"
#include "cocos2d.h"
#include <windows.h>
#include <tchar.h>

USING_NS_CC;

int WINAPI _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 创建应用实例，开始游戏循环
    AppDelegate app;
    return Application::getInstance()->run();
}