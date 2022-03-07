/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "js_window_register_manager.h"
#include "singleton_container.h"
#include "window_manager.h"
#include "window_manager_hilog.h"
namespace OHOS {
namespace Rosen {
namespace {
    constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, HILOG_DOMAIN_WINDOW, "JsWindowRegisterManager"};
}

JsWindowRegisterManager::JsWindowRegisterManager()
{
    // white register list for window manager
    listenerProcess_[CaseType::CASE_WINDOW_MANAGER] = {
        {SYSTEM_BAR_TINT_CHANGE_CB,     &JsWindowRegisterManager::ProcessSystemBarChangeRegister }
    };
    // white register list for window
    listenerProcess_[CaseType::CASE_WINDOW] = {
        {WINDOW_SIZE_CHANGE_CB,         &JsWindowRegisterManager::ProcessWindowChangeRegister    },
        {SYSTEM_AVOID_AREA_CHANGE_CB,   &JsWindowRegisterManager::ProcessAvoidAreaChangeRegister },
        {LIFECYCLE_EVENT_CB,            &JsWindowRegisterManager::ProcessLifeCycleEventRegister  }
    };
    // white register list for window stage
    listenerProcess_[CaseType::CASE_STAGE] = {
        {WINDOW_STAGE_EVENT_CB,         &JsWindowRegisterManager::ProcessLifeCycleEventRegister  }
    };
}

JsWindowRegisterManager::~JsWindowRegisterManager()
{
}

bool JsWindowRegisterManager::ProcessWindowChangeRegister(sptr<JsWindowListener> listener,
    sptr<Window> window, bool isRegister)
{
    if (window == nullptr) {
        WLOGFE("window is nullptr");
        return false;
    }
    sptr<IWindowChangeListener> thisListener(listener);
    if (isRegister) {
        window->RegisterWindowChangeListener(thisListener);
    } else {
        window->UnregisterWindowChangeListener(thisListener);
    }
    return true;
}

bool JsWindowRegisterManager::ProcessAvoidAreaChangeRegister(sptr<JsWindowListener> listener,
    sptr<Window> window, bool isRegister)
{
    if (window == nullptr) {
        WLOGFE("window is nullptr");
        return false;
    }
    sptr<IAvoidAreaChangedListener> thisListener(listener);
    if (isRegister) {
        window->RegisterAvoidAreaChangeListener(thisListener);
    } else {
        window->UnregisterAvoidAreaChangeListener(thisListener);
    }
    return true;
}

bool JsWindowRegisterManager::ProcessLifeCycleEventRegister(sptr<JsWindowListener> listener,
    sptr<Window> window, bool isRegister)
{
    if (window == nullptr) {
        WLOGFE("window is nullptr");
        return false;
    }
    sptr<IWindowLifeCycle> thisListener(listener);
    if (isRegister) {
        window->RegisterLifeCycleListener(thisListener);
    } else {
        window->UnregisterLifeCycleListener(thisListener);
    }
    return true;
}

bool JsWindowRegisterManager::ProcessSystemBarChangeRegister(sptr<JsWindowListener> listener,
    sptr<Window> window, bool isRegister)
{
    sptr<ISystemBarChangedListener> thisListener(listener);
    if (isRegister) {
        SingletonContainer::Get<WindowManager>().RegisterSystemBarChangedListener(thisListener);
    } else {
        SingletonContainer::Get<WindowManager>().UnregisterSystemBarChangedListener(thisListener);
    }
    return true;
}

bool JsWindowRegisterManager::IsCallbackRegistered(std::string type, NativeValue* jsListenerObject)
{
    if (jsCbMap_.empty() || jsCbMap_.find(type) == jsCbMap_.end()) {
        WLOGFI("JsWindowRegisterManager::IsCallbackRegistered methodName %{public}s not registerted!", type.c_str());
        return false;
    }

    for (auto iter = jsCbMap_[type].begin(); iter != jsCbMap_[type].end(); ++iter) {
        if (jsListenerObject->StrictEquals(iter->first->Get())) {
            WLOGFE("JsWindowRegisterManager::IsCallbackRegistered %{public}s already registered!", type.c_str());
            return true;
        }
    }
    return false;
}

void JsWindowRegisterManager::RegisterListener(sptr<Window> window, std::string type,
    CaseType caseType, NativeEngine& engine, NativeValue* value)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (IsCallbackRegistered(type, value)) {
        return;
    }
    if (listenerProcess_[caseType].count(type) == 0) {
        WLOGFE("JsWindowRegisterManager not support method: %{public}s!", type.c_str());
        return;
    }
    std::shared_ptr<NativeReference> callbackRef;
    callbackRef.reset(engine.CreateReference(value, 1));
    sptr<JsWindowListener> windowManagerListener = new(std::nothrow) JsWindowListener(&engine, callbackRef);
    if (windowManagerListener == nullptr) {
        WLOGFE("JsWindowRegisterManager::RegisterListener malloc failed");
        return;
    }
    if (!((this->*listenerProcess_[caseType][type])(windowManagerListener, window, true))) {
        WLOGFE("JsWindowRegisterManager::RegisterListener %{public}s failed!", type.c_str());
        return;
    }
    jsCbMap_[type][callbackRef] = windowManagerListener;
    WLOGFI("JsWindowRegisterManager::RegisterListener %{public}s success! callback map size: %{public}u",
        type.c_str(), jsCbMap_[type].size());
}

void JsWindowRegisterManager::UnregisterListener(sptr<Window> window, std::string type,
    CaseType caseType, NativeValue* value)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (jsCbMap_.empty() || jsCbMap_.find(type) == jsCbMap_.end()) {
        WLOGFI("JsWindowRegisterManager::UnregisterListener methodName %{public}s not registerted!",
            type.c_str());
        return;
    }
    if (listenerProcess_[caseType].count(type) == 0) {
        WLOGFE("JsWindowRegisterManager not support method: %{public}s!", type.c_str());
        return;
    }
    if (value == nullptr) {
        for (auto it = jsCbMap_[type].begin(); it != jsCbMap_[type].end();) {
            if (!((this->*listenerProcess_[caseType][type])(it->second, window, false))) {
                WLOGFE("JsWindowRegisterManager::UnregisterListener %{public}s failed!", type.c_str());
                return;
            }
            jsCbMap_[type].erase(it++);
        }
    } else {
        bool findFlag = false;
        for (auto it = jsCbMap_[type].begin(); it != jsCbMap_[type].end(); ++it) {
            if (!value->StrictEquals(it->first->Get())) {
                continue;
            }
            findFlag = true;
            if (!(this->*listenerProcess_[caseType][type])(it->second, window, false)) {
                WLOGFE("JsWindowRegisterManager::UnregisterListener %{public}s failed!", type.c_str());
                return;
            }
            jsCbMap_[type].erase(it);
            break;
        }
        if (!findFlag) {
            WLOGFE("JsWindowRegisterManager::UnregisterListener %{public}s failed because not found callback!",
                type.c_str());
            return;
        }
    }
    WLOGFI("JsWindowRegisterManager::UnregisterListener %{public}s success! callback map size: %{public}u",
        type.c_str(), jsCbMap_[type].size());
    // erase type when there is no callback in one type
    if (jsCbMap_[type].empty()) {
        jsCbMap_.erase(type);
    }
}
} // namespace Rosen
} // namespace OHOS