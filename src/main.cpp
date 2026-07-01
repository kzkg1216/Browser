// MiniBrowser: Win32 + WebView2 による最小限のブラウザ
//
// 構成:
//   - メインウィンドウ (Win32 API)
//   - ツールバー: 戻る / 進む / 再読み込み ボタン + アドレスバー
//   - 残りの領域に WebView2 (Edge のレンダリングエンジン) を表示

#include <windows.h>
#include <commctrl.h>
#include <wrl.h>
#include <string>

#include "WebView2.h"

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr int kIdBack = 1001;
constexpr int kIdForward = 1002;
constexpr int kIdReload = 1003;
constexpr int kIdUrlBar = 1004;

constexpr wchar_t kHomeUrl[] = L"https://www.google.com";

HWND g_mainWindow = nullptr;
HWND g_backButton = nullptr;
HWND g_forwardButton = nullptr;
HWND g_reloadButton = nullptr;
HWND g_urlBar = nullptr;

ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webview;

int Scale(HWND hwnd, int value) {
    return MulDiv(value, GetDpiForWindow(hwnd), 96);
}

// ツールバーと WebView2 の位置・サイズをウィンドウに合わせて更新する
void Layout(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);

    const int pad = Scale(hwnd, 4);
    const int toolbarHeight = Scale(hwnd, 36);
    const int buttonWidth = Scale(hwnd, 32);
    const int controlHeight = toolbarHeight - pad * 2;

    int x = pad;
    MoveWindow(g_backButton, x, pad, buttonWidth, controlHeight, TRUE);
    x += buttonWidth + pad;
    MoveWindow(g_forwardButton, x, pad, buttonWidth, controlHeight, TRUE);
    x += buttonWidth + pad;
    MoveWindow(g_reloadButton, x, pad, buttonWidth, controlHeight, TRUE);
    x += buttonWidth + pad;
    MoveWindow(g_urlBar, x, pad, client.right - x - pad, controlHeight, TRUE);

    if (g_controller) {
        RECT bounds = {0, toolbarHeight, client.right, client.bottom};
        g_controller->put_Bounds(bounds);
    }
}

void UpdateNavButtons() {
    BOOL canGoBack = FALSE;
    BOOL canGoForward = FALSE;
    if (g_webview) {
        g_webview->get_CanGoBack(&canGoBack);
        g_webview->get_CanGoForward(&canGoForward);
    }
    EnableWindow(g_backButton, canGoBack);
    EnableWindow(g_forwardButton, canGoForward);
}

// アドレスバーの文字列へ移動する。スキーム省略時は https:// を補う
void NavigateToUrlBarText() {
    if (!g_webview) return;

    wchar_t buffer[2048];
    GetWindowTextW(g_urlBar, buffer, ARRAYSIZE(buffer));
    std::wstring url = buffer;
    if (url.empty()) return;
    if (url.find(L"://") == std::wstring::npos) {
        url = L"https://" + url;
    }
    if (FAILED(g_webview->Navigate(url.c_str()))) {
        MessageBoxW(g_mainWindow, L"URL を開けませんでした。", L"MiniBrowser",
                    MB_ICONWARNING);
    }
}

// アドレスバーで Enter キーを拾うためのサブクラス化プロシージャ
LRESULT CALLBACK UrlBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                            UINT_PTR /*subclassId*/, DWORD_PTR /*refData*/) {
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        NavigateToUrlBarText();
        return 0;
    }
    if (msg == WM_CHAR && wParam == VK_RETURN) {
        return 0;  // Enter のビープ音を抑止
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void CreateToolbarControls(HWND hwnd) {
    const HINSTANCE instance =
        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));

    g_backButton = CreateWindowW(
        L"BUTTON", L"←", WS_CHILD | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0,
        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdBack)), instance,
        nullptr);
    g_forwardButton = CreateWindowW(
        L"BUTTON", L"→", WS_CHILD | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0,
        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdForward)),
        instance, nullptr);
    g_reloadButton = CreateWindowW(
        L"BUTTON", L"↻", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdReload)), instance,
        nullptr);
    g_urlBar = CreateWindowW(
        L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0, 0,
        0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdUrlBar)),
        instance, nullptr);

    const HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    for (HWND control : {g_backButton, g_forwardButton, g_reloadButton, g_urlBar}) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
    SetWindowSubclass(g_urlBar, UrlBarProc, 0, 0);
}

// WebView2 の初期化。非同期 (COM のコールバック) で進む
void CreateWebView(HWND hwnd) {
    // ユーザーデータ (キャッシュ・Cookie 等) の保存先
    wchar_t userDataDir[MAX_PATH];
    ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\MiniBrowser", userDataDir,
                              ARRAYSIZE(userDataDir));

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataDir, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    MessageBoxW(hwnd,
                                L"WebView2 の初期化に失敗しました。\n"
                                L"Microsoft Edge WebView2 ランタイムを"
                                L"インストールしてください。",
                                L"MiniBrowser", MB_ICONERROR);
                    return result;
                }
                env->CreateCoreWebView2Controller(
                    hwnd,
                    Callback<
                        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd](HRESULT result,
                               ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) return result;

                            g_controller = controller;
                            g_controller->get_CoreWebView2(&g_webview);

                            EventRegistrationToken token;
                            // URL が変わったらアドレスバーへ反映
                            g_webview->add_SourceChanged(
                                Callback<ICoreWebView2SourceChangedEventHandler>(
                                    [](ICoreWebView2* sender,
                                       ICoreWebView2SourceChangedEventArgs*)
                                        -> HRESULT {
                                        LPWSTR uri = nullptr;
                                        if (SUCCEEDED(sender->get_Source(&uri)) &&
                                            uri) {
                                            SetWindowTextW(g_urlBar, uri);
                                            CoTaskMemFree(uri);
                                        }
                                        return S_OK;
                                    })
                                    .Get(),
                                &token);
                            // 履歴が変わったら戻る/進むボタンの有効状態を更新
                            g_webview->add_HistoryChanged(
                                Callback<ICoreWebView2HistoryChangedEventHandler>(
                                    [](ICoreWebView2*, IUnknown*) -> HRESULT {
                                        UpdateNavButtons();
                                        return S_OK;
                                    })
                                    .Get(),
                                &token);
                            // ページタイトルをウィンドウタイトルへ反映
                            g_webview->add_DocumentTitleChanged(
                                Callback<
                                    ICoreWebView2DocumentTitleChangedEventHandler>(
                                    [](ICoreWebView2* sender,
                                       IUnknown*) -> HRESULT {
                                        LPWSTR title = nullptr;
                                        if (SUCCEEDED(sender->get_DocumentTitle(
                                                &title)) &&
                                            title) {
                                            std::wstring text = title;
                                            CoTaskMemFree(title);
                                            text += L" - MiniBrowser";
                                            SetWindowTextW(g_mainWindow,
                                                           text.c_str());
                                        }
                                        return S_OK;
                                    })
                                    .Get(),
                                &token);

                            Layout(hwnd);
                            g_webview->Navigate(kHomeUrl);
                            return S_OK;
                        })
                        .Get());
                return S_OK;
            })
            .Get());

    if (FAILED(hr)) {
        MessageBoxW(hwnd,
                    L"WebView2 の初期化を開始できませんでした。\n"
                    L"Microsoft Edge WebView2 ランタイムを"
                    L"インストールしてください。",
                    L"MiniBrowser", MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateToolbarControls(hwnd);
            CreateWebView(hwnd);
            return 0;

        case WM_SIZE:
            if (g_controller) {
                g_controller->put_IsVisible(wParam != SIZE_MINIMIZED);
            }
            if (wParam != SIZE_MINIMIZED) {
                Layout(hwnd);
            }
            return 0;

        case WM_MOVE:
            if (g_controller) {
                g_controller->NotifyParentWindowPositionChanged();
            }
            return 0;

        case WM_DPICHANGED: {
            const RECT* suggested = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            Layout(hwnd);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case kIdBack:
                    if (g_webview) g_webview->GoBack();
                    return 0;
                case kIdForward:
                    if (g_webview) g_webview->GoForward();
                    return 0;
                case kIdReload:
                    if (g_webview) g_webview->Reload();
                    return 0;
            }
            break;

        case WM_DESTROY:
            if (g_controller) {
                g_controller->Close();
                g_controller = nullptr;
                g_webview = nullptr;
            }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MiniBrowserWindow";
    RegisterClassExW(&wc);

    g_mainWindow = CreateWindowExW(
        0, wc.lpszClassName, L"MiniBrowser", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800, nullptr, nullptr, instance,
        nullptr);
    if (!g_mainWindow) return 1;

    ShowWindow(g_mainWindow, showCommand);
    UpdateWindow(g_mainWindow);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
