// Test host: opens a real file dialog in its own process so the PoC can drive it.
// Usage: dialoghost.exe legacy-open | legacy-save | cid-open | cid-save | cid-folder
#include <windows.h>
#include <shobjidl.h>
#include <string>

// Test harness only: a background process cannot normally take the foreground, which a real dialog the user
// is looking at would have. After the dialog has loaded, raise it once (AttachThreadInput workaround).
static BOOL CALLBACK RaiseEnum(HWND h, LPARAM) {
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    wchar_t cls[32]{};
    GetClassNameW(h, cls, 32);
    if (pid == GetCurrentProcessId() && IsWindowVisible(h) && std::wstring(cls) == L"#32770") {
        const DWORD fg = GetWindowThreadProcessId(GetForegroundWindow(), nullptr), me = GetCurrentThreadId();
        AttachThreadInput(me, fg, TRUE);
        BringWindowToTop(h);
        SetForegroundWindow(h);
        AttachThreadInput(me, fg, FALSE);
        return FALSE;
    }
    return TRUE;
}
// Raises the dialog once after it has loaded, then again every time the named event "Local\tlf_raise_dlg" is set
// (lets a test script play "the user comes back to the dialog").
static DWORD WINAPI Raiser(LPVOID) {
    HANDLE ev = CreateEventW(nullptr, FALSE, FALSE, L"Local\\tlf_raise_dlg");
    Sleep(1500);
    EnumWindows(RaiseEnum, 0);
    while (ev && WaitForSingleObject(ev, INFINITE) == WAIT_OBJECT_0) EnumWindows(RaiseEnum, 0);
    return 0;
}

static void Legacy(bool save) {
    wchar_t file[MAX_PATH] = L"Untitled.txt";  // pre-filled name, like Notepad's Save As
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = L"C:\\Windows";
    ofn.lpstrFilter = L"All files\0*.*\0";
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;
    save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
}

static wchar_t g_resultFile[MAX_PATH] = L"";  // optional 3rd command-line argument, see Cid()

static void Cid(bool save, bool folder) {
    IFileDialog* dlg = nullptr;
    HRESULT hr = save ? CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dlg))
                      : CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dlg));
    if (FAILED(hr)) return;
    if (folder) { DWORD o; dlg->GetOptions(&o); dlg->SetOptions(o | FOS_PICKFOLDERS); }
    if (save) dlg->SetFileName(L"Untitled.txt");
    IShellItem* si = nullptr;
    if (SUCCEEDED(SHCreateItemFromParsingName(L"C:\\Windows", nullptr, IID_PPV_ARGS(&si)))) {
        dlg->SetFolder(si);
        si->Release();
    }
    const HRESULT shown = dlg->Show(nullptr);
    // Test aid: when a result file is given as the 3rd argument, write what the dialog returned (the folder or file the
    // user finally chose), or "CANCELLED". Lets a test prove what "Select Folder" / "Open" really selected.
    if (g_resultFile[0]) {
        std::wstring result = L"CANCELLED";
        IShellItem* item = nullptr;
        if (SUCCEEDED(shown) && SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) { result = path; CoTaskMemFree(path); }
            item->Release();
        }
        HANDLE f = CreateFileW(g_resultFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            const int n = WideCharToMultiByte(CP_UTF8, 0, result.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string utf8(static_cast<size_t>(n), '\0');
            WideCharToMultiByte(CP_UTF8, 0, result.c_str(), -1, utf8.data(), n, nullptr, nullptr);
            DWORD written = 0;
            WriteFile(f, utf8.c_str(), static_cast<DWORD>(utf8.size() - 1), &written, nullptr);
            CloseHandle(f);
        }
    }
    dlg->Release();
}

int wmain(int argc, wchar_t** argv) {
    std::wstring mode = argc > 1 ? argv[1] : L"legacy-open";
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (argc > 3) wcsncpy_s(g_resultFile, argv[3], _TRUNCATE);
    if (argc > 2 && std::wstring(argv[2]) == L"raise") CloseHandle(CreateThread(nullptr, 0, Raiser, nullptr, 0, nullptr));
    if (mode == L"legacy-open") Legacy(false);
    else if (mode == L"legacy-save") Legacy(true);
    else if (mode == L"cid-open") Cid(false, false);
    else if (mode == L"cid-save") Cid(true, false);
    else if (mode == L"cid-folder") Cid(false, true);
    CoUninitialize();
    return 0;
}
