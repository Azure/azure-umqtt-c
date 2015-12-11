// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONNECT_PACKET_H
#define CONNECT_PACKET_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "mqttconst.h"

typedef int(*CTRL_PACKET_IO_SEND)(const BYTE* data, size_t length, void* callContext);

extern int ctrlpacket_connect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, const MQTTCLIENT_OPTIONS* mqttOptions);
extern int ctrlpacket_disconnect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext);

extern int ctrlpacket_publish(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, QOS_VALUE qosValue, bool duplicateMsg, bool serverRetain, int packetId, const char* topicName, const BYTE* msgBuffer, size_t buffLen);
extern int ctrlpacket_publishAck(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
extern int ctrlpacket_publishRecieved(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
extern int ctrlpacket_publishRelease(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
extern int ctrlpacket_publishComplete(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);

extern int ctrlpacket_subscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, SUBSCRIBE_PAYLOAD* payloadList, size_t payloadCount);
extern int ctrlpacket_unsubscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, const char** payloadList, size_t payloadCount);

extern int ctrlpacket_ping(CTRL_PACKET_IO_SEND ioSendFn, void* callContext);

extern CONTROL_PACKET_TYPE ctrlpacket_processControlPacketType(BYTE pktByte, int* flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CONNECT_PACKET_H
