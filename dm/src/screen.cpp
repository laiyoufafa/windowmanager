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

#include "screen.h"

#include "display_manager_adapter.h"
#include "screen_group.h"
#include "screen_info.h"
#include "window_manager_hilog.h"

namespace OHOS::Rosen {
namespace {
    constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0, "Screen"};
}
class Screen::Impl : public RefBase {
friend class Screen;
public:
    Impl() = default;
    ~Impl() = default;
    void UpdateScreen(sptr<ScreenInfo> info);

    ScreenId id_ { SCREEN_ID_INVALID };
    uint32_t virtualWidth_ { 0 };
    uint32_t virtualHeight_ { 0 };
    float virtualPixelRatio_ { 0.0 };
    ScreenId parent_ { SCREEN_ID_INVALID };
    bool canHasChild_ { false };
    uint32_t modeId_ { 0 };
    std::vector<sptr<SupportedScreenModes>> modes_ {};
    Rotation rotation_ { Rotation::ROTATION_0 };
};

void Screen::Impl::UpdateScreen(sptr<ScreenInfo> info)
{
    if (info == nullptr) {
        WLOGFE("info is nullptr.");
        return;
    }
    virtualWidth_ = info->virtualWidth_;
    virtualHeight_ = info->virtualHeight_;
    virtualPixelRatio_ = info->virtualPixelRatio_;
    parent_ = info->parent_;
    canHasChild_ = info->canHasChild_;
    modeId_ = info->modeId_;
    modes_ = info->modes_;
    rotation_ = info->rotation_;
}

Screen::Screen(const ScreenInfo* info)
    : pImpl_(new Impl())
{
    if (info == nullptr) {
        WLOGFE("info is nullptr.");
        return;
    }
    pImpl_->id_ = info->id_;
    pImpl_->virtualWidth_ = info->virtualWidth_;
    pImpl_->virtualHeight_ = info->virtualHeight_;
    pImpl_->virtualPixelRatio_ = info->virtualPixelRatio_;
    pImpl_->parent_ = info->parent_;
    pImpl_->canHasChild_ = info->canHasChild_;
    pImpl_->modeId_ = info->modeId_;
    pImpl_->modes_ = info->modes_;
    pImpl_->rotation_ = info->rotation_;
}

Screen::~Screen()
{
}

bool Screen::IsGroup() const
{
    sptr<ScreenInfo> info = SingletonContainer::Get<DisplayManagerAdapter>().GetScreenInfo(pImpl_->id_);
    pImpl_->UpdateScreen(info);
    return pImpl_->canHasChild_;
}

ScreenId Screen::GetId() const
{
    return pImpl_->id_;
}

uint32_t Screen::GetWidth() const
{
    sptr<ScreenInfo> info = SingletonContainer::Get<DisplayManagerAdapter>().GetScreenInfo(pImpl_->id_);
    pImpl_->UpdateScreen(info);
    auto modeId = pImpl_->modeId_;
    auto modes = pImpl_->modes_;
    if (modeId < 0 || modeId >= modes.size()) {
        return 0;
    }
    return modes[modeId]->width_;
}

uint32_t Screen::GetHeight() const
{
    sptr<ScreenInfo> info = SingletonContainer::Get<DisplayManagerAdapter>().GetScreenInfo(pImpl_->id_);
    pImpl_->UpdateScreen(info);
    auto modeId = pImpl_->modeId_;
    auto modes = pImpl_->modes_;
    if (modeId < 0 || modeId >= modes.size()) {
        return 0;
    }
    return modes[modeId]->height_;
}

uint32_t Screen::GetVirtualWidth() const
{
    sptr<ScreenInfo> info = SingletonContainer::Get<DisplayManagerAdapter>().GetScreenInfo(pImpl_->id_);
    pImpl_->UpdateScreen(info);
    return pImpl_->virtualWidth_;
}

uint32_t Screen::GetVirtualHeight() const
{
    sptr<ScreenInfo> info = SingletonContainer::Get<DisplayManagerAdapter>().GetScreenInfo(pImpl_->id_);
    pImpl_->UpdateScreen(info);
    return pImpl_->virtualHeight_;
}

float Screen::GetVirtualPixelRatio() const
{
    sptr<ScreenInfo> info = SingletonContainer::Get<DisplayManagerAdapter>().GetScreenInfo(pImpl_->id_);
    pImpl_->UpdateScreen(info);
    return pImpl_->virtualPixelRatio_;
}

Rotation Screen::GetRotation()
{
    sptr<ScreenInfo> info = SingletonContainer::Get<DisplayManagerAdapter>().GetScreenInfo(pImpl_->id_);
    pImpl_->UpdateScreen(info);
    return pImpl_->rotation_;
}

bool Screen::RequestRotation(Rotation rotation)
{
    WLOGFD("rotation the screen");
    return SingletonContainer::Get<DisplayManagerAdapter>().RequestRotation(pImpl_->id_, rotation);
}

DMError Screen::GetScreenSupportedColorGamuts(std::vector<ScreenColorGamut>& colorGamuts) const
{
    return SingletonContainer::Get<DisplayManagerAdapter>().GetScreenSupportedColorGamuts(pImpl_->id_, colorGamuts);
}

DMError Screen::GetScreenColorGamut(ScreenColorGamut& colorGamut) const
{
    return SingletonContainer::Get<DisplayManagerAdapter>().GetScreenColorGamut(pImpl_->id_, colorGamut);
}

DMError Screen::SetScreenColorGamut(int32_t colorGamutIdx)
{
    return SingletonContainer::Get<DisplayManagerAdapter>().SetScreenColorGamut(pImpl_->id_, colorGamutIdx);
}

DMError Screen::GetScreenGamutMap(ScreenGamutMap& gamutMap) const
{
    return SingletonContainer::Get<DisplayManagerAdapter>().GetScreenGamutMap(pImpl_->id_, gamutMap);
}

DMError Screen::SetScreenGamutMap(ScreenGamutMap gamutMap)
{
    return SingletonContainer::Get<DisplayManagerAdapter>().SetScreenGamutMap(pImpl_->id_, gamutMap);
}

DMError Screen::SetScreenColorTransform()
{
    return SingletonContainer::Get<DisplayManagerAdapter>().SetScreenColorTransform(pImpl_->id_);
}

ScreenId Screen::GetParentId() const
{
    return pImpl_->parent_;
}

uint32_t Screen::GetModeId() const
{
    return pImpl_->modeId_;
}

std::vector<sptr<SupportedScreenModes>> Screen::GetSupportedModes() const
{
    return pImpl_->modes_;
}

bool Screen::SetScreenActiveMode(uint32_t modeId)
{
    ScreenId screenId = pImpl_->id_;
    if (modeId < 0 || modeId >= pImpl_->modes_.size()) {
        return false;
    }
    if (SingletonContainer::Get<DisplayManagerAdapter>().SetScreenActiveMode(screenId, modeId)) {
        pImpl_->modeId_ = modeId;
        return true;
    }
    return false;
}
} // namespace OHOS::Rosen