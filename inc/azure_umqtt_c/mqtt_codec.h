// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_CODEC_H
#define MQTT_CODEC_H

#ifdef __cplusplus
#include <cstdint>
#include <cstdbool>
extern "C" {
#else
#include <stdint.h>
#include <stdbool.h>
#endif // __cplusplus

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_umqtt_c/mqttconst.h"

typedef struct MQTTCODEC_INSTANCE_TAG* MQTTCODEC_HANDLE;

typedef void(*ON_PACKET_COMPLETE_CALLBACK)(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData);

extern MQTTCODEC_HANDLE mqtt_codec_create(ON_PACKET_COMPLETE_CALLBACK packetComplete, void* callbackCtx);
extern void mqtt_codec_destroy(MQTTCODEC_HANDLE handle);

extern BUFFER_HANDLE mqtt_codec_connect(const MQTT_CLIENT_OPTIONS* mqttOptions);
extern BUFFER_HANDLE mqtt_codec_disconnect();
extern BUFFER_HANDLE mqtt_codec_publish(QOS_VALUE qosValue, bool duplicateMsg, bool serverRetain, uint16_t packetId, const char* topicName, const uint8_t* msgBuffer, size_t buffLen);
extern BUFFER_HANDLE mqtt_codec_publishAck(uint16_t packetId);
extern BUFFER_HANDLE mqtt_codec_publishReceived(uint16_t packetId);
extern BUFFER_HANDLE mqtt_codec_publishRelease(uint16_t packetId);
extern BUFFER_HANDLE mqtt_codec_publishComplete(uint16_t packetId);
extern BUFFER_HANDLE mqtt_codec_ping();
extern BUFFER_HANDLE mqtt_codec_subscribe(uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count);
extern BUFFER_HANDLE mqtt_codec_unsubscribe(uint16_t packetId, const char** unsubscribeList, size_t count);

extern int mqtt_codec_bytesReceived(MQTTCODEC_HANDLE handle, const unsigned char* buffer, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MQTT_CODEC_H
