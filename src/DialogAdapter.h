// DialogAdapter: recognises a standard Open / Save As dialog of another process,
// reads its current folder and navigates it. Plain Win32 messaging only; see docs/DESIGN_NOTE.md.
#pragma once
#include <windows.h>
#include <string>

namespace DialogAdapter {

// True for a class "#32770" window (generic dialog class, used by many windows).
bool IsCandidateClass(HWND h);

// True for a visible Open / Save As dialog of another process.
// The folder picker is deliberately NOT supported (path + OK would select and close it).
bool IsSupported(HWND dlg);

// Current folder as shown in the dialog's address bar, or empty if it cannot be read.
std::wstring CurrentFolder(HWND dlg);

// Sets the filename box to `folder` and presses the default button. Returns false if the messages
// could not be delivered (hung host, elevated host). Does not verify; compare with CurrentFolder.
bool Navigate(HWND dlg, const std::wstring& folder);

// The text in the dialog's filename box. Navigate() overwrites it, so callers that care (Save As)
// read it first and put it back with SetFileName once the dialog is in the new folder.
std::wstring GetFileName(HWND dlg);
bool SetFileName(HWND dlg, const std::wstring& text);

bool SamePath(const std::wstring& a, const std::wstring& b);

}  // namespace DialogAdapter
