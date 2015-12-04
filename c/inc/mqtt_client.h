// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "xio.h"
#include "mqttconst.h"
#include "iot_logging.h"
#include "macro_utils.h"
#include "mqtt_message.h"
#include "list.h"

typedef struct MQTTCLIENT_DATA_INSTANCE_TAG* MQTTCLIENT_HANDLE;

#define MQTTCLIENT_ACTION_VALUES    \
    MQTTCLIENT_ON_CONNACT,          \
    MQTTCLIENT_ON_SUBSCRIBE,        \
    MQTTCLIENT_ON_DISCONNECT

DEFINE_ENUM(MQTTCLIENT_ACTION_RESULT, MQTTCLIENT_ACTION_VALUES);

typedef int(*ON_MQTT_OPERATION_CALLBACK)(MQTTCLIENT_ACTION_RESULT actionResult, void* context);
typedef void(*ON_MQTT_MESSAGE_RECV_CALLBACK)(MQTT_MESSAGE_HANDLE msgHandle, void* context);

extern MQTTCLIENT_HANDLE mqttclient_init(LOGGER_LOG logger, ON_MQTT_OPERATION_CALLBACK opCallback, ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, void* callCtx);
extern void mqttclient_deinit(MQTTCLIENT_HANDLE handle);

extern int mqttclient_connect(MQTTCLIENT_HANDLE handle, XIO_HANDLE ioHandle, MQTTCLIENT_OPTIONS* mqttOptions);
extern void mqttclient_disconnect(MQTTCLIENT_HANDLE handle);

extern int mqttclient_subscribe(MQTTCLIENT_HANDLE handle, BYTE packetId, const char* subscribeTopic, QOS_VALUE qosValue);
extern int mqttclient_unsubscribe(MQTTCLIENT_HANDLE handle, BYTE packetId, const char* unsubscribeTopic, QOS_VALUE qosValue);

extern int mqttclient_publish(MQTTCLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle);
extern void mqttclient_dowork(MQTTCLIENT_HANDLE handle);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // MQTTCLIENT_H
