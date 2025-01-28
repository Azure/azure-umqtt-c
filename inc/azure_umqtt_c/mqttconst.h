// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTTCONST_H
#define MQTTCONST_H

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_macro_utils/macro_utils.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

// Define CONTROL_PACKET_TYPE without numeric constants
#define CONTROL_PACKET_TYPE_VALUES \
    CONNECT_TYPE, \
    CONNACK_TYPE, \
    PUBLISH_TYPE, \
    PUBACK_TYPE, \
    PUBREC_TYPE, \
    PUBREL_TYPE, \
    PUBCOMP_TYPE, \
    SUBSCRIBE_TYPE, \
    SUBACK_TYPE, \
    UNSUBSCRIBE_TYPE, \
    UNSUBACK_TYPE, \
    PINGREQ_TYPE, \
    PINGRESP_TYPE, \
    DISCONNECT_TYPE, \
    PACKET_TYPE_ERROR, \
    UNKNOWN_TYPE, \
    PACKET_INVALID1_TYPE, \
    PACKET_INVALID2_TYPE

MU_DEFINE_ENUM(CONTROL_PACKET_TYPE, CONTROL_PACKET_TYPE_VALUES)

// Define QOS_VALUE without numeric constants
#define QOS_VALUE_VALUES \
    DELIVER_AT_MOST_ONCE, \
    DELIVER_AT_LEAST_ONCE, \
    DELIVER_EXACTLY_ONCE, \
    DELIVER_FAILURE

MU_DEFINE_ENUM(QOS_VALUE, QOS_VALUE_VALUES)

// Map enum values to numeric constants
static const int CONTROL_PACKET_TYPE_NUMERIC_VALUES[] = {
    [CONNECT_TYPE] = 0x10,
    [CONNACK_TYPE] = 0x20,
    [PUBLISH_TYPE] = 0x30,
    [PUBACK_TYPE] = 0x40,
    [PUBREC_TYPE] = 0x50,
    [PUBREL_TYPE] = 0x60,
    [PUBCOMP_TYPE] = 0x70,
    [SUBSCRIBE_TYPE] = 0x80,
    [SUBACK_TYPE] = 0x90,
    [UNSUBSCRIBE_TYPE] = 0xA0,
    [UNSUBACK_TYPE] = 0xB0,
    [PINGREQ_TYPE] = 0xC0,
    [PINGRESP_TYPE] = 0xD0,
    [DISCONNECT_TYPE] = 0xE0,
    [PACKET_TYPE_ERROR] = 0xE1,
    [UNKNOWN_TYPE] = 0xE2,
    [PACKET_INVALID1_TYPE] = 0x00,
    [PACKET_INVALID2_TYPE] = 0xF0
};

static const int QOS_VALUE_NUMERIC_VALUES[] = {
    [DELIVER_AT_MOST_ONCE] = 0x00,
    [DELIVER_AT_LEAST_ONCE] = 0x01,
    [DELIVER_EXACTLY_ONCE] = 0x02,
    [DELIVER_FAILURE] = 0x80
};

typedef struct APP_PAYLOAD_TAG
{
    uint8_t* message;
    size_t length;
} APP_PAYLOAD;

typedef struct MQTT_CLIENT_OPTIONS_TAG
{
    char* clientId;
    char* willTopic;
    char* willMessage;
    char* username;
    char* password;
    uint16_t keepAliveInterval;
    bool messageRetain;
    bool useCleanSession;
    QOS_VALUE qualityOfServiceValue;
    bool log_trace;
} MQTT_CLIENT_OPTIONS;

typedef enum CONNECT_RETURN_CODE_TAG
{
    CONNECTION_ACCEPTED = 0x00,
    CONN_REFUSED_UNACCEPTABLE_VERSION = 0x01,
    CONN_REFUSED_ID_REJECTED = 0x02,
    CONN_REFUSED_SERVER_UNAVAIL = 0x03,
    CONN_REFUSED_BAD_USERNAME_PASSWORD = 0x04,
    CONN_REFUSED_NOT_AUTHORIZED = 0x05,
    CONN_REFUSED_UNKNOWN
} CONNECT_RETURN_CODE;

typedef struct CONNECT_ACK_TAG
{
    bool isSessionPresent;
    CONNECT_RETURN_CODE returnCode;
} CONNECT_ACK;

typedef struct SUBSCRIBE_PAYLOAD_TAG
{
    const char* subscribeTopic;
    QOS_VALUE qosReturn;
} SUBSCRIBE_PAYLOAD;

typedef struct SUBSCRIBE_ACK_TAG
{
    uint16_t packetId;
    QOS_VALUE* qosReturn;
    size_t qosCount;
} SUBSCRIBE_ACK;

typedef struct UNSUBSCRIBE_ACK_TAG
{
    uint16_t packetId;
} UNSUBSCRIBE_ACK;

typedef struct PUBLISH_ACK_TAG
{
    uint16_t packetId;
} PUBLISH_ACK;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // MQTTCONST_H
