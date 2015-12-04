// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONNECT_PACKET_H
#define CONNECT_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xio.h"
#include "mqttconst.h"
#include "buffer_.h"

typedef int(*CTRL_PACKET_IO_SEND)(BUFFER_HANDLE handle, void* callContext);

extern int ctrlpacket_connect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, const MQTTCLIENT_OPTIONS* mqttOptions);
extern int ctrlpacket_disconnect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext);

extern int ctrlpacket_publish(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, QOS_VALUE qosValue, bool duplicateMsg, bool serverRetain, int packetId, const char* topicName, const unsigned char* msgBuffer);
extern int ctrlpacket_publishAck(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
extern int ctrlpacket_publishRecieved(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
extern int ctrlpacket_publishRelease(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
extern int ctrlpacket_publishComplete(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);

extern int ctrlpacket_subscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, const char* subscribeTopic, QOS_VALUE qosValue);
extern int ctrlpacket_unsubscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, const char* unsubscribeTopic, QOS_VALUE qosValue);

extern int ctrlpacket_ping(CTRL_PACKET_IO_SEND ioSendFn, void* callContext);

extern CONTROL_PACKET_TYPE ctrlpacket_processControlPacketType(BYTE pktByte, int* flags);
extern int ctrlpacket_processVariableHeader(CONTROL_PACKET_TYPE type, BUFFER_HANDLE packetData);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CONNECT_PACKET_H
