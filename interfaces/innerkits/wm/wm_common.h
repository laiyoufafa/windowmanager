/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_ROSEN_WM_COMMON_H
#define OHOS_ROSEN_WM_COMMON_H

#include <cstdint>

namespace OHOS {
namespace Rosen {
enum class WindowType : uint32_t {
    APP_WINDOW_BASE = 1,
    APP_MAIN_WINDOW_BASE = APP_WINDOW_BASE,
    WINDOW_TYPE_APP_MAIN_WINDOW = APP_MAIN_WINDOW_BASE,
    APP_MAIN_WINDOW_END = WINDOW_TYPE_APP_MAIN_WINDOW, // equals last window type

    APP_SUB_WINDOW_BASE = 1000,
    WINDOW_TYPE_MEDIA = APP_SUB_WINDOW_BASE,
    WINDOW_TYPE_APP_SUB_WINDOW,
    APP_SUB_WINDOW_END = WINDOW_TYPE_APP_SUB_WINDOW, // equals last window type
    APP_WINDOW_END = APP_SUB_WINDOW_END,

    SYSTEM_WINDOW_BASE = 2000,
    BELOW_APP_SYSTEM_WINDOW_BASE = SYSTEM_WINDOW_BASE,
    WINDOW_TYPE_WALLPAPER = SYSTEM_WINDOW_BASE,
    BELOW_APP_SYSTEM_WINDOW_END = WINDOW_TYPE_WALLPAPER, // equals last window type

    ABOVE_APP_SYSTEM_WINDOW_BASE = 2100,
    WINDOW_TYPE_APP_LAUNCHING = ABOVE_APP_SYSTEM_WINDOW_BASE,
    WINDOW_TYPE_DOCK_SLICE,
    WINDOW_TYPE_INCOMING_CALL,
    WINDOW_TYPE_SEARCHING_BAR,
    WINDOW_TYPE_SYSTEM_ALARM_WINDOW,
    WINDOW_TYPE_INPUT_METHOD_FLOAT,
    WINDOW_TYPE_FLOAT,
    WINDOW_TYPE_TOAST,
    WINDOW_TYPE_STATUS_BAR,
    WINDOW_TYPE_PANEL,
    WINDOW_TYPE_KEYGUARD,
    WINDOW_TYPE_VOLUME_OVERLAY,
    WINDOW_TYPE_NAVIGATION_BAR,
    WINDOW_TYPE_DRAGGING_EFFECT,
    WINDOW_TYPE_POINTER,
    ABOVE_APP_SYSTEM_WINDOW_END = WINDOW_TYPE_POINTER, // equals last window type
    SYSTEM_WINDOW_END = ABOVE_APP_SYSTEM_WINDOW_END,
};

enum class WindowMode : uint32_t {
    WINDOW_MODE_FULLSCREEN,
    WINDOW_MODE_SPLIT_PRIMARY,
    WINDOW_MODE_SPLIT_SECONDARY,
    WINDOW_MODE_FLOATING,
    WINDOW_MODE_PIP
};

enum class WMError : int32_t {
    WM_OK = 0,
    WM_ERROR_SAMGR = 100,
    WM_ERROR_IPC_FAILED = 101,
    WM_ERROR_NO_MEM = 110,
    WM_ERROR_NULLPTR = 120,
    WM_ERROR_INVALID_PARAM = 130,
    WM_ERROR_DESTROYED_OBJECT = 140,
    WM_ERROR_DEATH_RECIPIENT = 150,
    WM_ERROR_INVALID_WINDOW = 160,
    WM_ERROR_INVALID_OPERATION = 170,
    WM_ERROR_UNKNOWN,

    /* weston adater */
    WM_ERROR_WINDOWMANAGER_GETINSTANCE = 100000,
    WM_ERROR_WINDOWMANAGER_INIT = 100001,
    WM_ERROR_WINDOWMANAGER_SHOW = 100002,
    WM_ERROR_WINDOWMANAGER_HIDE = 100003,
    WM_ERROR_WINDOWMANAGER_MOVE = 100004,
    WM_ERROR_WINDOWMANAGER_RESIZE = 100005,
};

enum class WindowFlag : uint32_t {
    WINDOW_FLAG_NEED_AVOID = 1,
    WINDOW_FLAG_PARENT_LIMIT = 1 << 1,
    WINDOW_FLAG_END = 1 << 2,
};

namespace {
    constexpr uint32_t SYSTEM_COLOR_WHITE = 0xE5FFFFFF;
    constexpr uint32_t SYSTEM_COLOR_BLACK = 0x66000000;
}

struct SystemBarProperty {
    bool enable_;
    uint32_t backgroundColor_;
    uint32_t contentColor_;
    SystemBarProperty() : enable_(true), backgroundColor_(SYSTEM_COLOR_WHITE), contentColor_(SYSTEM_COLOR_BLACK) {}
    SystemBarProperty(bool enable, uint32_t background, uint32_t content)
        : enable_(enable), backgroundColor_(background), contentColor_(content) {}
    bool operator == (const SystemBarProperty& a) const
    {
        return (enable_ == a.enable_ && backgroundColor_ == a.backgroundColor_ && contentColor_ == a.contentColor_);
    }
};

struct Rect {
    int32_t posX_;
    int32_t posY_;
    uint32_t width_;
    uint32_t height_;
};
}
}
#endif // OHOS_ROSEN_WM_COMMON_H
