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

#include "js_window_stage.h"
#include <string>
#include "js_runtime_utils.h"
#include "js_window.h"
#include "js_window_listener.h"
#include "js_window_utils.h"
#include "window_manager_hilog.h"
namespace OHOS {
namespace Rosen {
using namespace AbilityRuntime;
namespace {
const int CONTENT_STORAGE_ARG = 2;
constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, HILOG_DOMAIN_WINDOW, "JsWindowStage"};
} // namespace

static std::map<std::shared_ptr<NativeReference>, sptr<JsWindowListener>> g_jsCbMap;
void JsWindowStage::Finalizer(NativeEngine* engine, void* data, void* hint)
{
    WLOGFI("JsWindowStage::Finalizer is called");
    std::unique_ptr<JsWindowStage>(static_cast<JsWindowStage*>(data));
}

NativeValue* JsWindowStage::SetUIContent(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::SetUIContent is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OnSetUIContent(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::GetMainWindow(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::GetMainWindow is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OnGetMainWindow(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::On(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::On is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OnEvent(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::Off(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::Off is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OffEvent(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::LoadContent(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::LoadContent is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OnLoadContent(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::GetWindowMode(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::GetWindowMode is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OnGetWindowMode(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::CreateSubWindow(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::CreateSubWindow is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OnCreateSubWindow(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::GetSubWindow(NativeEngine* engine, NativeCallbackInfo* info)
{
    WLOGFI("JsWindowStage::GetSubWindow is called");
    JsWindowStage* me = CheckParamsAndGetThis<JsWindowStage>(engine, info);
    return (me != nullptr) ? me->OnGetSubWindow(*engine, *info) : nullptr;
}

NativeValue* JsWindowStage::OnSetUIContent(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OnSetUIContent is called");
    if (info.argc < 2) { // 2: minimum param nums
        WLOGFE("JsWindowStage::OnSetUIContent Not enough params");
        return engine.CreateUndefined();
    }
    if (windowScene_ == nullptr || windowScene_->GetMainWindow() == nullptr) {
        WLOGFE("JsWindowStage::OnSetUIContent windowScene_ or MainWindow is nullptr");
        return engine.CreateUndefined();
    }

    // Parse info->argv[0] as abilitycontext
    auto objContext = ConvertNativeValueTo<NativeObject>(info.argv[0]);
    if (objContext == nullptr) {
        WLOGFE("JsWindowStage::OnSetUIContent info->argv[0] InValid");
        return engine.CreateUndefined();
    }

    // Parse info->argv[1] as url
    std::string contextUrl;
    if (!ConvertFromJsValue(engine, info.argv[1], contextUrl)) {
        WLOGFE("JsWindowStage::OnSetUIContent failed to convert parameter to url");
        return engine.CreateUndefined();
    }
    WLOGFI("JsWindowStage::OnSetUIContent Get url: %{public}s", contextUrl.c_str());

    windowScene_->GetMainWindow()->SetUIContent(contextUrl, &engine, info.argv[CONTENT_STORAGE_ARG]);

    return engine.CreateUndefined();
}

NativeValue* JsWindowStage::OnGetMainWindow(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OnGetMainWindow is called");
    WMError errCode = WMError::WM_OK;
    if (windowScene_ == nullptr) {
        WLOGFE("JsWindowStage::OnGetMainWindow windowScene_ is nullptr");
        errCode = WMError::WM_ERROR_INVALID_PARAM;
    }
    AsyncTask::CompleteCallback complete =
        [=](NativeEngine& engine, AsyncTask& task, int32_t status) {
            if (errCode != WMError::WM_OK) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(errCode), "Invalidate params."));
                return;
            }
            auto window = windowScene_->GetMainWindow();
            if (window != nullptr) {
                task.Resolve(engine, OHOS::Rosen::CreateJsWindowObject(engine, window));
                WLOGFI("JsWindowStage::OnGetMainWindow success");
            } else {
                task.Reject(engine, CreateJsError(engine,
                    static_cast<int32_t>(Rosen::WMError::WM_ERROR_NULLPTR),
                    "JsWindowStage::OnGetMainWindow failed."));
            }
        };
    NativeValue* callback = (info.argc == 0) ? nullptr :
        (info.argv[0]->TypeOf() == NATIVE_FUNCTION ? info.argv[0] : nullptr);
    NativeValue* result = nullptr;
    AsyncTask::Schedule(
        engine, CreateAsyncTaskWithLastParam(engine, callback, nullptr, std::move(complete), &result));
    return result;
}

bool JsWindowStage::IsCallbackRegistered(std::string type, NativeValue* jsListenerObject)
{
    if (g_jsCbMap.empty()) {
        WLOGFI("JsWindowStage::JsWindowStage methodName %{public}s not registerted!", type.c_str());
        return false;
    }

    for (auto iter = g_jsCbMap.begin(); iter != g_jsCbMap.end(); iter++) {
        if (jsListenerObject->StrictEquals(iter->first->Get())) {
            WLOGFE("JsWindowStage::IsCallbackRegistered callback already registered!");
            return true;
        }
    }
    return false;
}

NativeValue* JsWindowStage::OnEvent(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OnEvent is called");
    if (windowScene_ == nullptr || info.argc < 2) { // 2: minimum param nums
        WLOGFE("JsWindowStage::OnEvent windowScene_ is nullptr or params not match");
        return engine.CreateUndefined();
    }
    // Parse info->argv[0] as string
    std::string eventString;
    if (!ConvertFromJsValue(engine, info.argv[0], eventString)) {
        WLOGFE("JsWindowStage::OnEvent info->argv[0] Failed to convert parameter to string");
        return engine.CreateUndefined();
    }
    if (eventString.compare("windowStageEvent") != 0) {
        WLOGFE("JsWindowStage::OnEvent info->argv[0] is %{public}s, InValid",
            eventString.c_str());
        return engine.CreateUndefined();
    }
    NativeValue* value = info.argv[1];
    if (!value->IsCallable()) {
        WLOGFE("JsWindowStage::OnEvent info->argv[1] is not callable");
        return engine.CreateUndefined();
    }
    std::lock_guard<std::mutex> lock(mtx_);
    if (IsCallbackRegistered(eventString, value)) {
        WLOGFE("JsWindowStage::OnEvent callback already registered!");
        return engine.CreateUndefined();
    }
    std::shared_ptr<NativeReference> reference = nullptr;
    reference.reset(engine.CreateReference(value, 1));
    sptr<JsWindowListener> windowListener = new(std::nothrow) JsWindowListener(&engine, reference);
    if (windowListener == nullptr) {
        WLOGFE("JsWindowStage::OnEvent windowListener malloc failed");
        return engine.CreateUndefined();
    }
    auto window = windowScene_->GetMainWindow();
    if (window != nullptr) {
        sptr<IWindowLifeCycle> thisListener(windowListener);
        window->RegisterLifeCycleListener(thisListener);
    }
    g_jsCbMap[reference] = windowListener;
    WLOGFI("JsWindowStage::OnEvent callback map size: %{public}u success", g_jsCbMap.size());
    return engine.CreateUndefined();
}

void JsWindowStage::UnregisterAllWindowListenerWithType(std::string type)
{
    if (g_jsCbMap.empty()) {
        WLOGFE("JsWindowStage::UnregisterAllWindowListenerWithType methodName %{public}s not registerted!",
            type.c_str());
        return;
    }
    auto window = windowScene_->GetMainWindow();
    if (window == nullptr) {
        WLOGFE("JsWindowStage::UnregisterAllWindowListenerWithType mainWindow is null!");
        return;
    }
    for (auto it = g_jsCbMap.begin(); it != g_jsCbMap.end();) {
        sptr<IWindowLifeCycle> thisListener(it->second);
        window->UnregisterLifeCycleListener(thisListener);
        g_jsCbMap.erase(it++);
    }
    WLOGFI("JsWindowStage::UnregisterWindowListenerWithType callback map size: %{public}u success",
        g_jsCbMap.size());
}

void JsWindowStage::UnregisterWindowListenerWithType(std::string type, NativeValue* value)
{
    if (g_jsCbMap.empty()) {
        WLOGFE("JsWindowStage::UnregisterWindowListenerWithType methodName %{public}s not registerted!",
            type.c_str());
        return;
    }

    for (auto it = g_jsCbMap.begin(); it != g_jsCbMap.end();it++) {
        if (value->StrictEquals(it->first->Get())) {
            auto window = windowScene_->GetMainWindow();
            if (window != nullptr) {
                sptr<IWindowLifeCycle> thisListener(it->second);
                window->UnregisterLifeCycleListener(thisListener);
            }
            g_jsCbMap.erase(it++);
            break;
        }
    }
    // one type with multi jscallback, erase type when there is no callback in one type
    WLOGFI("JsWindowStage::UnregisterWindowListenerWithType callback map size: %{public}u success",
        g_jsCbMap.size());
}

NativeValue* JsWindowStage::OffEvent(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OffEvent is called");
    if (windowScene_ == nullptr) {
        WLOGFE("JsWindowStage::OffEvent windowScene_ is nullptr");
        return engine.CreateUndefined();
    }
    if (info.argc < 1 || info.argc > 2) { // 1: minimum param nums, 2: maximum param nums
        WLOGFE("JsWindowStage::OffEvent wrong input params");
        return engine.CreateUndefined();
    }

    // Parse info->argv[0] as string
    std::string eventString;
    if (!ConvertFromJsValue(engine, info.argv[0], eventString)) {
        WLOGFE("JsWindowStage::OffEvent info->argv[0] Failed to convert parameter to string");
        return engine.CreateUndefined();
    }
    if (eventString.compare("windowStageEvent") != 0) {
        WLOGFE("JsWindowStage::OffEvent info->argv[0] is InValid");
        return engine.CreateUndefined();
    }

    NativeValue* value = info.argv[1];
    std::lock_guard<std::mutex> lock(mtx_);
    if (value->TypeOf() == NATIVE_FUNCTION) {
        WLOGFI("JsWindowStage::OffEvent info->argv[1] is callable type");
        UnregisterWindowListenerWithType(eventString, value);
    } else if (value->TypeOf() == NativeValueType::NATIVE_UNDEFINED) {
        WLOGFI("JsWindowStage::OffEvent info->argv[1] is native undefined type");
        UnregisterAllWindowListenerWithType(eventString);
    } else {
        WLOGFE("JsWindowStage::OffEvent info->argv[1] is InValid param");
    }
    return engine.CreateUndefined();
}

NativeValue* JsWindowStage::OnLoadContent(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OnLoadContent is called");
    WMError errCode = WMError::WM_OK;
    if (info.argc <= 0 || windowScene_ == nullptr) {
        WLOGFE("JsWindowStage param not match or windowScene_ is nullptr");
        errCode = WMError::WM_ERROR_INVALID_PARAM;
    }
    std::string contextUrl;
    if (!ConvertFromJsValue(engine, info.argv[0], contextUrl)) {
        WLOGFE("Failed to convert parameter to context url");
        errCode = WMError::WM_ERROR_INVALID_PARAM;
    }
    NativeValue* storage = nullptr;
    NativeValue* callBack = nullptr;
    NativeValue* value1 = info.argv[INDEX_ONE];
    NativeValue* value2 = info.argv[INDEX_TWO];
    if (value1->TypeOf() == NATIVE_FUNCTION) {
        callBack = value1;
    } else if (value1->TypeOf() == NATIVE_OBJECT) {
        storage = value1;
    }
    if (value2->TypeOf() == NATIVE_FUNCTION) {
        callBack = value2;
    }
    std::shared_ptr<NativeReference> contentStorage = (storage == nullptr) ? nullptr :
        std::shared_ptr<NativeReference>(engine.CreateReference(storage, 1));
    AsyncTask::CompleteCallback complete =
        [=](NativeEngine& engine, AsyncTask& task, int32_t status) {
            if (errCode != WMError::WM_OK) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(errCode), "Invalidate params."));
                return;
            }
            auto win = windowScene_->GetMainWindow();
            if (win == nullptr) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(WMError::WM_ERROR_NULLPTR)));
                return;
            }
            NativeValue* nativeStorage = (contentStorage == nullptr) ? nullptr : contentStorage->Get();
            Rosen::WMError ret = win->SetUIContent(contextUrl, &engine, nativeStorage, false);
            if (ret != Rosen::WMError::WM_OK) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(ret)));
                return;
            }
            task.Resolve(engine, engine.CreateUndefined());
            WLOGFI("JsWindowStage::OnLoadContent success");
        };
    NativeValue* result = nullptr;
    AsyncTask::Schedule(
        engine, CreateAsyncTaskWithLastParam(engine, callBack, nullptr, std::move(complete), &result));
    return result;
}

NativeValue* JsWindowStage::OnGetWindowMode(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OnGetWindowMode is called");
    WMError errCode = WMError::WM_OK;
    if (info.argc > ARGC_ONE || windowScene_ == nullptr) {
        WLOGFE("JsWindowStage::OnGetWindowMode params not match or windowScene is nullptr!");
        errCode = WMError::WM_ERROR_INVALID_PARAM;
    }
    AsyncTask::CompleteCallback complete =
        [=](NativeEngine& engine, AsyncTask& task, int32_t status) {
            if (errCode != WMError::WM_OK) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(errCode), "Invalidate params."));
                return;
            }
            auto window = windowScene_->GetMainWindow();
            if (window == nullptr) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(Rosen::WMError::WM_ERROR_NULLPTR),
                    "JsWindowStage::OnGetWindowMode failed."));
                WLOGFE("JsWindowStage window is nullptr");
                return;
            }
            Rosen::WindowMode mode = window->GetMode();
            if (NATIVE_TO_JS_WINDOW_MODE_MAP.count(mode) != 0) {
                task.Resolve(engine, CreateJsValue(engine, NATIVE_TO_JS_WINDOW_MODE_MAP.at(mode)));
                WLOGFI("JsWindowStage OnGetWindowMode success");
            } else {
                task.Resolve(engine, CreateJsValue(engine, mode));
                WLOGFI("JsWindowStage OnGetWindowMode success, but not in apimode");
            }
        };
    NativeValue* callback = (info.argc == 0) ? nullptr :
        (info.argv[0]->TypeOf() == NATIVE_FUNCTION ? info.argv[0] : nullptr);
    NativeValue* result = nullptr;
    AsyncTask::Schedule(
        engine, CreateAsyncTaskWithLastParam(engine, callback, nullptr, std::move(complete), &result));
    return result;
}

NativeValue* JsWindowStage::OnCreateSubWindow(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OnCreateSubWindow is called");
    WMError errCode = WMError::WM_OK;
    if (info.argc < ARGC_ONE || windowScene_ == nullptr) {
        WLOGFE("JsWindowStage::OnCreateSubWindow params less than 1!");
        errCode = WMError::WM_ERROR_INVALID_PARAM;
    }
    std::string windowName;
    if (!ConvertFromJsValue(engine, info.argv[0], windowName)) {
        WLOGFE("Failed to convert parameter to windowName");
        errCode = WMError::WM_ERROR_INVALID_PARAM;
    }
    AsyncTask::CompleteCallback complete =
        [=](NativeEngine& engine, AsyncTask& task, int32_t status) {
            if (errCode != WMError::WM_OK) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(errCode), "Invalidate params."));
                return;
            }
            sptr<Rosen::WindowOption> windowOption = new Rosen::WindowOption();
            windowOption->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_SUB_WINDOW);
            windowOption->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
            auto window = windowScene_->CreateWindow(windowName, windowOption);
            if (window == nullptr) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(Rosen::WMError::WM_ERROR_NULLPTR),
                    "JsWindowStage::OnCreateSubWindow failed."));
                WLOGFE("JsWindowStage window is nullptr");
                return;
            }
            task.Resolve(engine, CreateJsWindowObject(engine, window));
            WLOGFI("JsWindowStage OnCreateSubWindow success");
        };
    NativeValue* callback = (info.argc <= 1) ? nullptr :
        (info.argv[1]->TypeOf() == NATIVE_FUNCTION ? info.argv[1] : nullptr);
    NativeValue* result = nullptr;
    AsyncTask::Schedule(
        engine, CreateAsyncTaskWithLastParam(engine, callback, nullptr, std::move(complete), &result));
    return result;
}

NativeValue* JsWindowStage::CreateJsSubWindowArrayObject(NativeEngine& engine,
    std::vector<sptr<Window>> subWinVec)
{
    WLOGFI("JsWindowUtils::CreateJsSubWindowArrayObject is called");
    NativeValue* objValue = engine.CreateArray(subWinVec.size());
    NativeArray* array = ConvertNativeValueTo<NativeArray>(objValue);
    if (array == nullptr) {
        WLOGFE("Failed to convert subWinVec to jsArrayObject");
        return nullptr;
    }
    uint32_t index = 0;
    for (size_t i = 0; i < subWinVec.size(); i++) {
        array->SetElement(index++, CreateJsWindowObject(engine, subWinVec[i]));
    }
    return objValue;
}

NativeValue* JsWindowStage::OnGetSubWindow(NativeEngine& engine, NativeCallbackInfo& info)
{
    WLOGFI("JsWindowStage::OnGetSubWindow is called");
    WMError errCode = WMError::WM_OK;
    if (info.argc > 1 || windowScene_ == nullptr) {
        WLOGFE("JsWindowStage::OnGetSubWindow params not match or windowScene is nullptr!");
        errCode = WMError::WM_ERROR_INVALID_PARAM;
    }
    AsyncTask::CompleteCallback complete =
        [=](NativeEngine& engine, AsyncTask& task, int32_t status) {
            if (errCode != WMError::WM_OK) {
                task.Reject(engine, CreateJsError(engine, static_cast<int32_t>(errCode), "Invalidate params."));
                return;
            }
            std::vector<sptr<Window>> subWindowVec = windowScene_->GetSubWindow();
            task.Resolve(engine, CreateJsSubWindowArrayObject(engine, subWindowVec));
            WLOGFI("JsWindowStage OnGetSubWindow success");
        };
    NativeValue* callback = (info.argc == 0) ? nullptr :
        (info.argv[0]->TypeOf() == NATIVE_FUNCTION ? info.argv[0] : nullptr);
    NativeValue* result = nullptr;
    AsyncTask::Schedule(
        engine, CreateAsyncTaskWithLastParam(engine, callback, nullptr, std::move(complete), &result));
    return result;
}

NativeValue* CreateJsWindowStage(NativeEngine& engine,
    std::shared_ptr<Rosen::WindowScene> windowScene)
{
    WLOGFI("JsWindowStage::CreateJsWindowStage is called");
    NativeValue* objValue = engine.CreateObject();
    NativeObject* object = ConvertNativeValueTo<NativeObject>(objValue);

    std::unique_ptr<JsWindowStage> jsWindowStage = std::make_unique<JsWindowStage>(windowScene);
    object->SetNativePointer(jsWindowStage.release(), JsWindowStage::Finalizer, nullptr);

    BindNativeFunction(engine,
        *object, "setUIContent", JsWindowStage::SetUIContent);
    BindNativeFunction(engine,
        *object, "loadContent", JsWindowStage::LoadContent);
    BindNativeFunction(engine,
        *object, "getMainWindow", JsWindowStage::GetMainWindow);
    BindNativeFunction(engine,
        *object, "getWindowMode", JsWindowStage::GetWindowMode);
    BindNativeFunction(engine,
        *object, "createSubWindow", JsWindowStage::CreateSubWindow);
    BindNativeFunction(engine,
        *object, "getSubWindow", JsWindowStage::GetSubWindow);
    BindNativeFunction(engine, *object, "on", JsWindowStage::On);
    BindNativeFunction(engine, *object, "off", JsWindowStage::Off);

    return objValue;
}
}  // namespace Rosen
}  // namespace OHOS
