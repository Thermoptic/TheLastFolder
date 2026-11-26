#include "Settings.h"
#include <vector>

namespace {

constexpr wchar_t kKey[] = L"Software\\TheLastFolder";
constexpr wchar_t kEnabledName[] = L"CustomIconEnabled";  // REG_DWORD, 0 or 1
constexpr wchar_t kPathName[] = L"CustomIconPath";        // REG_SZ

}  // namespace

namespace Settings {

CustomIcon LoadCustomIcon() {
    CustomIcon out;
    DWORD enabled = 0, size = sizeof(enabled);
    if (RegGetValueW(HKEY_CURRENT_USER, kKey, kEnabledName, RRF_RT_REG_DWORD, nullptr, &enabled, &size) == ERROR_SUCCESS)
        out.enabled = enabled != 0;

    DWORD bytes = 0;  // ask for the length first: a path can be longer than MAX_PATH
    if (RegGetValueW(HKEY_CURRENT_USER, kKey, kPathName, RRF_RT_REG_SZ, nullptr, nullptr, &bytes) == ERROR_SUCCESS &&
        bytes >= sizeof(wchar_t)) {
        std::vector<wchar_t> buf(bytes / sizeof(wchar_t) + 1, L'\0');
        DWORD got = bytes;
        if (RegGetValueW(HKEY_CURRENT_USER, kKey, kPathName, RRF_RT_REG_SZ, nullptr, buf.data(), &got) == ERROR_SUCCESS)
            out.path = buf.data();
    }
    return out;
}

bool SaveCustomIcon(const CustomIcon& value) {
    const DWORD enabled = value.enabled ? 1 : 0;
    if (RegSetKeyValueW(HKEY_CURRENT_USER, kKey, kEnabledName, REG_DWORD, &enabled, sizeof(enabled)) != ERROR_SUCCESS)
        return false;
    return RegSetKeyValueW(HKEY_CURRENT_USER, kKey, kPathName, REG_SZ, value.path.c_str(),
                           static_cast<DWORD>((value.path.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

}  // namespace Settings
