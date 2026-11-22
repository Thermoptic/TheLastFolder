// Phase 0 helper: find a visible #32770 dialog (optionally of a given PID) and dump its UIA tree.
// Usage: uiadump.exe [hwnd-hex]
#include <windows.h>
#include <uiautomation.h>
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#include <string>

static IUIAutomation* g_uia;

static void Dump(IUIAutomationTreeWalker* w, IUIAutomationElement* e, int depth) {
    if (depth > 14) return;
    BSTR name = nullptr, cls = nullptr, aid = nullptr;
    CONTROLTYPEID ct = 0;
    e->get_CurrentName(&name);
    e->get_CurrentClassName(&cls);
    e->get_CurrentAutomationId(&aid);
    e->get_CurrentControlType(&ct);
    std::wstring n = name ? name : L"";
    if (n.size() > 70) n = n.substr(0, 70) + L"...";
    wprintf(L"%*s[%d] class=%s aid=%s name=\"%s\"\n", depth * 2, L"", (int)ct, cls ? cls : L"", aid ? aid : L"", n.c_str());
    SysFreeString(name); SysFreeString(cls); SysFreeString(aid);
    IUIAutomationElement* c = nullptr;
    w->GetFirstChildElement(e, &c);
    while (c) {
        Dump(w, c, depth + 1);
        IUIAutomationElement* next = nullptr;
        w->GetNextSiblingElement(c, &next);
        c->Release();
        c = next;
    }
}

static HWND g_found;
static BOOL CALLBACK Enum(HWND h, LPARAM) {
    wchar_t cls[64];
    GetClassNameW(h, cls, 64);
    if (IsWindowVisible(h) && wcscmp(cls, L"#32770") == 0 && GetDlgItem(h, 1148)) { g_found = h; return FALSE; }
    return TRUE;
}

int wmain(int argc, wchar_t** argv) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HWND h = argc > 1 ? (HWND)(UINT_PTR)wcstoull(argv[1], nullptr, 16) : nullptr;
    if (!h) { EnumWindows(Enum, 0); h = g_found; }
    if (!h) { wprintf(L"no dialog found\n"); return 1; }
    wprintf(L"hwnd=0x%p\n", h);
    CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_uia));
    IUIAutomationElement* root = nullptr;
    if (FAILED(g_uia->ElementFromHandle(h, &root)) || !root) { wprintf(L"ElementFromHandle failed\n"); return 2; }
    IUIAutomationTreeWalker* w = nullptr;
    g_uia->get_RawViewWalker(&w);
    Dump(w, root, 0);
    w->Release(); root->Release(); g_uia->Release();
    CoUninitialize();
    return 0;
}
