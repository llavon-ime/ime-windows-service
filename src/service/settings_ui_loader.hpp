#pragma once

#include <windows.h>

#include <cstdint>

namespace llavon::service {

class SettingsUiLoader final {
public:
    SettingsUiLoader() = default;
    SettingsUiLoader(const SettingsUiLoader&) = delete;
    SettingsUiLoader& operator=(const SettingsUiLoader&) = delete;
    ~SettingsUiLoader();

    bool show();

private:
    using StartFunction = std::int32_t (*)();
    using ShowFunction = void (*)();
    using StopFunction = std::int32_t (*)();

    bool load();
    void report_error(const wchar_t* detail) const noexcept;

    HMODULE module_ = nullptr;
    StartFunction start_ = nullptr;
    ShowFunction show_ = nullptr;
    StopFunction stop_ = nullptr;
    bool started_ = false;
};

}  // namespace llavon::service
