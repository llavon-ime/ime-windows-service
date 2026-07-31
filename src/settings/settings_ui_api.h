#pragma once

#include <stdint.h>

#if defined(_WIN32)
#if defined(LLAVON_SETTINGS_UI_EXPORTS)
#define LLAVON_SETTINGS_UI_API __declspec(dllexport)
#else
#define LLAVON_SETTINGS_UI_API __declspec(dllimport)
#endif
#else
#define LLAVON_SETTINGS_UI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Starts the settings UI's dedicated STA thread. Calling this function more
// than once is safe. Returns zero on success.
LLAVON_SETTINGS_UI_API int32_t llavon_settings_ui_start(void);

// These operations only enqueue work for the settings UI thread and return
// immediately. They never execute XAML code on the caller's thread.
LLAVON_SETTINGS_UI_API void llavon_settings_ui_show(void);
LLAVON_SETTINGS_UI_API void llavon_settings_ui_hide(void);

// Stops and joins the dedicated UI thread. Returns zero when the thread has
// completed its XAML shutdown sequence.
LLAVON_SETTINGS_UI_API int32_t llavon_settings_ui_stop(void);

#ifdef __cplusplus
}
#endif
