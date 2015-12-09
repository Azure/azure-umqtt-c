// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif // __cplusplus

#include "mqttconst.h"

typedef struct MQTT_MESSAGE* MQTT_MESSAGE_HANDLE;

extern MQTT_MESSAGE_HANDLE mqttmessage_create(PACKET_ID packetId, const char* topicName, QOS_VALUE qosValue, const BYTE* appMsg, size_t appLength, bool duplicateMsg, bool retainMsg);
extern void mqttmessage_destroyMessage(MQTT_MESSAGE_HANDLE handle);
extern MQTT_MESSAGE_HANDLE mqttmessage_clone(MQTT_MESSAGE_HANDLE handle);

extern PACKET_ID mqttmessage_getPacketId(MQTT_MESSAGE_HANDLE handle);
extern const char* mqttmessage_getTopicName(MQTT_MESSAGE_HANDLE handle);
extern QOS_VALUE mqttmessage_getQosType(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmessage_isDuplicateMsg(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmessage_isRetained(MQTT_MESSAGE_HANDLE handle);
extern const BYTE* mqttmessage_applicationMsg(MQTT_MESSAGE_HANDLE handle, size_t* msgLen);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MQTT_MESSAGE_H