// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

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

typedef struct MQTT_CLIENT_TAG* MQTT_CLIENT_HANDLE;

#define MQTT_CLIENT_EVENT_VALUES    \
    MQTT_CLIENT_ON_CONNACK,          \
    MQTT_CLIENT_ON_PUBLISH_ACK,      \
    MQTT_CLIENT_ON_PUBLISH_RECV,     \
    MQTT_CLIENT_ON_PUBLISH_REL,      \
    MQTT_CLIENT_ON_PUBLISH_COMP,     \
    MQTT_CLIENT_ON_SUBSCRIBE_ACK,    \
    MQTT_CLIENT_ON_UNSUBSCRIBE_ACK,  \
    MQTT_CLIENT_ON_DISCONNECT,       \
    MQTT_CLIENT_ON_ERROR

DEFINE_ENUM(MQTT_CLIENT_EVENT_RESULT, MQTT_CLIENT_EVENT_VALUES);

typedef void(*ON_MQTT_OPERATION_CALLBACK)(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_RESULT actionResult, const void* msgInfo, void* callbackCtx);
typedef void(*ON_MQTT_MESSAGE_RECV_CALLBACK)(MQTT_MESSAGE_HANDLE msgHandle, void* callbackCtx);

extern MQTT_CLIENT_HANDLE mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* callbackCtx, LOGGER_LOG logger);
extern void mqtt_client_deinit(MQTT_CLIENT_HANDLE handle);

extern int mqtt_client_connect(MQTT_CLIENT_HANDLE handle, XIO_HANDLE xioHandle, MQTT_CLIENT_OPTIONS* mqttOptions);
extern int mqtt_client_disconnect(MQTT_CLIENT_HANDLE handle);

extern int mqtt_client_subscribe(MQTT_CLIENT_HANDLE handle, uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count);
extern int mqtt_client_unsubscribe(MQTT_CLIENT_HANDLE handle, uint16_t packetId, const char** unsubscribeList, size_t count);

extern int mqtt_client_publish(MQTT_CLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle);

extern void mqtt_client_dowork(MQTT_CLIENT_HANDLE handle);

extern void mqtt_client_set_trace(MQTT_CLIENT_HANDLE handle, bool traceOn, bool rawBytesOn);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MQTTCLIENT_H
