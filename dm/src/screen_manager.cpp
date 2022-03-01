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
#include "screen_manager.h"
#include "display_manager_adapter.h"
#include "display_manager_agent_default.h"
#include "singleton_delegator.h"
#include "window_manager_hilog.h"

#include <map>
#include <vector>

namespace OHOS::Rosen {
namespace {
    constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, HILOG_DOMAIN_DISPLAY, "ScreenManager"};
}
class ScreenManager::Impl : public RefBase {
public:
    Impl() = default;
    ~Impl() = default;
    static inline SingletonDelegator<ScreenManager> delegator;
    bool RegisterScreenListener(sptr<IScreenListener> listener);
    bool UnregisterScreenListener(sptr<IScreenListener> listener);
    bool RegisterScreenGroupListener(sptr<IScreenGroupListener> listener);
    bool UnregisterScreenGroupListener(sptr<IScreenGroupListener> listener);
    sptr<Screen> GetScreen(ScreenId screenId);
    sptr<ScreenGroup> GetScreenGroup(ScreenId screenId);
    std::vector<sptr<Screen>> GetAllScreens();

private:
    void NotifyScreenConnect(sptr<ScreenInfo> info);
    void NotifyScreenDisconnect(ScreenId);
    void NotifyScreenChange(const sptr<ScreenInfo>& screenInfo);
    void NotifyScreenChange(const std::vector<sptr<ScreenInfo>>& screenInfos);
    bool UpdateScreenInfoLocked(sptr<ScreenInfo>);

    class ScreenManagerListener;
    sptr<ScreenManagerListener> screenManagerListener_;
    std::map<ScreenId, sptr<Screen>> screenMap_;
    std::map<ScreenId, sptr<ScreenGroup>> screenGroupMap_;
    std::recursive_mutex mutex_;
    std::vector<sptr<IScreenListener>> screenListeners_;
    std::vector<sptr<IScreenGroupListener>> screenGroupListeners_;
};

class ScreenManager::Impl::ScreenManagerListener : public DisplayManagerAgentDefault {
public:
    ScreenManagerListener(sptr<Impl> impl) : pImpl_(impl)
    {
    }

    void OnScreenConnect(sptr<ScreenInfo> screenInfo)
    {
        if (screenInfo == nullptr || screenInfo->GetScreenId() == SCREEN_ID_INVALID) {
            WLOGFE("OnScreenConnect, screenInfo is invalid.");
            return;
        }
        if (pImpl_ == nullptr) {
            WLOGFE("OnScreenConnect, impl is nullptr.");
            return;
        }
        pImpl_->NotifyScreenConnect(screenInfo);
        for (auto listener : pImpl_->screenListeners_) {
            listener->OnConnect(screenInfo->GetScreenId());
        }
    };

    void OnScreenDisconnect(ScreenId screenId)
    {
        if (screenId == SCREEN_ID_INVALID) {
            WLOGFE("OnScreenDisconnect, screenId is invalid.");
            return;
        }
        if (pImpl_ == nullptr) {
            WLOGFE("OnScreenDisconnect, impl is nullptr.");
            return;
        }
        pImpl_->NotifyScreenDisconnect(screenId);
        for (auto listener : pImpl_->screenListeners_) {
            listener->OnDisconnect(screenId);
        }
    };

    void OnScreenChange(const sptr<ScreenInfo>& screenInfo, ScreenChangeEvent event)
    {
        if (screenInfo == nullptr) {
            WLOGFE("OnScreenChange, screenInfo is null.");
            return;
        }
        if (pImpl_ == nullptr) {
            WLOGFE("OnScreenChange, impl is nullptr.");
            return;
        }
        WLOGFD("OnScreenChange. event %{public}u", event);
        pImpl_->NotifyScreenChange(screenInfo);
        for (auto listener: pImpl_->screenListeners_) {
            listener->OnChange(screenInfo->GetScreenId());
        }
    };

    void OnScreenGroupChange(const std::vector<sptr<ScreenInfo>>& screenInfos, ScreenGroupChangeEvent groupEvent)
    {
        if (screenInfos.empty()) {
            WLOGFE("OnScreenGroupChange, screenInfos is empty.");
            return;
        }
        if (pImpl_ == nullptr) {
            WLOGFE("OnScreenGroupChange, impl is nullptr.");
            return;
        }
        WLOGFD("OnScreenGroupChange. event %{public}u", groupEvent);
        pImpl_->NotifyScreenChange(screenInfos);
        std::vector<ScreenId> screenIds;
        for (auto screenInfo : screenInfos) {
            if (screenInfo->GetScreenId() != SCREEN_ID_INVALID) {
                screenIds.push_back(screenInfo->GetScreenId());
            }
        }
        for (auto listener: pImpl_->screenGroupListeners_) {
            listener->OnChange(screenIds, groupEvent);
        }
    };
private:
    sptr<Impl> pImpl_;
};
WM_IMPLEMENT_SINGLE_INSTANCE(ScreenManager)

ScreenManager::ScreenManager()
{
    pImpl_ = new Impl();
}

ScreenManager::~ScreenManager()
{
}

sptr<Screen> ScreenManager::Impl::GetScreen(ScreenId screenId)
{
    auto screenInfo = SingletonContainer::Get<ScreenManagerAdapter>().GetScreenInfo(screenId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!UpdateScreenInfoLocked(screenInfo)) {
        screenMap_.erase(screenId);
        return nullptr;
    }
    return screenMap_[screenId];
}

sptr<Screen> ScreenManager::GetScreenById(ScreenId screenId)
{
    return pImpl_->GetScreen(screenId);
}

sptr<ScreenGroup> ScreenManager::Impl::GetScreenGroup(ScreenId screenId)
{
    auto screenGroupInfo = SingletonContainer::Get<ScreenManagerAdapter>().GetScreenGroupInfoById(screenId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (screenGroupInfo == nullptr) {
        WLOGFE("screenGroupInfo is null");
        screenGroupMap_.erase(screenId);
        return nullptr;
    }
    auto iter = screenGroupMap_.find(screenId);
    if (iter != screenGroupMap_.end() && iter->second != nullptr) {
        WLOGFI("get screenGroup in screenGroup map");
        iter->second->UpdateScreenGroupInfo(screenGroupInfo);
        return iter->second;
    }
    sptr<ScreenGroup> screenGroup = new ScreenGroup(screenGroupInfo);
    screenGroupMap_[screenId] = screenGroup;
    return screenGroup;
}

sptr<ScreenGroup> ScreenManager::GetScreenGroup(ScreenId screenId)
{
    return pImpl_->GetScreenGroup(screenId);
}

std::vector<sptr<Screen>> ScreenManager::Impl::GetAllScreens()
{
    auto screenInfos = SingletonContainer::Get<ScreenManagerAdapter>().GetAllScreenInfos();
    std::vector<sptr<Screen>> screens;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto info: screenInfos) {
        if (UpdateScreenInfoLocked(info)) {
            screens.emplace_back(screenMap_[info->GetScreenId()]);
        }
    }
    screenMap_.clear();
    for (auto screen: screens) {
        screenMap_.insert(std::make_pair(screen->GetId(), screen));
    }
    return screens;
}

std::vector<sptr<Screen>> ScreenManager::GetAllScreens()
{
    return pImpl_->GetAllScreens();
}

bool ScreenManager::Impl::RegisterScreenListener(sptr<IScreenListener> listener)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    screenListeners_.push_back(listener);
    if (screenManagerListener_ == nullptr) {
        screenManagerListener_ = new ScreenManagerListener(this);
        SingletonContainer::Get<ScreenManagerAdapter>().RegisterDisplayManagerAgent(
            screenManagerListener_,
            DisplayManagerAgentType::SCREEN_EVENT_LISTENER);
    }
    return true;
}

bool ScreenManager::RegisterScreenListener(sptr<IScreenListener> listener)
{
    if (listener == nullptr) {
        WLOGFE("RegisterScreenListener listener is nullptr.");
        return false;
    }
    return pImpl_->RegisterScreenListener(listener);
}

bool ScreenManager::Impl::UnregisterScreenListener(sptr<IScreenListener> listener)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto iter = std::find(screenListeners_.begin(), screenListeners_.end(), listener);
    if (iter == screenListeners_.end()) {
        WLOGFE("could not find this listener");
        return false;
    }
    screenListeners_.erase(iter);
    if (screenListeners_.empty() && screenManagerListener_ != nullptr) {
        SingletonContainer::Get<ScreenManagerAdapter>().UnregisterDisplayManagerAgent(
            screenManagerListener_,
            DisplayManagerAgentType::SCREEN_EVENT_LISTENER);
        screenManagerListener_ = nullptr;
    }
    return true;
}

bool ScreenManager::UnregisterScreenListener(sptr<IScreenListener> listener)
{
    if (listener == nullptr) {
        WLOGFE("UnregisterScreenListener listener is nullptr.");
        return false;
    }
    return pImpl_->UnregisterScreenListener(listener);
}

bool ScreenManager::Impl::RegisterScreenGroupListener(sptr<IScreenGroupListener> listener)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    screenGroupListeners_.push_back(listener);
    if (screenManagerListener_ == nullptr) {
        screenManagerListener_ = new ScreenManagerListener(this);
        SingletonContainer::Get<ScreenManagerAdapter>().RegisterDisplayManagerAgent(
            screenManagerListener_,
            DisplayManagerAgentType::SCREEN_EVENT_LISTENER);
    }
    return true;
}

bool ScreenManager::RegisterScreenGroupListener(sptr<IScreenGroupListener> listener)
{
    if (listener == nullptr) {
        WLOGFE("RegisterScreenGroupListener listener is nullptr.");
        return false;
    }
    return pImpl_->RegisterScreenGroupListener(listener);
}

bool ScreenManager::Impl::UnregisterScreenGroupListener(sptr<IScreenGroupListener> listener)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto iter = std::find(screenGroupListeners_.begin(), screenGroupListeners_.end(), listener);
    if (iter == screenGroupListeners_.end()) {
        WLOGFE("could not find this listener");
        return false;
    }
    screenGroupListeners_.erase(iter);
    if (screenGroupListeners_.empty() && screenManagerListener_ != nullptr) {
        SingletonContainer::Get<ScreenManagerAdapter>().UnregisterDisplayManagerAgent(
            screenManagerListener_,
            DisplayManagerAgentType::SCREEN_EVENT_LISTENER);
        screenManagerListener_ = nullptr;
    }
    return true;
}

bool ScreenManager::UnregisterScreenGroupListener(sptr<IScreenGroupListener> listener)
{
    if (listener == nullptr) {
        WLOGFE("UnregisterScreenGroupListener listener is nullptr.");
        return false;
    }
    return pImpl_->UnregisterScreenGroupListener(listener);
}

ScreenId ScreenManager::MakeExpand(const std::vector<ExpandOption>& options)
{
    WLOGFI("make expand");
    if (options.empty()) {
        return SCREEN_ID_INVALID;
    }
    std::vector<ScreenId> screenIds;
    std::vector<Point> startPoints;
    for (auto& option: options) {
        screenIds.emplace_back(option.screenId_);
        startPoints.emplace_back(Point(option.startX_, option.startY_));
    }
    ScreenId group = SingletonContainer::Get<ScreenManagerAdapter>().MakeExpand(screenIds, startPoints);
    if (group == SCREEN_ID_INVALID) {
        WLOGFI("make expand failed");
    }
    return group;
}

ScreenId ScreenManager::MakeMirror(ScreenId mainScreenId, std::vector<ScreenId> mirrorScreenId)
{
    WLOGFI("create mirror for screen: %{public}" PRIu64"", mainScreenId);
    // TODO: "record screen" should use another function, "MakeMirror" should return group id.
    ScreenId group = SingletonContainer::Get<ScreenManagerAdapter>().MakeMirror(mainScreenId, mirrorScreenId);
    if (group == SCREEN_ID_INVALID) {
        WLOGFI("create mirror failed");
    }
    return group;
}

ScreenId ScreenManager::CreateVirtualScreen(VirtualScreenOption option)
{
    return SingletonContainer::Get<ScreenManagerAdapter>().CreateVirtualScreen(option);
}

DMError ScreenManager::DestroyVirtualScreen(ScreenId screenId)
{
    return SingletonContainer::Get<ScreenManagerAdapter>().DestroyVirtualScreen(screenId);
}

DMError ScreenManager::SetVirtualScreenSurface(ScreenId screenId, sptr<Surface> surface)
{
    return SingletonContainer::Get<ScreenManagerAdapter>().SetVirtualScreenSurface(screenId, surface);
}

void ScreenManager::Impl::NotifyScreenConnect(sptr<ScreenInfo> info)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    UpdateScreenInfoLocked(info);
}

void ScreenManager::Impl::NotifyScreenDisconnect(ScreenId screenId)
{
    WLOGFI("screenId:%{public}" PRIu64".", screenId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    screenMap_.erase(screenId);
}

void ScreenManager::Impl::NotifyScreenChange(const sptr<ScreenInfo>& screenInfo)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    UpdateScreenInfoLocked(screenInfo);
}

void ScreenManager::Impl::NotifyScreenChange(const std::vector<sptr<ScreenInfo>>& screenInfos)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto screenInfo : screenInfos) {
        UpdateScreenInfoLocked(screenInfo);
    }
}

bool ScreenManager::Impl::UpdateScreenInfoLocked(sptr<ScreenInfo> screenInfo)
{
    if (screenInfo == nullptr) {
        WLOGFE("displayInfo is null");
        return false;
    }
    ScreenId screenId = screenInfo->GetScreenId();
    WLOGFI("screenId:%{public}" PRIu64".", screenId);
    if (screenId == SCREEN_ID_INVALID) {
        WLOGFE("displayId is invalid.");
        return false;
    }
    auto iter = screenMap_.find(screenId);
    if (iter != screenMap_.end() && iter->second != nullptr) {
        WLOGFI("get screen in screen map");
        iter->second->UpdateScreenInfo(screenInfo);
        return true;
    }
    sptr<Screen> screen = new Screen(screenInfo);
    screenMap_[screenId] = screen;
    return true;
}
} // namespace OHOS::Rosen