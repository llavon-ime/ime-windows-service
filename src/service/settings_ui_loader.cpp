#include "settings_ui_loader.hpp"

#include <filesystem>
#include <string>

namespace llavon::service {
namespace {

constexpr wchar_t settings_ui_filename[] = L"llavon-ime-settings-ui.dll";

std::filesystem::path executable_directory() {
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD copied =
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (copied == 0) {
            return {};
        }
        if (copied < buffer.size() - 1) {
            buffer.resize(copied);
            return std::filesystem::path(buffer).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}

template <typename Function>
Function resolve(HMODULE module, const char* name) {
    return reinterpret_cast<Function>(GetProcAddress(module, name));
}

}  // namespace

SettingsUiLoader::~SettingsUiLoader() {
    if (!module_) {
        return;
    }

    bool can_unload = true;
    if (started_ && stop_) {
        can_unload = stop_() == 0;
    }
    if (can_unload) {
        FreeLibrary(module_);
    }
}

bool SettingsUiLoader::show() {
    if (!load()) {
        report_error(L"The settings UI module could not be loaded.");
        return false;
    }
    if (!started_) {
        const std::int32_t result = start_();
        if (result != 0) {
            report_error(L"The settings UI thread could not be started.");
            return false;
        }
        started_ = true;
    }

    show_();
    return true;
}

bool SettingsUiLoader::load() {
    if (module_) {
        return true;
    }

    const std::filesystem::path directory = executable_directory();
    if (directory.empty()) {
        return false;
    }
    const std::filesystem::path module_path = directory / settings_ui_filename;
    module_ = LoadLibraryExW(module_path.c_str(), nullptr,
                             LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module_) {
        return false;
    }

    start_ = resolve<StartFunction>(module_, "llavon_settings_ui_start");
    show_ = resolve<ShowFunction>(module_, "llavon_settings_ui_show");
    stop_ = resolve<StopFunction>(module_, "llavon_settings_ui_stop");
    if (!start_ || !show_ || !stop_) {
        FreeLibrary(module_);
        module_ = nullptr;
        start_ = nullptr;
        show_ = nullptr;
        stop_ = nullptr;
        return false;
    }
    return true;
}

void SettingsUiLoader::report_error(const wchar_t* detail) const noexcept {
    const DWORD error = GetLastError();
    std::wstring message(detail);
    if (error != ERROR_SUCCESS) {
        message += L"\n\nWindows error: ";
        message += std::to_wstring(error);
    }
    MessageBoxW(nullptr, message.c_str(), L"Llavon IME", MB_OK | MB_ICONERROR);
}

}  // namespace llavon::service
