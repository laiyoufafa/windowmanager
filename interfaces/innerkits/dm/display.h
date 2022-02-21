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
 * See the License for the specific language governing p ermissions and
 * limitations under the License.
 */

#ifndef FOUNDATION_DM_DISPLAY_H
#define FOUNDATION_DM_DISPLAY_H

#include <string>
#include "dm_common.h"

namespace OHOS::Rosen {
class DisplayInfo;

typedef enum DisplayType {
    DEFAULT = 0,
} DisplayType;

class Display : public RefBase {
public:
    Display(const std::string& name, DisplayInfo* info);
    ~Display() = default;

    DisplayId GetId() const;
    int32_t GetWidth() const;
    int32_t GetHeight() const;
    uint32_t GetFreshRate() const;
    ScreenId GetScreenId() const;
    float GetVirtualPixelRatio() const;

    void SetId(DisplayId displayId);
    void SetWidth(int32_t width);
    void SetHeight(int32_t height);
    void SetFreshRate(uint32_t freshRate);

private:
    class Impl;
    sptr<Impl> pImpl_;
};
} // namespace OHOS::Rosen

#endif // FOUNDATION_DM_DISPLAY_H