#include <Windows.h>
#include <string>
#include <format>
#include <cstdint>

/// <summary>
/// ウィンドウプロシージャ
/// </summary>
/// <param name="hwnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/// <summary>
/// ロガー
/// </summary>
/// <param name="message">デバックログに出力したい変数など</param>
void Log(const std::string& message);

/// <summary>
/// 文字列をwstringに変換
/// </summary>
/// <param name="str">変換したい文字列</param>
/// <returns></returns>
std::wstring ConvertString(const std::string& str);

/// <summary>
/// 文字列をstringに変換
/// </summary>
/// <param name="str">変換したい文字列</param>
/// <returns></returns>
std::string ConvertString(const std::wstring& str);

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    WNDCLASS wc{};
    //ウィンドウプロシージャ
    wc.lpfnWndProc = WindowProc;
    //ウィンドウクラス名
    wc.lpszClassName = L"CG2WindowClass";
    //インスタンスハンドル
    wc.hInstance = GetModuleHandle(nullptr);
    //カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    //ウィンドウクラスを登録する
    RegisterClass(&wc);

    //クライアント領域のサイズ
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeigth = 720;

    //ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc = {0, 0, kClientWidth, kClientHeigth};

    //クライアント領域を元に実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    //ウィンドウの生成
    HWND hwnd = CreateWindow(
        wc.lpszClassName,       //利用するクラス名
        L"CG2",                 //タイトルバーの文字(何でも良い)
        WS_OVERLAPPEDWINDOW,    //よく見るウィンドウスタイル
        CW_USEDEFAULT,          //表示X座標(Windowsに任せる)
        CW_USEDEFAULT,          //表示Y座標(WindowsOSに任せる)
        wrc.right - wrc.left,   //ウィンドウ横幅
        wrc.bottom - wrc.top,   //ウィンドウ縦幅
        nullptr,                //親ウィンドウハンドル
        nullptr,                //メニューハンドル
        wc.hInstance,           //インスタンスハンドル
        nullptr);               //オプション

    //ウィンドウを表示する
    ShowWindow(hwnd, SW_SHOW);

    MSG msg{};
    //ウィンドウの×ボタンが押されるまでループ
    while (msg.message != WM_QUIT) {
        //ウィンドウにメッセージが来てたら最優先で処理させる
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            //ゲームの処理

        }
    }

	return 0;
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    //メッセージに応じてゲーム固有の処理を行う
    switch (msg)
    {
    case WM_DESTROY:
        //OSに対して、アプリの終了を伝える
        PostQuitMessage(0);
        return 0;
    }

    //標準のメッセージ処理を行う
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message) {
    OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0) {
        return std::wstring();
    }
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

std::string ConvertString(const std::wstring& str) {
    if (str.empty()) {
        return std::string();
    }

    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) {
        return std::string();
    }
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}