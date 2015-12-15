// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif // __cplusplus

#include "xio.h"
#include "mqttconst.h"
#include "xlogging.h"
#include "macro_utils.h"
#include "mqtt_message.h"
#include "list.h"

typedef struct MQTTCLIENT_DATA_INSTANCE_TAG* MQTTCLIENT_HANDLE;

#define MQTTCLIENT_ACTION_VALUES    \
    MQTTCLIENT_ON_CONNACK,          \
    MQTTCLIENT_ON_PUBLISH_ACK,      \
    MQTTCLIENT_ON_PUBLISH_RECV,     \
    MQTTCLIENT_ON_PUBLISH_REL,      \
    MQTTCLIENT_ON_PUBLISH_COMP,     \
    MQTTCLIENT_ON_SUBSCRIBE_ACK,    \
    MQTTCLIENT_ON_UNSUBSCRIBE_ACK,  \
    MQTTCLIENT_ON_DISCONNECT

DEFINE_ENUM(MQTTCLIENT_ACTION_RESULT, MQTTCLIENT_ACTION_VALUES);

typedef int(*ON_MQTT_OPERATION_CALLBACK)(MQTTCLIENT_ACTION_RESULT actionResult, void* msgInfo, void* callbackCtx);
typedef int(*ON_MQTT_MESSAGE_RECV_CALLBACK)(MQTT_MESSAGE_HANDLE msgHandle, void* callbackCtx);

extern MQTTCLIENT_HANDLE mqttclient_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* callCtx, LOGGER_LOG logger);
extern void mqttclient_deinit(MQTTCLIENT_HANDLE handle);

extern int mqttclient_connect(MQTTCLIENT_HANDLE handle, XIO_HANDLE ioHandle, MQTTCLIENT_OPTIONS* mqttOptions);
extern void mqttclient_disconnect(MQTTCLIENT_HANDLE handle);

extern int mqttclient_subscribe(MQTTCLIENT_HANDLE handle, uint8_t packetId, SUBSCRIBE_PAYLOAD* payloadList, size_t payloadCount);
extern int mqttclient_unsubscribe(MQTTCLIENT_HANDLE handle, uint8_t packetId, const char** unsubscribeTopic, size_t payloadCount);

extern int mqttclient_publish(MQTTCLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle);

extern void mqttclient_dowork(MQTTCLIENT_HANDLE handle);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MQTTCLIENT_H
