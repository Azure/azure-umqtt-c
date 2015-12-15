// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "gballoc.h"

#include "mqtt_client.h"
#include "mqtt_codec.h"
#include "data_byte_util.h"
#include "platform.h"
#include "tickcounter.h"

#define KEEP_ALIVE_BUFFER_SEC           2
#define VARIABLE_HEADER_OFFSET          2
#define RETAIN_FLAG_MASK                0x1
#define QOS_LEAST_ONCE_FLAG_MASK        0x2
#define QOS_EXACTLY_ONCE_FLAG_MASK      0x4
#define DUPLICATE_FLAG_MASK             0x8

typedef struct MQTTCLIENT_DATA_INSTANCE_TAG
{
    XIO_HANDLE ioHandle;
    MQTTCODEC_HANDLE codec_handle;
    CONTROL_PACKET_TYPE packetState;
    LOGGER_LOG logFunc;
    TICK_COUNTER_HANDLE packetTickCntr;
    uint64_t packetSendTimeMs;
    ON_MQTT_OPERATION_CALLBACK fnOperationCallback;
    ON_MQTT_MESSAGE_RECV_CALLBACK fnMessageRecv;
    void* ctx;
    QOS_VALUE qosValue;
    int keepAliveInterval;
} MQTTCLIENT_DATA_INSTANCE;


