// FileManagers: remembers which file manager (Explorer or Total Commander) the user used last and can
// read the folder it is showing. Read-only; nothing is sent to those programs.
#pragma once
#include <windows.h>
#include <string>

class FileManagers {
public:
    FileManagers() = default;
    ~FileManagers() { Stop(); }
    FileManagers(const FileManagers&) = delete;
    FileManagers& operator=(const FileManagers&) = delete;

    // Call for every foreground-window change (including when a file dialog becomes active).
    // Cheap: only a class-name check for unrelated windows. When the user LEAVES a file manager, the
    // folder it shows at that moment is read and kept (see Remembered()).
    void NoteForeground(HWND hwnd);

    // Folder the most recently used file manager was showing when the user left it. Empty if unknown,
    // the location is not a local drive-letter folder, or (Total Commander) the active panel was not
    // known. Never guesses.
    const std::wstring& Remembered() const { return remembered_; }

    DWORD LastPid() const { return pid_; }
    // Increases every time the user leaves a file manager (a new Remembered() value was taken).
    unsigned Serial() const { return serial_; }

    void Stop();

private:
    enum class Kind { None, Explorer, TotalCommander };

    static void CALLBACK FocusProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
    std::wstring ReadCurrentFolder();  // live read of the tracked file manager's folder
    void EnsureTcHook(DWORD pid);
    void CaptureTcFocus(HWND tc);
    std::wstring ExplorerFolder(HWND top);
    std::wstring TotalCommanderFolder(HWND tc);

    static FileManagers* self_;
    Kind kind_ = Kind::None;
    HWND hwnd_ = nullptr;
    DWORD pid_ = 0;
    unsigned serial_ = 0;
    bool active_ = false;      // a file manager window is the foreground window right now
    std::wstring remembered_;  // folder it showed when the user last left it
    HWINEVENTHOOK tcHook_ = nullptr;
    DWORD tcHookPid_ = 0;
    HWND tcFocus_ = nullptr;  // last file-list control that had focus in Total Commander (= active panel)
};
