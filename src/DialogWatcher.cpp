#include "DialogWatcher.h"
#include "DialogAdapter.h"

DialogWatcher* DialogWatcher::self_ = nullptr;

namespace {
constexpr UINT kRetryMs = 100;
constexpr int kMaxRetries = 30;  // ~3 s: a new dialog can get focus well before its controls exist/are visible
}

bool DialogWatcher::Start(Callback cb) {
    if (fgHook_) return true;
    cb_ = std::move(cb);
    self_ = this;
    // SKIPOWNPROCESS: our own overlay taking focus never reaches us.
    fgHook_ = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, ForegroundProc, 0, 0,
                              WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (!fgHook_) return false;
    OnForeground(GetForegroundWindow());  // a dialog may already be open
    return true;
}

void DialogWatcher::Stop() {
    CancelRetry();
    if (dlgHook_) { UnhookWinEvent(dlgHook_); dlgHook_ = nullptr; }
    if (fgHook_) { UnhookWinEvent(fgHook_); fgHook_ = nullptr; }
    current_ = nullptr;
    if (self_ == this) self_ = nullptr;
}

void CALLBACK DialogWatcher::ForegroundProc(HWINEVENTHOOK, DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD) {
    if (self_ && idObject == OBJID_WINDOW && idChild == CHILDID_SELF) self_->OnForeground(hwnd);
}

void CALLBACK DialogWatcher::DialogProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD) {
    DialogWatcher* w = self_;
    if (!w || hwnd != w->current_ || idObject != OBJID_WINDOW || idChild != CHILDID_SELF) return;
    switch (event) {
        case EVENT_OBJECT_DESTROY:
        case EVENT_OBJECT_HIDE:
            w->Drop();
            break;
        case EVENT_OBJECT_LOCATIONCHANGE:
            w->cb_(Event::Moved, hwnd);
            break;
    }
}

void CALLBACK DialogWatcher::RetryTimer(HWND, UINT, UINT_PTR, DWORD) {
    DialogWatcher* w = self_;
    if (!w) return;
    HWND p = w->pending_;
    if (!p || !IsWindow(p) || GetForegroundWindow() != p || ++w->retries_ > kMaxRetries) {
        w->CancelRetry();
    } else if (DialogAdapter::IsSupported(p)) {
        w->CancelRetry();
        w->Track(p);
    }
}

void DialogWatcher::OnForeground(HWND hwnd) {
    if (!hwnd || hwnd == current_) return;
    CancelRetry();
    if (current_) Drop();  // focus moved to something else
    if (DialogAdapter::IsSupported(hwnd)) {
        Track(hwnd);
        return;
    }
    if (DialogAdapter::IsCandidateClass(hwnd)) StartRetry(hwnd);
    cb_(Event::Other, hwnd);
}

void DialogWatcher::Track(HWND dlg) {
    DWORD pid = 0;
    const DWORD tid = GetWindowThreadProcessId(dlg, &pid);
    current_ = dlg;
    dlgHook_ = SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_LOCATIONCHANGE, nullptr, DialogProc, pid, tid,
                               WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    cb_(Event::Active, dlg);
}

void DialogWatcher::Drop() {
    HWND old = current_;
    if (dlgHook_) { UnhookWinEvent(dlgHook_); dlgHook_ = nullptr; }
    current_ = nullptr;
    if (old) cb_(Event::Lost, old);
}

void DialogWatcher::StartRetry(HWND hwnd) {
    pending_ = hwnd;
    retries_ = 0;
    retryTimer_ = SetTimer(nullptr, 0, kRetryMs, RetryTimer);
}

void DialogWatcher::CancelRetry() {
    if (retryTimer_) { KillTimer(nullptr, retryTimer_); retryTimer_ = 0; }
    pending_ = nullptr;
}
