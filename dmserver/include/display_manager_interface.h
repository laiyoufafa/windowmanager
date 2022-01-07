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

#ifndef FOUNDATION_DMSERVER_DISPLAY_MANAGER_INTERFACE_H
#define FOUNDATION_DMSERVER_DISPLAY_MANAGER_INTERFACE_H

#include <iremote_broker.h>

#include <surface.h>

#include "display_info.h"

#include "virtual_display_info.h"
// #include "pixel_map.h"

namespace OHOS::Rosen {
class IDisplayManager : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.IDisplayManager");

    enum {
        TRANS_ID_GET_DEFAULT_DISPLAY_ID = 0,
        TRANS_ID_GET_DISPLAY_BY_ID,
        TRANS_ID_CREATE_VIRTUAL_DISPLAY,
        TRANS_ID_DESTROY_VIRTUAL_DISPLAY,
        // TODO: fix me
        // TRANS_ID_GET_DISPLAY_SNAPSHOT,
    };

    virtual DisplayId GetDefaultDisplayId() = 0;
    virtual DisplayInfo GetDisplayInfoById(DisplayId displayId) = 0;

    virtual DisplayId CreateVirtualDisplay(const VirtualDisplayInfo &virtualDisplayInfo,
        sptr<Surface> surface) = 0;
    virtual bool DestroyVirtualDisplay(DisplayId displayId) = 0;

    // TODO: fix me
    // virtual sptr<Media::PixelMap> GetDispalySnapshot(DisplayId displayId) = 0;
};
} // namespace OHOS::Rosen

#endif // FOUNDATION_DMSERVER_DISPLAY_MANAGER_INTERFACE_H