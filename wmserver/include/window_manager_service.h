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

#ifndef OHOS_WINDOW_MANAGER_SERVICE_H
#define OHOS_WINDOW_MANAGER_SERVICE_H

#include <vector>
#include <map>

#include <input_window_monitor.h>
#include <nocopyable.h>
#include <system_ability.h>
#include "display_change_listener.h"
#include "drag_controller.h"
#include "freeze_controller.h"
#include "singleton_delegator.h"
#include "wm_single_instance.h"
#include "window_controller.h"
#include "window_manager_stub.h"
#include "window_root.h"
#include "snapshot_controller.h"

namespace OHOS {
namespace Rosen {
class DisplayChangeListener : public IDisplayChangeListener {
public:
    virtual void OnDisplayStateChange(DisplayId id, DisplayStateChangeType type) override;
};
class WindowManagerService : public SystemAbility, public WindowManagerStub {
friend class DisplayChangeListener;
DECLARE_SYSTEM_ABILITY(WindowManagerService);
WM_DECLARE_SINGLE_INSTANCE_BASE(WindowManagerService);

public:
    void OnStart() override;
    void OnStop() override;

    WMError CreateWindow(sptr<IWindow>& window, sptr<WindowProperty>& property,
        const std::shared_ptr<RSSurfaceNode>& surfaceNode,
        uint32_t& windowId, sptr<IRemoteObject> token) override;
    WMError AddWindow(sptr<WindowProperty>& property) override;
    WMError RemoveWindow(uint32_t windowId) override;
    void NotifyWindowTransition(WindowTransitionInfo fromInfo, WindowTransitionInfo toInfo) override;
    WMError DestroyWindow(uint32_t windowId, bool onlySelf = false) override;
    WMError RequestFocus(uint32_t windowId) override;
    WMError SetWindowBackgroundBlur(uint32_t windowId, WindowBlurLevel level) override;
    WMError SetAlpha(uint32_t windowId, float alpha) override;
    std::vector<Rect> GetAvoidAreaByType(uint32_t windowId, AvoidAreaType avoidAreaType) override;
    void ProcessPointDown(uint32_t windowId, bool isStartDrag) override;
    void ProcessPointUp(uint32_t windowId) override;
    WMError GetTopWindowId(uint32_t mainWinId, uint32_t& topWinId) override;
    void MinimizeAllAppWindows(DisplayId displayId) override;
    WMError MaxmizeWindow(uint32_t windowId) override;
    WMError SetWindowLayoutMode(DisplayId displayId, WindowLayoutMode mode) override;
    WMError UpdateProperty(sptr<WindowProperty>& windowProperty, PropertyChangeAction action) override;
    WMError GetAccessibilityWindowInfo(sptr<AccessibilityWindowInfo>& windowInfo) override;

    void RegisterWindowManagerAgent(WindowManagerAgentType type,
        const sptr<IWindowManagerAgent>& windowManagerAgent) override;
    void UnregisterWindowManagerAgent(WindowManagerAgentType type,
        const sptr<IWindowManagerAgent>& windowManagerAgent) override;

    WMError SetWindowAnimationController(const sptr<RSIWindowAnimationController>& controller) override;
    WMError GetSystemDecorEnable(bool& isSystemDecorEnable) override;
    WMError GetModeChangeHotZones(DisplayId displayId, ModeChangeHotZones& hotZones) override;

protected:
    WindowManagerService();
    virtual ~WindowManagerService() = default;

private:
    bool Init();
    void RegisterSnapshotHandler();
    void RegisterWindowManagerServiceHandler();
    void OnWindowEvent(Event event, uint32_t windowId);
    void NotifyDisplayStateChange(DisplayId id, DisplayStateChangeType type);
    void ConfigureWindowManagerService();

    static inline SingletonDelegator<WindowManagerService> delegator;
    std::recursive_mutex mutex_;
    sptr<WindowRoot> windowRoot_;
    sptr<WindowController> windowController_;
    sptr<InputWindowMonitor> inputWindowMonitor_;
    sptr<SnapshotController> snapshotController_;
    sptr<DragController> dragController_;
    sptr<FreezeController> freezeDisplayController_;
    bool isSystemDecorEnable_ = true;
    ModeChangeHotZonesConfig hotZonesConfig_ { false, 0, 0, 0 };
};
}
}
#endif // OHOS_WINDOW_MANAGER_SERVICE_H
