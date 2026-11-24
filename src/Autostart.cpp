#include "Autostart.h"
#include <string>

namespace {

constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kApprovedKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run";

// The command Windows runs at login: this exe's full path, quoted (the path may contain spaces).
std::wstring Command() {
    wchar_t path[MAX_PATH * 2]{};
    const DWORD n = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    if (n == 0 || n >= ARRAYSIZE(path)) return L"";
    return std::wstring(L"\"") + path + L"\"";
}

// Windows records "turned off in Task Manager / Settings" as the first byte of a binary value: even = enabled
// (02, 06), odd = disabled (03). No value means nothing was ever switched off.
bool DisabledByWindows(const wchar_t* name) {
    BYTE data[16]{};
    DWORD size = sizeof(data), type = 0;
    if (RegGetValueW(HKEY_CURRENT_USER, kApprovedKey, name, RRF_RT_REG_BINARY, &type, data, &size) != ERROR_SUCCESS)
        return false;
    return size >= 1 && (data[0] & 1) != 0;
}

bool DeleteValue(const wchar_t* key, const wchar_t* name) {
    const LSTATUS r = RegDeleteKeyValueW(HKEY_CURRENT_USER, key, name);
    return r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND || r == ERROR_PATH_NOT_FOUND;
}

}  // namespace

namespace Autostart {

bool IsEnabled(const wchar_t* name) {
    const std::wstring expected = Command();
    if (expected.empty()) return false;
    wchar_t value[MAX_PATH * 2 + 4]{};
    DWORD size = sizeof(value) - sizeof(wchar_t);  // keep room for a terminator
    if (RegGetValueW(HKEY_CURRENT_USER, kRunKey, name, RRF_RT_REG_SZ, nullptr, value, &size) != ERROR_SUCCESS)
        return false;
    // Registered for a different copy of the program (moved or renamed exe)? Then it is not "us" and shows as off.
    if (_wcsicmp(value, expected.c_str()) != 0) return false;
    return !DisabledByWindows(name);
}

bool SetEnabled(bool enable, const wchar_t* name) {
    if (!enable) {
        const bool a = DeleteValue(kRunKey, name);
        const bool b = DeleteValue(kApprovedKey, name);  // tidy: nothing left behind
        return a && b;
    }
    const std::wstring cmd = Command();
    if (cmd.empty()) return false;
    const LSTATUS r = RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, name, REG_SZ, cmd.c_str(),
                                      static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t)));
    if (r != ERROR_SUCCESS) return false;
    return DeleteValue(kApprovedKey, name);  // an earlier "disabled" mark would otherwise keep it from starting
}

}  // namespace Autostart
