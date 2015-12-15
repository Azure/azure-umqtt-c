// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif // __cplusplus

#include "mqttconst.h"

typedef struct MQTT_MESSAGE_TAG* MQTT_MESSAGE_HANDLE;

extern MQTT_MESSAGE_HANDLE mqttmessage_create(uint16_t packetId, const char* topicName, QOS_VALUE qosValue, const uint8_t* appMsg, size_t appMsgLength);
extern void mqttmessage_destroyMessage(MQTT_MESSAGE_HANDLE handle);
extern MQTT_MESSAGE_HANDLE mqttmessage_clone(MQTT_MESSAGE_HANDLE handle);

extern uint16_t mqttmessage_getPacketId(MQTT_MESSAGE_HANDLE handle);
extern const char* mqttmessage_getTopicName(MQTT_MESSAGE_HANDLE handle);
extern QOS_VALUE mqttmessage_getQosType(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmessage_getIsDuplicateMsg(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmessage_getIsRetained(MQTT_MESSAGE_HANDLE handle);
extern int mqttmessage_setIsDuplicateMsg(MQTT_MESSAGE_HANDLE handle, bool duplicateMsg);
extern int mqttmessage_setIsRetained(MQTT_MESSAGE_HANDLE handle, bool retainMsg);
extern const APP_PAYLOAD* mqttmessage_getApplicationMsg(MQTT_MESSAGE_HANDLE handle);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MQTT_MESSAGE_H