#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "gballoc.h"
#include "crt_abstractions.h"
#include "control_packet.h"
#include "buffer_.h"
#include "data_byte_util.h"

#define USERNAME_FLAG                       0x80
#define PASSWORD_FLAG                       0x40
#define WILL_RETAIN_FLAG                    0x20
#define WILL_QOS_FLAG_                      0x18
#define WILL_FLAG_FLAG                      0x04
#define CLEAN_SESSION_FLAG                  0x02

#define PUBLISH_DUP_FLAG                    0x8
#define PUBLISH_QOS_EXACTLY_ONCE            0x4
#define PUBLISH_QOS_AT_LEAST_ONCE           0x2
#define PUBLISH_QOS_RETAIN                  0x1

#define PACKET_TYPE_MASK                    0xF0
#define PACKET_TYPE_BYTE(p)                 ((BYTE)(((unsigned int)(p)) & 0xf0))
#define FLAG_VALUE_BYTE(p)                  ((BYTE)(((unsigned int)(p)) & 0xf))

#define PROTOCOL_NUMBER                     4
#define CONN_FLAG_BYTE_OFFSET               7

#define CONNECT_FIXED_HEADER_SIZE           2
#define CONNECT_VARIABLE_HEADER_SIZE        10
#define SUBSCRIBE_FIXED_HEADER_FLAG         0x2
#define UNSUBSCRIBE_FIXED_HEADER_FLAG       0x2

typedef struct PUBLISH_HEADER_INFO_TAG
{
    const char* topicName;
    int packetId;
    const char* msgBuffer;
    QOS_VALUE qualityOfServiceValue;
} PUBLISH_HEADER_INFO;

static int addListItemsToUnsubscribePacket(BUFFER_HANDLE ctrlPacket, const char** payloadList, size_t payloadCount)
{
    int result = 0;
    if (payloadList == NULL || ctrlPacket == NULL)
    {
        result = __LINE__;
    }
    else
    {
        for (size_t index = 0; index < payloadCount && result == 0; index++)
        {
            // Add the Payload
            size_t offsetLen = BUFFER_length(ctrlPacket);
            size_t topicLen = strlen(payloadList[index]);
            if (BUFFER_enlarge(ctrlPacket, topicLen + 2) != 0)
            {
                result = __LINE__;
            }
            else
            {
                BYTE* iterator = BUFFER_u_char(ctrlPacket);
                iterator += offsetLen;
                byteutil_writeUTF(&iterator, payloadList[index], topicLen);
            }
        }
    }
    return result;
}

static int addListItemsToSubscribePacket(BUFFER_HANDLE ctrlPacket, SUBSCRIBE_PAYLOAD* payloadList, size_t payloadCount)
{
    int result = 0;
    if (payloadList == NULL || ctrlPacket == NULL)
    {
        result = __LINE__;
    }
    else
    {
        for (size_t index = 0; index < payloadCount && result == 0; index++)
        {
            // Add the Payload
            size_t offsetLen = BUFFER_length(ctrlPacket);
            size_t topicLen = strlen(payloadList[index].subscribeTopic);
            if (BUFFER_enlarge(ctrlPacket, topicLen + 2 + 1) != 0)
            {
                result = __LINE__;
            }
            else
            {
                BYTE* iterator = BUFFER_u_char(ctrlPacket);
                iterator += offsetLen;
                byteutil_writeUTF(&iterator, payloadList[index].subscribeTopic, topicLen);
                *iterator = payloadList[index].qosReturn;
            }
        }
    }
    return result;
}

static BUFFER_HANDLE constructPublishReply(CONTROL_PACKET_TYPE type, int flags, int packetId)
{
    BUFFER_HANDLE result = BUFFER_new();
    if (result != NULL)
    {
        if (BUFFER_pre_build(result, 4) != 0)
        {
            BUFFER_delete(result);
            result = NULL;
        }
        else
        {
            BYTE* iterator = BUFFER_u_char(result);
            if (iterator == NULL)
            {
                BUFFER_delete(result);
                result = NULL;
            }
            else
            {
                *iterator = type | flags;
                iterator++;
                *iterator = 0x2;
                iterator++;
                byteutil_writeInt(&iterator, packetId);
            }
        }
    }
    return result;
}

static int constructFixedHeader(BUFFER_HANDLE ctrlPacket, CONTROL_PACKET_TYPE packetType, BYTE flags)
{
    int result;
    if (ctrlPacket == NULL)
    {
        return __LINE__;
    }
    else
    {
        size_t packetLen = BUFFER_length(ctrlPacket);
        BYTE remainSize[4] = { 0 };
        size_t index = 0;

        do
        {
            BYTE encode = packetLen % 128;
            packetLen /= 128;
            // if there are more data to encode, set the top bit of this byte
            if (packetLen > 0)
            {
                encode = encode |= 0x80;
            }
            remainSize[index++] = encode;
        } while (packetLen > 0);

        BUFFER_HANDLE fixedHeader = BUFFER_new();
        if (fixedHeader == NULL)
        {
            result = __LINE__;
        }
        else if (BUFFER_pre_build(fixedHeader, index + 1) != 0)
        {
            BUFFER_delete(fixedHeader);
            result = __LINE__;
        }
        else
        {
            BYTE* iterator = BUFFER_u_char(fixedHeader);
            *iterator = (packetType & 0xff) | ((flags & 0xff) << 8);
            iterator++;
            memcpy(iterator, remainSize, index);

            result = BUFFER_prepend(ctrlPacket, fixedHeader);
            BUFFER_delete(fixedHeader);
        }
    }
    return result;
}

static int constructVariableHeader(BUFFER_HANDLE ctrlPacket, CONTROL_PACKET_TYPE packetType, const void* headerData)
{
    int result = 0;
    switch (packetType)
    {
        case CONNECT_TYPE:
        {
            MQTTCLIENT_OPTIONS* mqttOptions = (MQTTCLIENT_OPTIONS*)headerData;
            if (BUFFER_enlarge(ctrlPacket, CONNECT_VARIABLE_HEADER_SIZE) != 0)
            {
                result = __LINE__;
            }
            else
            {
                BYTE* iterator = BUFFER_u_char(ctrlPacket);
                if (iterator == NULL)
                {
                    result = __LINE__;
                }
                else
                {
                    byteutil_writeUTF(&iterator, "MQTT", 4);
                    byteutil_writeByte(&iterator, PROTOCOL_NUMBER);
                    byteutil_writeByte(&iterator, 0); // Flags will be entered later
                    byteutil_writeInt(&iterator, mqttOptions->keepAliveInterval);
                    result = 0;
                }
            }
            break;
        }
        case PUBLISH_TYPE:
        {
            PUBLISH_HEADER_INFO* publishHeader = (PUBLISH_HEADER_INFO*)headerData;
            size_t topicLen = 0;
            size_t spaceLen = 0;
            size_t idLen = 2;

            size_t currLen = BUFFER_length(ctrlPacket);

            topicLen = strlen(publishHeader->topicName);
            spaceLen += 2;
            if (BUFFER_enlarge(ctrlPacket, topicLen + idLen + spaceLen) != 0)
            {
                result = __LINE__;
            }
            else
            {
                BYTE* iterator = BUFFER_u_char(ctrlPacket);
                if (iterator == NULL)
                {
                    result = __LINE__;
                }
                else
                {
                    iterator += currLen;
                    /* The Topic Name MUST be present as the first field in the PUBLISH Packet Variable header.It MUST be 792 a UTF-8 encoded string [MQTT-3.3.2-1] as defined in section 1.5.3.*/
                    byteutil_writeUTF(&iterator, publishHeader->topicName, topicLen);
                    if (publishHeader->qualityOfServiceValue != DELIVER_AT_MOST_ONCE)
                    {
                        byteutil_writeInt(&iterator, publishHeader->packetId);
                    }
                    result = 0;
                }
            }
            break;
        }
        case SUBSCRIBE_TYPE:
        case UNSUBSCRIBE_TYPE:
        {
            int* packetId = (int*)headerData;
            if (BUFFER_enlarge(ctrlPacket, 2) != 0)
            {
                result = __LINE__;
            }
            else
            {
                BYTE* iterator = BUFFER_u_char(ctrlPacket);
                if (iterator == NULL)
                {
                    result = __LINE__;
                }
                else
                {
                    byteutil_writeInt(&iterator, *packetId);
                    result = 0;
                }
            }
            break;
        }
    }
    return result;
}

static int constructConnPayload(BUFFER_HANDLE ctrlPacket, const MQTTCLIENT_OPTIONS* mqttOptions)
{
    int result = 0;
    if (mqttOptions == NULL || ctrlPacket == NULL)
    {
        result = __LINE__;
    }
    else
    {
        size_t clientLen = 0;
        size_t usernameLen = 0;
        size_t passwordLen = 0;
        size_t willMessageLen = 0;
        size_t willTopicLen = 0;
        size_t spaceLen = 0;

        if (mqttOptions->clientId != NULL)
        {
            spaceLen += 2;
            clientLen = strlen(mqttOptions->clientId);
        }
        if (mqttOptions->username != NULL)
        {
            spaceLen += 2;
            usernameLen = strlen(mqttOptions->username);
        }
        if (mqttOptions->password != NULL)
        {
            spaceLen += 2;
            passwordLen = strlen(mqttOptions->password);
        }
        if (mqttOptions->willMessage != NULL)
        {
            spaceLen += 2;
            willMessageLen = strlen(mqttOptions->willMessage);
        }
        if (mqttOptions->willTopic != NULL)
        { 
            spaceLen += 2;
            willTopicLen = strlen(mqttOptions->willTopic);
        }

        size_t currLen = BUFFER_length(ctrlPacket);
        size_t totalLen = clientLen + usernameLen + passwordLen + willMessageLen + willTopicLen + spaceLen;

        // Validate the Username & Password
        if (usernameLen > 0 && passwordLen == 0)
        {
            result = __LINE__;
        }
        else if (BUFFER_enlarge(ctrlPacket, totalLen) != 0)
        {
            result = __LINE__;
        }
        else
        {
            BYTE* packet = BUFFER_u_char(ctrlPacket);
            BYTE* iterator = packet;

            iterator += currLen;
            byteutil_writeUTF(&iterator, mqttOptions->clientId, clientLen);

            // TODO: Read on the Will Topic
            if (willMessageLen > 0)
            {
                packet[CONN_FLAG_BYTE_OFFSET] |= WILL_FLAG_FLAG;
                byteutil_writeUTF(&iterator, mqttOptions->willMessage, willMessageLen);
                packet[CONN_FLAG_BYTE_OFFSET] |= mqttOptions->qualityOfServiceValue;
            }
            if (willTopicLen > 0)
            {
                byteutil_writeUTF(&iterator, mqttOptions->willMessage, willMessageLen);
            }
            if (usernameLen > 0)
            {
                packet[CONN_FLAG_BYTE_OFFSET] |= USERNAME_FLAG|PASSWORD_FLAG;
                byteutil_writeUTF(&iterator, mqttOptions->username, usernameLen);
                byteutil_writeUTF(&iterator, mqttOptions->password, passwordLen);
            }
            // TODO: Get the rest of the flags
            if (mqttOptions->useCleanSession)
            {
                packet[CONN_FLAG_BYTE_OFFSET] |= CLEAN_SESSION_FLAG;
            }
        }
    }
    return result;
}

int ctrlpacket_connect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, const MQTTCLIENT_OPTIONS* mqttOptions)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_001: [If the parameters ioSendFn is NULL or mqttOptions is NULL then ctrlpacket_connect shall return a non-zero value.] */
    if (ioSendFn == NULL || mqttOptions == NULL)
    {
        result = __LINE__;
    }
    else
    {
        BUFFER_HANDLE connPacket = BUFFER_new();
        if (connPacket == NULL)
        {
            result = __LINE__;
        }
        else
        {
            // Add Variable Header Information
            /* Codes_SRS_CONTROL_PACKET_07_002: [ctrlpacket_connect shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
            if (constructVariableHeader(connPacket, CONNECT_TYPE, mqttOptions) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_CONTROL_PACKET_07_003: [ctrlpacket_connect shall use the function constructConnPayload to created the CONNECT payload and shall return a non-zero value on failure.] */
                if (constructConnPayload(connPacket, mqttOptions) != 0)
                {
                    result = __LINE__;
                }
                else
                {
                    if (constructFixedHeader(connPacket, CONNECT_TYPE, 0) != 0)
                    {
                        result = __LINE__;
                    }
                    else
                    {
                        /* Codes_SRS_CONTROL_PACKET_07_004: [ctrlpacket_connect shall call the ioSendFn callback and return a non-zero value on failure.] */
                        if (ioSendFn(BUFFER_u_char(connPacket), BUFFER_length(connPacket), callContext) != 0)
                        {
                            result = __LINE__;
                        }
                        else
                        {
                            /* Codes_SRS_CONTROL_PACKET_07_007: [ctrlpacket_connect shall return a non-zero value on any failure.] */
                            result = 0;
                        }
                    }
                }
            }
            /* Codes_SRS_CONTROL_PACKET_07_005: [ctrlpacket_connect shall call clean all data that has been allocated.] */
            BUFFER_delete(connPacket);
        }
    }
    /* Codes_SRS_CONTROL_PACKET_07_006: [ctrlpacket_connect shall return zero on successfull completion.] */
    return result;
}

int ctrlpacket_disconnect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_008: [If the parameters ioSendFn is NULL then ctrlpacket_disconnect shall return a non-zero value.] */
    if (ioSendFn == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_CONTROL_PACKET_07_009: [ctrlpacket_disconnect shall construct the MQTT DISCONNECT packet.] */
        BYTE disconnPacket[2] = { 0 };
        // Add the Fixed Header
        disconnPacket[0] = DISCONNECT_TYPE;
        disconnPacket[1] = 0;
        /* Codes_SRS_CONTROL_PACKET_07_010: [ctrlpacket_disconnect shall call the ioSendFn callback and return a non-zero value on failure.] */
        if (ioSendFn(disconnPacket, 2, callContext) != 0)
        {
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_011: [ctrlpacket_disconnect shall return a zero value on successful completion.] */
            result = 0;
        }
    }
    return result;
}

int ctrlpacket_publish(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, QOS_VALUE qosValue, bool duplicateMsg, bool serverRetain, int packetId, const char* topicName, const BYTE* msgBuffer, size_t buffLen)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_051: [If the parameters ioSendFn, topicName, or msgBuffer is NULL then ctrlpacket_publish shall return a non-zero value.] */
    if (ioSendFn == NULL || topicName == NULL || msgBuffer == NULL)
    {
        result = __LINE__;
    }
    else
    {
        PUBLISH_HEADER_INFO publishInfo = { 0 };
        publishInfo.topicName = topicName;
        publishInfo.packetId = packetId;
        publishInfo.msgBuffer = msgBuffer;
        publishInfo.qualityOfServiceValue = qosValue;

        BYTE headerFlags = 0;
        if (duplicateMsg) headerFlags |= PUBLISH_DUP_FLAG;
        if (serverRetain) headerFlags |= PUBLISH_QOS_RETAIN;
        if (qosValue != DELIVER_AT_MOST_ONCE)
        {
            if (qosValue == DELIVER_AT_LEAST_ONCE)
            {
                headerFlags |= PUBLISH_QOS_AT_LEAST_ONCE;
            }
            else
            {
                headerFlags |= PUBLISH_QOS_EXACTLY_ONCE;
            }
        }

        BUFFER_HANDLE publishPacket = BUFFER_new();
        if (publishPacket == NULL)
        {
            /* Codes_SRS_CONTROL_PACKET_07_054: [ctrlpacket_publish shall return a non-zero value on any failure.] */
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_052: [ctrlpacket_publish shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
            if (constructVariableHeader(publishPacket, PUBLISH_TYPE, &publishInfo) != 0)
            {
                result = __LINE__;
            }
            else
            {
                size_t payloadOffset = BUFFER_length(publishPacket);
                size_t buffLen = strlen(msgBuffer);
                if (BUFFER_enlarge(publishPacket, buffLen + 2) != 0)
                {
                    result = __LINE__;
                }
                else
                {
                    BYTE* iterator = BUFFER_u_char(publishPacket);
                    if (iterator == NULL)
                    {
                        result = __LINE__;
                    }
                    else
                    {
                        iterator += payloadOffset;
                        // Write Message
                        byteutil_writeUTF(&iterator, msgBuffer, buffLen);
                        /* Codes_SRS_CONTROL_PACKET_07_053: [ctrlpacket_publish shall use the function constructFixedHeader to create the MQTT PUBLISh fixed packet.] */
                        if (constructFixedHeader(publishPacket, PUBLISH_TYPE, headerFlags) != 0)
                        {
                            result = __LINE__;
                        }
                        else
                        {
                            /* Codes_SRS_CONTROL_PACKET_07_055: [ctrlpacket_publish shall call the ioSendFn callback and return a non-zero value on failure.] */
                            if (ioSendFn(BUFFER_u_char(publishPacket), BUFFER_length(publishPacket), callContext) != 0)
                            {
                                result = __LINE__;
                            }
                            else
                            {
                                /* Codes_SRS_CONTROL_PACKET_07_056: [ctrlpacket_publish shall return zero on successful completion.] */
                                result = 0;
                            }
                        }
                    }
                }
            }
            BUFFER_delete(publishPacket);
        }
    }
    return result;
}

int ctrlpacket_publishAck(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_012: [If the parameters ioSendFn is NULL then ctrlpacket_publishAck shall return a non-zero value.] */
    if (ioSendFn == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_CONTROL_PACKET_07_013: [ctrlpacket_publishAck shall use constructPublishReply to construct the MQTT PUBACK reply.] */
        /* Codes_SRS_CONTROL_PACKET_07_016: [if constructPublishReply fails then ctrlpacket_publishAck shall return a non-zero value.] */
        BUFFER_HANDLE pubAckPacket = constructPublishReply(PUBACK_TYPE, 0, packetId);
        if (pubAckPacket == NULL)
        {
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_015: [ctrlpacket_publishAck shall call the ioSendFn callback and return a non-zero value on failure.] */
            if (ioSendFn(BUFFER_u_char(pubAckPacket), BUFFER_length(pubAckPacket), callContext) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_CONTROL_PACKET_07_014: [ctrlpacket_publishAck shall return a zero value on successful completion.] */
                result = 0;
            }
            BUFFER_delete(pubAckPacket);
        }
    }
    return result;
}

int ctrlpacket_publishRecieved(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_017: [If the parameters ioSendFn is NULL then ctrlpacket_publishRecieved shall return a non-zero value.] */
    if (ioSendFn == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_CONTROL_PACKET_07_018: [ctrlpacket_publishRecieved shall use constructPublishReply to construct the MQTT PUBREC reply.] */
        /* Codes_SRS_CONTROL_PACKET_07_021: [if constructPublishReply fails then ctrlpacket_publishAck shall return a non-zero value.] */
        BUFFER_HANDLE pubRecPacket = constructPublishReply(PUBREC_TYPE, 0, packetId);
        if (pubRecPacket == NULL)
        {
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_020: [ctrlpacket_publishRecieved shall call the ioSendFn callback and return a non-zero value on failure.] */
            if (ioSendFn(BUFFER_u_char(pubRecPacket), BUFFER_length(pubRecPacket), callContext) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_CONTROL_PACKET_07_019: [ctrlpacket_publishRecieved shall return a zero value on successful completion.] */
                result = 0;
            }
            BUFFER_delete(pubRecPacket);
        }
    }
    return result;
}

int ctrlpacket_publishRelease(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_022: [If the parameters ioSendFn is NULL then ctrlpacket_publishRelease shall return a non-zero value.] */
    if (ioSendFn == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_CONTROL_PACKET_07_023: [ctrlpacket_publishRelease shall use constructPublishReply to construct the MQTT PUBREL reply.] */
        /* Codes_SRS_CONTROL_PACKET_07_026: [if constructPublishReply fails then ctrlpacket_publishRelease shall return a non-zero value.] */
        BUFFER_HANDLE pubRelPacket = constructPublishReply(PUBREL_TYPE, 2, packetId);
        if (pubRelPacket == NULL)
        {
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_025: [ctrlpacket_publishRelease shall call the ioSendFn callback and return a non-zero value on failure.] */
            if (ioSendFn(BUFFER_u_char(pubRelPacket), BUFFER_length(pubRelPacket), callContext) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_CONTROL_PACKET_07_024: [ctrlpacket_publishRelease shall return a zero value on successful completion.] */
                result = 0;
            }
            BUFFER_delete(pubRelPacket);
        }
    }
    return result;
}

int ctrlpacket_publishComplete(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_027: [If the parameters ioSendFn is NULL then ctrlpacket_publishComplete shall return a non-zero value.] */
    if (ioSendFn == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_CONTROL_PACKET_07_028: [ctrlpacket_publishComplete shall use constructPublishReply to construct the MQTT PUBCOMP reply.] */
        /* Codes_SRS_CONTROL_PACKET_07_031: [if constructPublishReply fails then ctrlpacket_publishComplete shall return a non-zero value.] */
        BUFFER_HANDLE pubCompPacket = constructPublishReply(PUBCOMP_TYPE, 0, packetId);
        if (pubCompPacket == NULL)
        {
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_030: [ctrlpacket_publishComplete shall call the ioSendFn callback and return a non-zero value on failure.] */
            if (ioSendFn(BUFFER_u_char(pubCompPacket), BUFFER_length(pubCompPacket), callContext) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_CONTROL_PACKET_07_029: [ctrlpacket_publishComplete shall return a zero value on successful completion.] */
                result = 0;
            }
            BUFFER_delete(pubCompPacket);
        }
    }
    return result;
}

int ctrlpacket_ping(CTRL_PACKET_IO_SEND ioSendFn, void* callContext)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_032: [If the parameters ioSendFn is NULL then ctrlpacket_ping shall return a non-zero value.] */
    if (ioSendFn == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_CONTROL_PACKET_07_033: [ctrlpacket_ping shall construct the MQTT PING packet.] */
        BYTE pingPacket[2];

        // Add the Fixed Header
        pingPacket[0] = PINGREQ_TYPE;
        pingPacket[1] = 0;

        /* Codes_SRS_CONTROL_PACKET_07_035: [ctrlpacket_ping shall call the ioSendFn callback and return a non-zero value on failure.] */
        if (ioSendFn(pingPacket, 2, callContext) != 0)
        {
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_034: [ctrlpacket_ping shall return a zero value on successful completion.] */
            result = 0;
        }
    }
    return result;
}

int ctrlpacket_subscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, SUBSCRIBE_PAYLOAD* payloadList, size_t payloadCount)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_045: [If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_Subscribe shall return a non-zero value.] */
    if (ioSendFn == NULL || payloadList == NULL)
    {
        result = __LINE__;
    }
    else
    {
        BUFFER_HANDLE subscribePacket = BUFFER_new();
        if (subscribePacket == NULL)
        {
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_046: [ctrlpacket_Subscribe shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
            if (constructVariableHeader(subscribePacket, SUBSCRIBE_TYPE, &packetId) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_CONTROL_PACKET_07_047: [ctrlpacket_Subscribe shall use the function addListItemToSubscribePacket to create the MQTT SUBSCRIBE payload packet.] */
                if (addListItemsToSubscribePacket(subscribePacket, payloadList, payloadCount) != 0)
                {
                    result = __LINE__;
                }
                else
                {
                    /* Codes_SRS_CONTROL_PACKET_07_048: [ctrlpacket_Subscribe shall use the function constructFixedHeader to create the MQTT SUBSCRIBE fixed packet.] */
                    if (constructFixedHeader(subscribePacket, SUBSCRIBE_TYPE, SUBSCRIBE_FIXED_HEADER_FLAG) != 0)
                    {
                        result = __LINE__;
                    }
                    else
                    {
                        /* Codes_SRS_CONTROL_PACKET_07_049: [ctrlpacket_Subscribe shall call the ioSendFn callback and return a non-zero value on failure.] */
                        if (ioSendFn(BUFFER_u_char(subscribePacket), BUFFER_length(subscribePacket), callContext) != 0)
                        {
                            result = __LINE__;
                        }
                        else
                        {
                            /* Codes_SRS_CONTROL_PACKET_07_050: [ctrlpacket_Subscribe shall return a zero value on successful completion.] */
                            result = 0;
                        }
                    }
                }
            }
            BUFFER_delete(subscribePacket);
        }
    }
    return result;
}

int ctrlpacket_unsubscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, const char** payloadList, size_t payloadCount)
{
    int result;
    /* Codes_SRS_CONTROL_PACKET_07_038: [If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_unsubscribe shall return a non-zero value.] */
    if (ioSendFn == NULL || payloadList == NULL || payloadCount == 0)
    {
        result = __LINE__;
    }
    else
    {
        BUFFER_HANDLE unsubPacket = BUFFER_new();
        if (unsubPacket == NULL)
        {
            /* Codes_SRS_CONTROL_PACKET_07_044: [ctrlpacket_unsubscribe shall return a non-zero value on any error encountered.] */
            result = __LINE__;
        }
        else
        {
            /* Codes_SRS_CONTROL_PACKET_07_039: [ctrlpacket_unsubscribe shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
            if (constructVariableHeader(unsubPacket, UNSUBSCRIBE_TYPE, &packetId) != 0)
            {
                result = __LINE__;
            }
            else
            {
                /* Codes_SRS_CONTROL_PACKET_07_040: [ctrlpacket_unsubscribe shall use the function addListItemToUnsubscribePacket to create the MQTT UNSUBSCRIBE payload packet.] */
                if (addListItemsToUnsubscribePacket(unsubPacket, payloadList, payloadCount) != 0)
                {
                    result = __LINE__;
                }
                else
                {
                    /* Codes_SRS_CONTROL_PACKET_07_041: [ctrlpacket_unsubscribe shall use the function constructFixedHeader to create the MQTT UNSUBSCRIBE fixed packet.] */
                    if (constructFixedHeader(unsubPacket, UNSUBSCRIBE_TYPE, UNSUBSCRIBE_FIXED_HEADER_FLAG) != 0)
                    {
                        result = __LINE__;
                    }
                    else
                    {
                        /* Codes_SRS_CONTROL_PACKET_07_042: [ctrlpacket_unsubscribe shall call the ioSendFn callback and return a non-zero value on failure.] */
                        if (ioSendFn(BUFFER_u_char(unsubPacket), BUFFER_length(unsubPacket), callContext) != 0)
                        {
                            result = __LINE__;
                        }
                        else
                        {
                            result = 0;
                        }
                    }
                }
            }
            BUFFER_delete(unsubPacket);
        }
    }
    /* Codes_SRS_CONTROL_PACKET_07_043: [ctrlpacket_unsubscribe shall return a zero value on successful completion.] */
    return result;
}

CONTROL_PACKET_TYPE ctrlpacket_processControlPacketType(BYTE pktByte, int* flags)
{
    CONTROL_PACKET_TYPE result;
    /* Codes_SRS_CONTROL_PACKET_07_036: [ctrlpacket_processControlPacketType shall retrieve the CONTROL_PACKET_TYPE from pktBytes variable.] */
    result = PACKET_TYPE_BYTE(pktByte);
    if (flags != NULL)
    {
        /* Codes_SRS_CONTROL_PACKET_07_037: [if the parameter flags is Non_Null then ctrlpacket_processControlPacketType shall retrieve the flag value from the pktBytes variable.] */
        *flags = FLAG_VALUE_BYTE(pktByte);
    }
    return result;
}
