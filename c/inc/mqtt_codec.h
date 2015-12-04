// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_CODEC_H
#define MQTT_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xio.h"
#include "mqttconst.h"
#include "buffer_.h"

typedef struct MQTTCODEC_INSTANCE_TAG* MQTTCODEC_HANDLE;

typedef void(*ON_PACKET_COMPLETE_CALLBACK)(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData);

extern MQTTCODEC_HANDLE mqttcodec_init(ON_PACKET_COMPLETE_CALLBACK packetComplete, void* callContext);
extern void mqttcodec_deinit(MQTTCODEC_HANDLE handle);

extern int mqttcodec_bytesReceived(MQTTCODEC_HANDLE handle, const void* buffer, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // MQTT_CODEC_H
