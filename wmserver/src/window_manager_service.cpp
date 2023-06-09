/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "window_manager_service.h"

#include <cinttypes>

#include <ability_manager_client.h>
#include <ipc_skeleton.h>
#include <rs_iwindow_animation_controller.h>
#include <system_ability_definition.h>

#include "dm_common.h"
#include "display_manager_service_inner.h"
#include "drag_controller.h"
#include "singleton_container.h"
#include "window_helper.h"
#include "window_inner_manager.h"
#include "window_manager_agent_controller.h"
#include "window_manager_config.h"
#include "window_manager_hilog.h"
#include "wm_common.h"
#include "wm_trace.h"

namespace OHOS {
namespace Rosen {
namespace {
    constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, HILOG_DOMAIN_WINDOW, "WindowManagerService"};
}
WM_IMPLEMENT_SINGLE_INSTANCE(WindowManagerService)

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(&SingletonContainer::Get<WindowManagerService>());

WindowManagerService::WindowManagerService() : SystemAbility(WINDOW_MANAGER_SERVICE_ID, true)
{
    windowRoot_ = new WindowRoot(mutex_,
        std::bind(&WindowManagerService::OnWindowEvent, this, std::placeholders::_1, std::placeholders::_2));
    inputWindowMonitor_ = new InputWindowMonitor(windowRoot_);
    windowController_ = new WindowController(windowRoot_, inputWindowMonitor_);
    snapshotController_ = new SnapshotController(windowRoot_);
    dragController_ = new DragController(windowRoot_);
    freezeDisplayController_ = new FreezeController();
}

void WindowManagerService::OnStart()
{
    WLOGFI("WindowManagerService::OnStart start");
    if (!Init()) {
        return;
    }
    SingletonContainer::Get<WindowInnerManager>().Init();
    sptr<IDisplayChangeListener> listener = new DisplayChangeListener();
    DisplayManagerServiceInner::GetInstance().RegisterDisplayChangeListener(listener);
    RegisterSnapshotHandler();
    RegisterWindowManagerServiceHandler();
}

void WindowManagerService::RegisterSnapshotHandler()
{
    if (snapshotController_ == nullptr) {
        snapshotController_ = new SnapshotController(windowRoot_);
    }
    if (AAFwk::AbilityManagerClient::GetInstance()->RegisterSnapshotHandler(snapshotController_) != ERR_OK) {
        WLOGFW("WindowManagerService::RegisterSnapshotHandler failed, create async thread!");
        auto fun = [this]() {
            WLOGFI("WindowManagerService::RegisterSnapshotHandler async thread enter!");
            int counter = 0;
            while (AAFwk::AbilityManagerClient::GetInstance()->RegisterSnapshotHandler(snapshotController_) != ERR_OK) {
                usleep(10000); // 10000us equals to 10ms
                counter++;
                if (counter >= 2000) { // wait for 2000 * 10ms = 20s
                    WLOGFE("WindowManagerService::RegisterSnapshotHandler timeout!");
                    return;
                }
            }
            WLOGFI("WindowManagerService::RegisterSnapshotHandler async thread register handler successfully!");
        };
        std::thread thread(fun);
        thread.detach();
        WLOGFI("WindowManagerService::RegisterSnapshotHandler async thread has been detached!");
    } else {
        WLOGFI("WindowManagerService::RegisterSnapshotHandler OnStart succeed!");
    }
}

void WindowManagerService::RegisterWindowManagerServiceHandler()
{
}

bool WindowManagerService::Init()
{
    WLOGFI("WindowManagerService::Init start");
    bool ret = Publish(this);
    if (!ret) {
        WLOGFW("WindowManagerService::Init failed");
        return false;
    }
    if (WindowManagerConfig::LoadConfigXml(WINDOW_MANAGER_CONFIG_XML)) {
        WindowManagerConfig::DumpConfig();
        ConfigureWindowManagerService();
    }
    WLOGFI("WindowManagerService::Init success");
    return true;
}

void WindowManagerService::ConfigureWindowManagerService()
{
    auto enableConfig = WindowManagerConfig::GetEnableConfig();
    auto numbersConfig = WindowManagerConfig::GetNumbersConfig();

    if (enableConfig.count("decor") != 0) {
        isSystemDecorEnable_ = enableConfig["decor"];
    }

    if (enableConfig.count("minimizeByOther") != 0) {
        windowRoot_->SetMinimizedByOtherWindow(enableConfig["minimizeByOther"]);
    }

    if (numbersConfig.count("maxAppWindowNumber") != 0) {
        auto numbers = numbersConfig["maxAppWindowNumber"];
        if (numbers.size() == 1) {
            windowRoot_->SetMaxAppWindowNumber(numbers[0]);
        }
    }

    if (numbersConfig.count("modeChangeHotZones") != 0) {
        auto numbers = numbersConfig["modeChangeHotZones"];
        if (numbers.size() == 3) { // 3 hot zones
            hotZonesConfig_.fullscreenRange_ = static_cast<uint32_t>(numbers[0]); // 0 fullscreen
            hotZonesConfig_.primaryRange_ = static_cast<uint32_t>(numbers[1]);    // 1 primary
            hotZonesConfig_.secondaryRange_ = static_cast<uint32_t>(numbers[2]);  // 2 secondary
            hotZonesConfig_.isModeChangeHotZoneConfigured_ = true;
        }
    }
}

void WindowManagerService::OnStop()
{
    SingletonContainer::Get<WindowInnerManager>().SendMessage(InnerWMCmd::INNER_WM_DESTROY_THREAD);
    WLOGFI("ready to stop service.");
}

void WindowManagerService::NotifyWindowTransition(WindowTransitionInfo fromInfo, WindowTransitionInfo toInfo)
{
    windowController_->NotifyWindowTransition(fromInfo, toInfo);
}

WMError WindowManagerService::CreateWindow(sptr<IWindow>& window, sptr<WindowProperty>& property,
    const std::shared_ptr<RSSurfaceNode>& surfaceNode, uint32_t& windowId, sptr<IRemoteObject> token)
{
    WM_SCOPED_TRACE("wms:CreateWindow(%u)", windowId);
    if (window == nullptr || property == nullptr || surfaceNode == nullptr) {
        WLOGFE("window is invalid");
        return WMError::WM_ERROR_NULLPTR;
    }
    if ((!window) || (!window->AsObject())) {
        WLOGFE("failed to get window agent");
        return WMError::WM_ERROR_NULLPTR;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->CreateWindow(window, property, surfaceNode, windowId, token);
}

WMError WindowManagerService::AddWindow(sptr<WindowProperty>& property)
{
    Rect rect = property->GetRequestRect();
    uint32_t windowId = property->GetWindowId();
    WLOGFI("[WMS] Add: %{public}5d %{public}4d %{public}4d %{public}4d [%{public}4d %{public}4d " \
        "%{public}4d %{public}4d]", windowId, property->GetWindowType(), property->GetWindowMode(),
        property->GetWindowFlags(), rect.posX_, rect.posY_, rect.width_, rect.height_);
    WM_SCOPED_TRACE("wms:AddWindow(%u)", windowId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    WMError res = windowController_->AddWindowNode(property);
    if (property->GetWindowType() == WindowType::WINDOW_TYPE_DRAGGING_EFFECT) {
        dragController_->StartDrag(windowId);
    }
    return res;
}

WMError WindowManagerService::RemoveWindow(uint32_t windowId)
{
    WLOGFI("[WMS] Remove: %{public}u", windowId);
    WM_SCOPED_TRACE("wms:RemoveWindow(%u)", windowId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->RemoveWindowNode(windowId);
}

WMError WindowManagerService::DestroyWindow(uint32_t windowId, bool onlySelf)
{
    WLOGFI("[WMS] Destroy: %{public}u", windowId);
    WM_SCOPED_TRACE("wms:DestroyWindow(%u)", windowId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto node = windowRoot_->GetWindowNode(windowId);
    if (node != nullptr && node->GetWindowType() == WindowType::WINDOW_TYPE_DRAGGING_EFFECT) {
        dragController_->FinishDrag(windowId);
    }
    return windowController_->DestroyWindow(windowId, onlySelf);
}

WMError WindowManagerService::RequestFocus(uint32_t windowId)
{
    WLOGFI("[WMS] RequestFocus: %{public}u", windowId);
    WM_SCOPED_TRACE("wms:RequestFocus");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->RequestFocus(windowId);
}

WMError WindowManagerService::SetWindowBackgroundBlur(uint32_t windowId, WindowBlurLevel level)
{
    WM_SCOPED_TRACE("wms:SetWindowBackgroundBlur");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->SetWindowBackgroundBlur(windowId, level);
}

WMError WindowManagerService::SetAlpha(uint32_t windowId, float alpha)
{
    WM_SCOPED_TRACE("wms:SetAlpha");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->SetAlpha(windowId, alpha);
}

std::vector<Rect> WindowManagerService::GetAvoidAreaByType(uint32_t windowId, AvoidAreaType avoidAreaType)
{
    WLOGFI("[WMS] GetAvoidAreaByType: %{public}u, Type: %{public}u", windowId, static_cast<uint32_t>(avoidAreaType));
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->GetAvoidAreaByType(windowId, avoidAreaType);
}

void WindowManagerService::RegisterWindowManagerAgent(WindowManagerAgentType type,
    const sptr<IWindowManagerAgent>& windowManagerAgent)
{
    if ((windowManagerAgent == nullptr) || (windowManagerAgent->AsObject() == nullptr)) {
        WLOGFE("windowManagerAgent is null");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    WindowManagerAgentController::GetInstance().RegisterWindowManagerAgent(windowManagerAgent, type);
    if (type == WindowManagerAgentType::WINDOW_MANAGER_AGENT_TYPE_SYSTEM_BAR) { // if system bar, notify once
        windowController_->NotifySystemBarTints();
    }
}

void WindowManagerService::UnregisterWindowManagerAgent(WindowManagerAgentType type,
    const sptr<IWindowManagerAgent>& windowManagerAgent)
{
    if ((windowManagerAgent == nullptr) || (windowManagerAgent->AsObject() == nullptr)) {
        WLOGFE("windowManagerAgent is null");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    WindowManagerAgentController::GetInstance().UnregisterWindowManagerAgent(windowManagerAgent, type);
}

WMError WindowManagerService::SetWindowAnimationController(const sptr<RSIWindowAnimationController>& controller)
{
    if (controller == nullptr) {
        WLOGFE("Failed to set window animation controller, controller is null!");
        return WMError::WM_ERROR_NULLPTR;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->SetWindowAnimationController(controller);
}

void WindowManagerService::OnWindowEvent(Event event, uint32_t windowId)
{
    if (event == Event::REMOTE_DIED) {
        DestroyWindow(windowId, true);
    }
}

void WindowManagerService::NotifyDisplayStateChange(DisplayId id, DisplayStateChangeType type)
{
    WM_SCOPED_TRACE("wms:NotifyDisplayStateChange(%u)", type);
    if (type == DisplayStateChangeType::FREEZE) {
        freezeDisplayController_->FreezeDisplay(id);
    } else if (type == DisplayStateChangeType::UNFREEZE) {
        freezeDisplayController_->UnfreezeDisplay(id);
    } else {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return windowController_->NotifyDisplayStateChange(id, type);
    }
}

void DisplayChangeListener::OnDisplayStateChange(DisplayId id, DisplayStateChangeType type)
{
    WindowManagerService::GetInstance().NotifyDisplayStateChange(id, type);
}

void WindowManagerService::ProcessPointDown(uint32_t windowId, bool isStartDrag)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    windowController_->ProcessPointDown(windowId, isStartDrag);
}

void WindowManagerService::ProcessPointUp(uint32_t windowId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    windowController_->ProcessPointUp(windowId);
}

void WindowManagerService::MinimizeAllAppWindows(DisplayId displayId)
{
    WLOGFI("displayId %{public}" PRIu64"", displayId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    windowController_->MinimizeAllAppWindows(displayId);
}

WMError WindowManagerService::MaxmizeWindow(uint32_t windowId)
{
    WM_SCOPED_TRACE("wms:MaxmizeWindow");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->MaxmizeWindow(windowId);
}

WMError WindowManagerService::GetTopWindowId(uint32_t mainWinId, uint32_t& topWinId)
{
    WM_SCOPED_TRACE("wms:GetTopWindowId(%u)", mainWinId);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->GetTopWindowId(mainWinId, topWinId);
}

WMError WindowManagerService::SetWindowLayoutMode(DisplayId displayId, WindowLayoutMode mode)
{
    WLOGFI("SetWindowLayoutMode, displayId: %{public}" PRIu64", layoutMode: %{public}u", displayId, mode);
    WM_SCOPED_TRACE("wms:SetWindowLayoutMode");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return windowController_->SetWindowLayoutMode(displayId, mode);
}

WMError WindowManagerService::UpdateProperty(sptr<WindowProperty>& windowProperty, PropertyChangeAction action)
{
    if (windowProperty == nullptr) {
        WLOGFE("property is invalid");
        return WMError::WM_ERROR_NULLPTR;
    }
    WM_SCOPED_TRACE("wms:UpdateProperty");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    WMError res = windowController_->UpdateProperty(windowProperty, action);
    if (action == PropertyChangeAction::ACTION_UPDATE_RECT && res == WMError::WM_OK &&
        windowProperty->GetWindowSizeChangeReason() == WindowSizeChangeReason::MOVE) {
        dragController_->UpdateDragInfo(windowProperty->GetWindowId());
    }
    return res;
}

WMError WindowManagerService::GetAccessibilityWindowInfo(sptr<AccessibilityWindowInfo>& windowInfo)
{
    if (windowInfo == nullptr) {
        WLOGFE("windowInfo is invalid");
        return WMError::WM_ERROR_NULLPTR;
    }
    WM_SCOPED_TRACE("wms:GetAccessibilityWindowInfo");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    WMError res = windowRoot_->GetAccessibilityWindowInfo(windowInfo);
    return res;
}

WMError WindowManagerService::GetSystemDecorEnable(bool& isSystemDecorEnable)
{
    isSystemDecorEnable = isSystemDecorEnable_;
    return WMError::WM_OK;
}

WMError WindowManagerService::GetModeChangeHotZones(DisplayId displayId, ModeChangeHotZones& hotZones)
{
    if (!hotZonesConfig_.isModeChangeHotZoneConfigured_) {
        return WMError::WM_DO_NOTHING;
    }

    return windowController_->GetModeChangeHotZones(displayId, hotZones, hotZonesConfig_);
}
} // namespace Rosen
} // namespace OHOS