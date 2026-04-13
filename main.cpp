#include <windows.h>
//
#include <windowsx.h>
#include <winerror.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.UI.h>
#include <winrt/base.h>

using namespace winrt;
using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Windowing;
using namespace winrt::Windows::Graphics;

static constexpr int kLogicalTitleBarHeight = 40;
static constexpr UINT kMsgInitTitleBar = WM_APP + 1;

struct AppState
{
    AppWindow appWindow{nullptr};
    AppWindowTitleBar titleBar{nullptr};

    int leftInset = 0;
    int rightInset = 0;
    int titleHeight = kLogicalTitleBarHeight;
};

static AppState g_state{};
STDAPI WindowsAppRuntime_EnsureIsLoaded();

static int GetDpiForHwndSafe(HWND hwnd)
{
    using GetDpiForWindowFn = UINT(WINAPI *)(HWND);
    static auto fn =
        reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    return fn ? static_cast<int>(fn(hwnd)) : 96;
}

static int ScaleByDpi(HWND hwnd, int px96) { return MulDiv(px96, GetDpiForHwndSafe(hwnd), 96); }

#include <winrt/Microsoft.UI.Interop.h>

static winrt::Windows::UI::Color MakeColor(BYTE a, BYTE r, BYTE g, BYTE b)
{
    winrt::Windows::UI::Color c{};
    c.A = a;
    c.R = r;
    c.G = g;
    c.B = b;
    return c;
}

static void ApplyTitleBarColors()
{
    auto tb = g_state.titleBar;

    auto bg = MakeColor(255, 45, 45, 48);
    auto fg = MakeColor(255, 255, 255, 255);
    auto hoverBg = MakeColor(255, 60, 60, 64);
    auto pressedBg = MakeColor(255, 75, 75, 80);
    auto inactiveBg = MakeColor(255, 45, 45, 48);
    auto inactiveFg = MakeColor(255, 200, 200, 200);

    tb.BackgroundColor(bg);
    tb.ForegroundColor(fg);
    tb.InactiveBackgroundColor(inactiveBg);
    tb.InactiveForegroundColor(inactiveFg);

    tb.ButtonBackgroundColor(bg);
    tb.ButtonForegroundColor(fg);
    tb.ButtonHoverBackgroundColor(hoverBg);
    tb.ButtonHoverForegroundColor(fg);
    tb.ButtonPressedBackgroundColor(pressedBg);
    tb.ButtonPressedForegroundColor(fg);
    tb.ButtonInactiveBackgroundColor(inactiveBg);
    tb.ButtonInactiveForegroundColor(inactiveFg);
}

static bool InitAppWindowTitleBar(HWND hwnd)
{
    try
    {
        if (!AppWindowTitleBar::IsCustomizationSupported())
        {
            printf("AppWindowTitleBar::IsCustomizationSupported() == false\n");
            return false;
        }

        printf("Calling GetWindowIdFromWindow\n");
        auto id = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);
        printf("Calling AppWindow::GetFromWindowId\n");
        g_state.appWindow = AppWindow::GetFromWindowId(id);
        if (!g_state.appWindow)
        {
            printf("AppWindow::GetFromWindowId returned null\n");
            return false;
        }

        printf("Calling AppWindow::TitleBar\n");
        g_state.titleBar = g_state.appWindow.TitleBar();
        if (!g_state.titleBar)
        {
            printf("AppWindow::TitleBar returned null\n");
            return false;
        }

        printf("ExtendsContentIntoTitleBar\n");
        g_state.titleBar.ExtendsContentIntoTitleBar(true);
        printf("ExtendsContentIntoTitleBar done\n");
        g_state.titleBar.IconShowOptions(IconShowOptions::ShowIconAndSystemMenu);
        ApplyTitleBarColors();

        winrt::Windows::UI::Color themeColor{255, 45, 45, 48};
        winrt::Windows::UI::Color whiteColor{255, 255, 255, 255};

        g_state.titleBar.ButtonBackgroundColor(themeColor);
        g_state.titleBar.BackgroundColor(themeColor);
        g_state.titleBar.ForegroundColor(whiteColor);

        return true;
    }
    catch (const winrt::hresult_error &e)
    {
        printf("InitAppWindowTitleBar hresult_error: 0x%08lx\n", static_cast<unsigned long>(e.code().value));
        auto msg = winrt::to_string(e.message());
        if (!msg.empty()) { printf("Message: %s\n", msg.c_str()); }
        return false;
    }
    catch (...)
    {
        printf("InitAppWindowTitleBar unknown exception\n");
        return false;
    }
}

static void UpdateTitleBarMetrics(HWND hwnd)
{
    if (!g_state.titleBar) return;

    g_state.leftInset = g_state.titleBar.LeftInset();
    g_state.rightInset = g_state.titleBar.RightInset();
    g_state.titleHeight = g_state.titleBar.Height();

    if (g_state.titleHeight <= 0) { g_state.titleHeight = ScaleByDpi(hwnd, kLogicalTitleBarHeight); }
}

static void DrawTitleBar(HDC hdc, const RECT &rcClient)
{

    RECT rcContent = {0, 0, rcClient.right, rcClient.bottom};
    HBRUSH hbrContent = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &rcContent, hbrContent);
    DeleteObject(hbrContent);
}
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            PostMessageW(hwnd, kMsgInitTitleBar, 0, 0);
            return 0;

        case kMsgInitTitleBar:
            if (InitAppWindowTitleBar(hwnd))
            {
                UpdateTitleBarMetrics(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else
            {
                MessageBoxW(hwnd, L"InitAppWindowTitleBar failed", L"Warning", MB_ICONWARNING);
            }
            return 0;

        case WM_ACTIVATE:
        case WM_NCACTIVATE:
        case WM_SIZE:
        case WM_DPICHANGED:
            UpdateTitleBarMetrics(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rcClient{};
            GetClientRect(hwnd, &rcClient);
            DrawTitleBar(hdc, rcClient);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void PrintHResultMessage(HRESULT hr)
{
    wchar_t *buffer = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD size =
        FormatMessageW(flags, nullptr, static_cast<DWORD>(hr), 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    if (size && buffer)
    {
        wprintf(L"HRESULT message: %ls\n", buffer);
        LocalFree(buffer);
    }
}

int main()
{
    init_apartment(apartment_type::single_threaded);

    const HRESULT ensureHr = WindowsAppRuntime_EnsureIsLoaded();
    if (FAILED(ensureHr))
    {
        printf("WindowsAppRuntime_EnsureIsLoaded failed: 0x%08lx\n", static_cast<unsigned long>(ensureHr));
        PrintHResultMessage(ensureHr);
        return 1;
    }
    try
    {
        auto factory = winrt::get_activation_factory<winrt::Microsoft::UI::Windowing::AppWindow,
                                                     winrt::Windows::Foundation::IActivationFactory>();

        printf("AppWindow activation factory OK\n");
    }
    catch (const winrt::hresult_error &e)
    {
        printf("AppWindow factory failed: 0x%08lx\n", static_cast<unsigned long>(e.code().value));
        auto msg = winrt::to_string(e.message());
        if (!msg.empty()) printf("Message: %s\n", msg.c_str());
    }

    const wchar_t kClassName[] = L"WinRTWindowTitleBar";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, kClassName, L"WinRT Sample", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                                CW_USEDEFAULT, 1000, 700, nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd)
    {
        MessageBoxW(nullptr, L"HWND Creation Failed (Check InitAppWindowTitleBar)", L"Error", MB_ICONERROR);
        printf("HWND Creation Failed (Check InitAppWindowTitleBar)\n");
        return 1;
    }
    if (!hwnd) return 1;

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
