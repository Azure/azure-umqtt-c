// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#include "macro_utils.h"
#include "map.h" 

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#include "mqttconst.h"

typedef void* MQTT_MESSAGE_HANDLE;

extern MQTT_MESSAGE_HANDLE mqttmsg_createMessage(BYTE packetId, const char* topicName, QOS_VALUE qosValue, const BYTE* appMsg, bool duplicateMsg, bool retainMsg);
extern void mqttmsg_destroyMessage(MQTT_MESSAGE_HANDLE handle);
extern MQTT_MESSAGE_HANDLE mqttmsg_clone(MQTT_MESSAGE_HANDLE handle);

extern BYTE mqttmsg_getPacketId(MQTT_MESSAGE_HANDLE handle);
extern const char* mqttmsg_getTopicName(MQTT_MESSAGE_HANDLE handle);
extern QOS_VALUE mqttmsg_getQosType(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmsg_isDuplicateMsg(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmsg_isRetained(MQTT_MESSAGE_HANDLE handle);
extern const BYTE* mqttmsg_applicationMsg(MQTT_MESSAGE_HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif // MQTT_MESSAGE_H