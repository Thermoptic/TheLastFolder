// The Last Folder: application shell. Hidden window + tray icon + dialog watcher + file manager tracking.
//
// Behavior: remember the folder of the file manager (Explorer / Total Commander / Double Commander) the user used last.
// When the user comes back to a standard Open / Save As dialog of another program, jump it to that folder.
// No UI besides the tray icon and its menu (Run at Startup, About, Exit).
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>  // CoInitializeEx; not pulled in by WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <string>
#include <vector>
#include "Autostart.h"
#include "DialogAdapter.h"
#include "DialogWatcher.h"
#include "FileManagers.h"
#include "TrayIcon.h"

namespace {

constexpr wchar_t kWindowClass[] = L"TheLastFolderWnd";
constexpr wchar_t kAppName[] = L"The Last Folder";
constexpr wchar_t kVersion[] = L"0.4.0 (prototype)";

enum : UINT { WM_TRAYICON = WM_APP + 1 };
enum : UINT { ID_ABOUT = 1001, ID_EXIT = 1002, ID_STARTUP = 1003 };
constexpr UINT_PTR kAutoNavTimer = 1;
constexpr UINT_PTR kVerifyTimer = 2;
constexpr UINT kAutoNavDelayMs = 250;  // let a freshly shown dialog finish initializing
constexpr UINT kVerifyIntervalMs = 100;
constexpr int kVerifyMaxTicks = 20;    // ~2 s

// Built every time the menu is shown, so the check mark always reflects what the registry says right now.
void ShowTrayMenu(TrayIcon& tray, POINT pt) {
    const MenuItem items[] = {
        {ID_STARTUP, L"Run at Startup", Autostart::IsEnabled()},
        {0, nullptr},  // separator
        {ID_ABOUT, L"About"},
        {ID_EXIT, L"Exit"},
    };
    tray.ShowMenu(pt, items, ARRAYSIZE(items));
}

struct App {
    HWND hwnd = nullptr;
    TrayIcon tray;
    DialogWatcher watcher;
    FileManagers fm;
    UINT taskbarCreated = 0;
    bool aboutOpen = false;

    // A dialog opens as usual and is never jumped the first time it is seen. It only jumps when the user comes
    // back to it after having used a file manager: `serial` is FileManagers::Serial() as of the last time that
    // dialog was handled, so "serial grew" means "a file manager was left since".
    struct SeenDialog { HWND hwnd; unsigned serial; };
    std::vector<SeenDialog> seen;

    // Verification of a jump in progress.
    HWND navDialog = nullptr;
    std::wstring navTarget;
    std::wstring savedFileName;  // Save As: the filename box is overwritten by navigation and restored after
    int verifyTicks = 0;
};

void KillTimers(App& app) {
    KillTimer(app.hwnd, kAutoNavTimer);
    KillTimer(app.hwnd, kVerifyTimer);
}

// Ends a jump in progress (confirmed, timed out, or the dialog lost focus): put the user's file name back.
void FinishNav(App& app) {
    KillTimer(app.hwnd, kVerifyTimer);
    if (app.navDialog && IsWindow(app.navDialog)) DialogAdapter::SetFileName(app.navDialog, app.savedFileName);
    app.navDialog = nullptr;
}

// The record for `dlg`; null if this dialog has not been seen before.
App::SeenDialog* FindSeen(App& app, HWND dlg) {
    for (auto& s : app.seen)
        if (s.hwnd == dlg) return &s;
    return nullptr;
}

void OnDialogEvent(App& app, DialogWatcher::Event e, HWND dlg) {
    switch (e) {
        case DialogWatcher::Event::Active: {
            // Coming from a file manager straight to the dialog: this also records what the file manager showed.
            app.fm.NoteForeground(dlg);

            // Forget dialogs that no longer exist.
            app.seen.erase(std::remove_if(app.seen.begin(), app.seen.end(),
                                          [](const App::SeenDialog& s) { return !IsWindow(s.hwnd); }),
                           app.seen.end());

            App::SeenDialog* s = FindSeen(app, dlg);
            if (!s) {  // first time we see this dialog: it opens as usual, in the folder the program chose
                app.seen.push_back({dlg, app.fm.Serial()});
                return;
            }
            if (app.fm.Serial() == s->serial) return;  // no file manager used since: leave the dialog alone
            DWORD pid = 0;
            GetWindowThreadProcessId(dlg, &pid);
            if (pid == app.fm.LastPid()) return;  // a dialog of the file manager itself
            SetTimer(app.hwnd, kAutoNavTimer, kAutoNavDelayMs, nullptr);
            break;
        }
        case DialogWatcher::Event::Lost:
            KillTimer(app.hwnd, kAutoNavTimer);
            FinishNav(app);
            break;
        case DialogWatcher::Event::Other:
            app.fm.NoteForeground(dlg);  // here `dlg` is the new foreground window
            break;
        case DialogWatcher::Event::Moved:
            break;
    }
}

void OnAutoNav(App& app) {
    KillTimer(app.hwnd, kAutoNavTimer);
    HWND dlg = app.watcher.Current();
    if (!dlg) return;
    if (App::SeenDialog* s = FindSeen(app, dlg)) s->serial = app.fm.Serial();  // handled, whatever happens next

    const std::wstring folder = app.fm.Remembered();
    if (folder.empty()) return;  // unknown: do nothing rather than guess
    if (DialogAdapter::SamePath(DialogAdapter::CurrentFolder(dlg), folder)) return;

    app.navDialog = dlg;
    app.savedFileName = DialogAdapter::GetFileName(dlg);
    if (!DialogAdapter::Navigate(dlg, folder)) {
        FinishNav(app);
        return;
    }
    // Confirm without blocking the UI thread: check every 100 ms for up to 2 s.
    app.navTarget = folder;
    app.verifyTicks = 0;
    SetTimer(app.hwnd, kVerifyTimer, kVerifyIntervalMs, nullptr);
}

void OnVerifyTick(App& app) {
    HWND dlg = app.watcher.Current();
    if (!dlg || dlg != app.navDialog) { FinishNav(app); return; }
    const bool done = DialogAdapter::SamePath(DialogAdapter::CurrentFolder(dlg), app.navTarget);
    if (done || ++app.verifyTicks >= kVerifyMaxTicks) FinishNav(app);  // also on failure: do not leave our path behind
}

void ShowAbout(HWND hwnd, App& app) {
    if (app.aboutOpen) return;
    app.aboutOpen = true;
    wchar_t text[160];
    wsprintfW(text, L"%s\nVersion %s", kAppName, kVersion);
    MessageBoxW(hwnd, text, L"About", MB_OK | MB_ICONINFORMATION);
    app.aboutOpen = false;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    if (!app) return DefWindowProcW(hwnd, msg, wp, lp);

    if (msg == app->taskbarCreated) {  // Explorer restarted: tray icons are gone, add ours again
        app->tray.Readd();
        return 0;
    }

    switch (msg) {
        case WM_TRAYICON:
            if (LOWORD(lp) == WM_CONTEXTMENU)  // NOTIFYICON_VERSION_4: wp = anchor point in screen coords
                ShowTrayMenu(app->tray, {GET_X_LPARAM(wp), GET_Y_LPARAM(wp)});
            return 0;
        case WM_TIMER:
            if (wp == kAutoNavTimer) OnAutoNav(*app);
            else if (wp == kVerifyTimer) OnVerifyTick(*app);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wp) == ID_ABOUT) ShowAbout(hwnd, *app);
            else if (LOWORD(wp) == ID_STARTUP) Autostart::SetEnabled(!Autostart::IsEnabled());  // a failure leaves the check unchanged
            else if (LOWORD(wp) == ID_EXIT) DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            KillTimers(*app);
            app->watcher.Stop();
            app->fm.Stop();
            app->tray.Remove();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

struct ComScope {  // declared before App so it is released last
    bool ok;
    ComScope() : ok(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {}
    ~ComScope() { if (ok) CoUninitialize(); }
};

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    ComScope com;
    if (!com.ok) return 1;
    App app;
    app.taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&wc)) return 1;

    // A normal (never shown) top-level window, not HWND_MESSAGE: message-only windows
    // do not receive the broadcast "TaskbarCreated" message.
    app.hwnd = CreateWindowExW(0, kWindowClass, kAppName, WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, inst, &app);
    if (!app.hwnd) return 1;

    if (!app.tray.Add(app.hwnd, WM_TRAYICON, kAppName) ||
        !app.watcher.Start([&app](DialogWatcher::Event e, HWND h) { OnDialogEvent(app, e, h); })) {
        DestroyWindow(app.hwnd);  // WM_DESTROY releases whatever was set up
        return 1;
    }

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return static_cast<int>(m.wParam);
}
