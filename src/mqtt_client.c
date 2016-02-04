// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "gballoc.h"

#include "mqtt_client.h"
#include "mqtt_codec.h"
#include "platform.h"
#include "tickcounter.h"
#include "crt_abstractions.h"

#define KEEP_ALIVE_BUFFER_SEC           2
#define VARIABLE_HEADER_OFFSET          2
#define RETAIN_FLAG_MASK                0x1
#define QOS_LEAST_ONCE_FLAG_MASK        0x2
#define QOS_EXACTLY_ONCE_FLAG_MASK      0x4
#define DUPLICATE_FLAG_MASK             0x8
#define CONNECT_PACKET_MASK             0xf0

static const char* FORMAT_HEX_CHAR = "0x%02x ";

typedef struct MQTT_CLIENT_TAG
{
    XIO_HANDLE xioHandle;
    MQTTCODEC_HANDLE codec_handle;
    CONTROL_PACKET_TYPE packetState;
    LOGGER_LOG logFunc;
    TICK_COUNTER_HANDLE packetTickCntr;
    uint64_t packetSendTimeMs;
    ON_MQTT_OPERATION_CALLBACK fnOperationCallback;
    ON_MQTT_MESSAGE_RECV_CALLBACK fnMessageRecv;
    void* ctx;
    QOS_VALUE qosValue;
    uint16_t keepAliveInterval;
    MQTT_CLIENT_OPTIONS mqttOptions;
    bool clientConnected;
    bool socketConnected;
    bool logTrace;
    bool rawBytesTrace;
} MQTT_CLIENT;

static uint16_t byteutil_read_uint16(uint8_t** buffer)
{
    uint16_t result = 0;
    if (buffer != NULL)
    {
        result = 256 * ((uint8_t)(**buffer)) + (uint8_t)(*(*buffer + 1));
        *buffer += 2; // Move the ptr
    }
    return result;
}

static char* byteutil_readUTF(uint8_t** buffer, size_t* byteLen)
{
    char* result = NULL;
    if (buffer != NULL)
    {
        // Get the length of the string
        int len = byteutil_read_uint16(buffer);
        if (len > 0)
        {
            result = (char*)malloc(len + 1);
            if (result != NULL)
            {
                (void)memcpy(result, *buffer, len);
                result[len] = '\0';
                *buffer += len;
                if (byteLen != NULL)
                {
                    *byteLen = len;
                }
            }
        }
    }
    return result;
}

static uint8_t byteutil_readByte(uint8_t** buffer)
{
    uint8_t result = 0;
    if (buffer != NULL)
    {
        result = **buffer;
        (*buffer)++;
    }
    return result;
}

static void sendComplete(void* context, IO_SEND_RESULT send_result)
{
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)context;
    if (mqttData != NULL && mqttData->fnOperationCallback != NULL)
    {
        if (mqttData->packetState == DISCONNECT_TYPE)
        {
            /*Codes_SRS_MQTT_CLIENT_07_032: [If the actionResult parameter is of type MQTT_CLIENT_ON_DISCONNECT or MQTT_CLIENT_ON_ERROR the the msgInfo value shall be NULL.]*/
            mqttData->fnOperationCallback(mqttData, MQTT_CLIENT_ON_DISCONNECT, NULL, mqttData->ctx);

            // close the xio
            (void)xio_close(mqttData->xioHandle, NULL, mqttData->ctx);
            mqttData->socketConnected = false;
            mqttData->clientConnected = false;
        }
    }
}

static const char* retrievePacketType(CONTROL_PACKET_TYPE packet)
{
    switch (packet&CONNECT_PACKET_MASK)
    {
        case CONNECT_TYPE: return "CONNECT";
        case CONNACK_TYPE:  return "CONNACK";
        case PUBLISH_TYPE:  return "PUBLISH";
        case PUBACK_TYPE:  return "PUBACK";
        case PUBREC_TYPE:  return "PUBREC";
        case PUBREL_TYPE:  return "PUBREL";
        case SUBSCRIBE_TYPE:  return "SUBSCRIBE";
        case SUBACK_TYPE:  return "SUBACK";
        case UNSUBSCRIBE_TYPE:  return "UNSUBSCRIBE";
        case UNSUBACK_TYPE:  return "UNSUBACK";
        case PINGREQ_TYPE:  return "PINGREQ";
        case PINGRESP_TYPE:  return "PINGRESP";
        case DISCONNECT_TYPE:  return "DISCONNECT";
        default:
        case PACKET_TYPE_ERROR:
        case UNKNOWN_TYPE:
            return "UNKNOWN";
    }
}

static void logOutgoingingMsgTrace(MQTT_CLIENT* clientData, const uint8_t* data, size_t length)
{
    if (clientData != NULL && data != NULL && length > 0 && clientData->logTrace)
    {
        LOG(clientData->logFunc, 0, "-> %s: ", retrievePacketType((unsigned char)data[0]));
        for (size_t index = 0; index < length; index++)
        {
            LOG(clientData->logFunc, 0, (char*)FORMAT_HEX_CHAR, (unsigned char)data[index]);
        }
        LOG(clientData->logFunc, LOG_LINE, "");
    }
}

static void logIncomingMsgTrace(MQTT_CLIENT* clientData, CONTROL_PACKET_TYPE packet, int flags, const uint8_t* data, size_t length)
{
    if (clientData != NULL && data != NULL && length > 0 && clientData->logTrace)
    {
        LOG(clientData->logFunc, 0, "<- %s: 0x%02x 0x%02x ", retrievePacketType((unsigned char)packet), (unsigned char)(packet | flags), length);
        for (size_t index = 0; index < length; index++)
        {
            LOG(clientData->logFunc, 0, (char*)FORMAT_HEX_CHAR, (unsigned char)data[index]);
        }
        LOG(clientData->logFunc, LOG_LINE, "");
    }
}

static int sendPacketItem(MQTT_CLIENT* clientData, const int8_t* data, size_t length)
{
    logOutgoingingMsgTrace(clientData, data, length);

    (void)tickcounter_get_current_ms(clientData->packetTickCntr, &clientData->packetSendTimeMs);
    int result = xio_send(clientData->xioHandle, data, length, sendComplete, clientData);
    if (result != 0)
    {
        LOG(clientData->logFunc, LOG_LINE, "%d: Failure sending control packet data", result);
        result = __LINE__;
    }
    return result;
}

static void onOpenComplete(void* context, IO_OPEN_RESULT open_result)
{
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)context;
    if (mqttData != NULL)
    {
        if (open_result == IO_OPEN_OK)
        {
            mqttData->packetState = CONNECT_TYPE;
            mqttData->socketConnected = true;
            // Send the Connect packet
            BUFFER_HANDLE connPacket = mqtt_codec_connect(&mqttData->mqttOptions);
            if (connPacket == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_connect failed");
            }
            else
            {
                /*Codes_SRS_MQTT_CLIENT_07_009: [On success mqtt_client_connect shall send the MQTT CONNECT to the endpoint.]*/
                if (sendPacketItem(mqttData, BUFFER_u_char(connPacket), BUFFER_length(connPacket)) != 0)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                    LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_connect failed");
                }
                BUFFER_delete(connPacket);
            }
        }
        else if (open_result == IO_OPEN_ERROR)
        {
            (void)mqttData->fnOperationCallback(mqttData, MQTT_CLIENT_ON_ERROR, NULL, mqttData->ctx);
        }
    }
}

static void onBytesReceived(void* context, const unsigned char* buffer, size_t size)
{
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)context;
    if (mqttData != NULL)
    {
        if (mqtt_codec_bytesReceived(mqttData->codec_handle, buffer, size) != 0)
        {
            if (mqttData->fnOperationCallback)
            {
                mqttData->fnOperationCallback(mqttData, MQTT_CLIENT_ON_ERROR, NULL, mqttData->ctx);
            }
        }
    }
}

static void onIoError(void* context)
{
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)context;
    if (mqttData != NULL && mqttData->fnOperationCallback)
    {
        /*Codes_SRS_MQTT_CLIENT_07_032: [If the actionResult parameter is of type MQTT_CLIENT_ON_DISCONNECT or MQTT_CLIENT_ON_ERROR the the msgInfo value shall be NULL.]*/
        mqttData->fnOperationCallback(mqttData, MQTT_CLIENT_ON_ERROR, NULL, mqttData->ctx);
    }
}

static int cloneMqttOptions(MQTT_CLIENT* mqttData, const MQTT_CLIENT_OPTIONS* mqttOptions)
{
    int result = 0;
    if (mqttOptions->clientId != NULL)
    {
        if (mallocAndStrcpy_s(&mqttData->mqttOptions.clientId, mqttOptions->clientId) != 0)
        {
            result = __LINE__;
        }
    }
    if (result == 0 && mqttOptions->willTopic != NULL)
    {
        if (mallocAndStrcpy_s(&mqttData->mqttOptions.willTopic, mqttOptions->willTopic) != 0)
        {
            result = __LINE__;
        }
    }
    if (result == 0 && mqttOptions->willMessage != NULL)
    {
        if (mallocAndStrcpy_s(&mqttData->mqttOptions.willMessage, mqttOptions->willMessage) != 0)
        {
            result = __LINE__;
        }
    }
    if (result == 0 && mqttOptions->username != NULL)
    {
        if (mallocAndStrcpy_s(&mqttData->mqttOptions.username, mqttOptions->username) != 0)
        {
            result = __LINE__;
        }
    }
    if (result == 0 && mqttOptions->password != NULL)
    {
        if (mallocAndStrcpy_s(&mqttData->mqttOptions.password, mqttOptions->password) != 0)
        {
            result = __LINE__;
        }
    }
    if (result == 0)
    {
        mqttData->mqttOptions.keepAliveInterval = mqttOptions->keepAliveInterval;
        mqttData->mqttOptions.messageRetain = mqttOptions->messageRetain;
        mqttData->mqttOptions.useCleanSession = mqttOptions->useCleanSession;
        mqttData->mqttOptions.qualityOfServiceValue = mqttOptions->qualityOfServiceValue;
    }
    else
    {
        free(mqttData->mqttOptions.clientId);
        free(mqttData->mqttOptions.willTopic);
        free(mqttData->mqttOptions.willMessage);
        free(mqttData->mqttOptions.username);
        free(mqttData->mqttOptions.password);
    }
    return result;
}

static void recvCompleteCallback(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData)
{
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)context;
    if (mqttData != NULL && headerData != NULL)
    {
        size_t len = BUFFER_length(headerData);
        uint8_t* iterator = BUFFER_u_char(headerData);

        logIncomingMsgTrace(mqttData, packet, flags, iterator, len);

        if (iterator != NULL && len > 0)
        {
            switch (packet)
            {
            case CONNACK_TYPE:
            {
                if (mqttData->fnOperationCallback != NULL)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_028: [If the actionResult parameter is of type CONNECT_ACK then the msgInfo value shall be a CONNECT_ACK structure.]*/
                    CONNECT_ACK connack = { 0 };
                    connack.isSessionPresent = (byteutil_readByte(&iterator) == 0x1) ? true : false;
                    connack.returnCode = byteutil_readByte(&iterator);

                    mqttData->fnOperationCallback(mqttData, MQTT_CLIENT_ON_CONNACK, (void*)&connack, mqttData->ctx);

                    if (connack.returnCode == CONNECTION_ACCEPTED)
                    {
                        mqttData->clientConnected = true;
                    }
                }
                break;
            }
            case PUBLISH_TYPE:
            {
                if (mqttData->fnMessageRecv != NULL)
                {
                    //uint8_t ctrlPacket = byteutil_readByte(&iterator);
                    bool isDuplicateMsg = (flags & DUPLICATE_FLAG_MASK) ? true : false;
                    bool isRetainMsg = (flags & RETAIN_FLAG_MASK) ? true : false;
                    QOS_VALUE qosValue = (flags == 0) ? DELIVER_AT_MOST_ONCE : (flags & QOS_LEAST_ONCE_FLAG_MASK) ? DELIVER_AT_LEAST_ONCE : DELIVER_EXACTLY_ONCE;

                    uint8_t* initialPos = iterator;
                    char* topicName = byteutil_readUTF(&iterator, NULL);
                    uint16_t packetId = 0;
                    if (qosValue != DELIVER_AT_MOST_ONCE)
                    {
                        packetId = byteutil_read_uint16(&iterator);
                    }
                    size_t length = len - (iterator - initialPos);

                    MQTT_MESSAGE_HANDLE msgHandle = mqttmessage_create(packetId, topicName, qosValue, iterator, length);
                    if (msgHandle == NULL)
                    {
                        LOG(mqttData->logFunc, LOG_LINE, "failure in mqttmessage_create");
                    }
                    else
                    {
                        (void)mqttmessage_setIsDuplicateMsg(msgHandle, isDuplicateMsg);
                        (void)mqttmessage_setIsRetained(msgHandle, isRetainMsg);
                        mqttData->fnMessageRecv(msgHandle, mqttData->ctx);

                        BUFFER_HANDLE pubRel = NULL;
                        if (qosValue == DELIVER_EXACTLY_ONCE)
                        {
                            pubRel = mqtt_codec_publishReceived(packetId);
                        }
                        else if (qosValue == DELIVER_AT_LEAST_ONCE)
                        {
                            pubRel = mqtt_codec_publishAck(packetId);
                        }
                        if (pubRel != NULL)
                        {
                            (void)sendPacketItem(mqttData, BUFFER_u_char(pubRel), BUFFER_length(pubRel));
                            BUFFER_delete(pubRel);
                        }
                        free(topicName);
                        mqttmessage_destroy(msgHandle);
                    }
                }
                break;
            }
            case PUBACK_TYPE:
            case PUBREC_TYPE:
            case PUBREL_TYPE:
            case PUBCOMP_TYPE:
            {
                if (mqttData->fnOperationCallback)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_029: [If the actionResult parameter are of types PUBACK_TYPE, PUBREC_TYPE, PUBREL_TYPE or PUBCOMP_TYPE then the msgInfo value shall be a PUBLISH_ACK structure.]*/
                    MQTT_CLIENT_EVENT_RESULT action = (packet == PUBACK_TYPE) ? MQTT_CLIENT_ON_PUBLISH_ACK :
                        (packet == PUBREC_TYPE) ? MQTT_CLIENT_ON_PUBLISH_RECV :
                        (packet == PUBREL_TYPE) ? MQTT_CLIENT_ON_PUBLISH_REL : MQTT_CLIENT_ON_PUBLISH_COMP;

                    PUBLISH_ACK publish_ack = { 0 };
                    publish_ack.packetId = byteutil_read_uint16(&iterator);

                    BUFFER_HANDLE pubRel = NULL;
                    mqttData->fnOperationCallback(mqttData, action, (void*)&publish_ack, mqttData->ctx);
                    if (packet == PUBREC_TYPE)
                    {
                        pubRel = mqtt_codec_publishRelease(publish_ack.packetId);
                    }
                    else if (packet == PUBREL_TYPE)
                    {
                        pubRel = mqtt_codec_publishComplete(publish_ack.packetId);
                    }
                    if (pubRel != NULL)
                    {
                        (void)sendPacketItem(mqttData, BUFFER_u_char(pubRel), BUFFER_length(pubRel));
                        BUFFER_delete(pubRel);
                    }
                }
                break;
            }
            case SUBACK_TYPE:
            {
                if (mqttData->fnOperationCallback)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_030: [If the actionResult parameter is of type SUBACK_TYPE then the msgInfo value shall be a SUBSCRIBE_ACK structure.]*/
                    SUBSCRIBE_ACK suback = { 0 };

                    size_t remainLen = len;
                    suback.packetId = byteutil_read_uint16(&iterator);
                    remainLen -= 2;

                    // Allocate the remaining len
                    suback.qosReturn = (QOS_VALUE*)malloc(sizeof(QOS_VALUE)*remainLen);
                    if (suback.qosReturn != NULL)
                    {
                        while (remainLen > 0)
                        {
                            suback.qosReturn[suback.qosCount++] = byteutil_readByte(&iterator);
                            remainLen--;
                        }
                        (void)mqttData->fnOperationCallback(mqttData, MQTT_CLIENT_ON_SUBSCRIBE_ACK, (void*)&suback, mqttData->ctx);
                        free(suback.qosReturn);
                    }
                }
                break;
            }
            case UNSUBACK_TYPE:
            {
                if (mqttData->fnOperationCallback)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_031: [If the actionResult parameter is of type UNSUBACK_TYPE then the msgInfo value shall be a UNSUBSCRIBE_ACK structure.]*/
                    UNSUBSCRIBE_ACK unsuback = { 0 };
                    iterator += VARIABLE_HEADER_OFFSET;
                    unsuback.packetId = byteutil_read_uint16(&iterator);

                    (void)mqttData->fnOperationCallback(mqttData, MQTT_CLIENT_ON_UNSUBSCRIBE_ACK, (void*)&unsuback, mqttData->ctx);
                }
                break;
            }
            case PINGRESP_TYPE:
                // Ping responses do not get forwarded
                break;
            default:
                break;
            }
        }
    }
}

MQTT_CLIENT_HANDLE mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* callbackCtx, LOGGER_LOG logger)
{
    MQTT_CLIENT* result;
    /*Codes_SRS_MQTT_CLIENT_07_001: [If the parameters ON_MQTT_MESSAGE_RECV_CALLBACK is NULL then mqttclient_init shall return NULL.]*/
    if (msgRecv == NULL)
    {
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(MQTT_CLIENT));
        if (result == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
            LOG(logger, LOG_LINE, "mqtt_client_inti failure: Allocation Failure");
        }
        else
        {
            /*Codes_SRS_MQTT_CLIENT_07_003: [mqttclient_init shall allocate MQTTCLIENT_DATA_INSTANCE and return the MQTTCLIENT_HANDLE on success.]*/
            result->xioHandle = NULL;
            result->packetState = UNKNOWN_TYPE;
            result->logFunc = logger;
            result->packetSendTimeMs = 0;
            result->fnOperationCallback = opCallback;
            result->fnMessageRecv = msgRecv;
            result->ctx = callbackCtx;
            result->qosValue = DELIVER_AT_MOST_ONCE;
            result->keepAliveInterval = 0;
            result->packetTickCntr = tickcounter_create();
            result->mqttOptions.clientId = NULL;
            result->mqttOptions.willTopic = NULL;
            result->mqttOptions.willMessage = NULL;
            result->mqttOptions.username = NULL;
            result->mqttOptions.password = NULL;
            result->socketConnected = false;
            result->clientConnected = false;
            result->logTrace = false;
            result->rawBytesTrace = false;
            if (result->packetTickCntr == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
                LOG(logger, LOG_LINE, "mqtt_client_init failure: tickcounter_create failure");
                free(result);
                result = NULL;
            }
            else
            {
                result->codec_handle = mqtt_codec_create(recvCompleteCallback, result);
                if (result->codec_handle == NULL)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
                    LOG(logger, LOG_LINE, "mqtt_client_init failure: mqtt_codec_create failure");
                    tickcounter_destroy(result->packetTickCntr);
                    free(result);
                    result = NULL;
                }
            }
        }
    }
    return result;
}

void mqtt_client_deinit(MQTT_CLIENT_HANDLE handle)
{
    /*Codes_SRS_MQTT_CLIENT_07_004: [If the parameter handle is NULL then function mqtt_client_deinit shall do nothing.]*/
    if (handle != NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_005: [mqtt_client_deinit shall deallocate all memory allocated in this unit.]*/
        MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
        tickcounter_destroy(mqttData->packetTickCntr);
        mqtt_codec_destroy(mqttData->codec_handle);
        free(mqttData->mqttOptions.clientId);
        free(mqttData->mqttOptions.willTopic);
        free(mqttData->mqttOptions.willMessage);
        free(mqttData->mqttOptions.username);
        free(mqttData->mqttOptions.password);
        free(handle);
    }
}

int mqtt_client_connect(MQTT_CLIENT_HANDLE handle, XIO_HANDLE xioHandle, MQTT_CLIENT_OPTIONS* mqttOptions)
{
    int result;
    /*SRS_MQTT_CLIENT_07_006: [If any of the parameters handle, ioHandle, or mqttOptions are NULL then mqtt_client_connect shall return a non-zero value.]*/
    if (handle == NULL || mqttOptions == NULL)
    {
        result = __LINE__;
    }
    else
    {
        MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
        if (xioHandle == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
            LOG(mqttData->logFunc, LOG_LINE, "Error: mqttcodec_connect failed");
            result = __LINE__;
        }
        else
        {
            mqttData->xioHandle = xioHandle;
            mqttData->packetState = UNKNOWN_TYPE;
            mqttData->qosValue = mqttOptions->qualityOfServiceValue;
            mqttData->keepAliveInterval = mqttOptions->keepAliveInterval;
            if (cloneMqttOptions(mqttData, mqttOptions) != 0)
            {
                LOG(mqttData->logFunc, LOG_LINE, "Error: Clone Mqtt Options failed");
                result = __LINE__;
            }
            /*Codes_SRS_MQTT_CLIENT_07_008: [mqtt_client_connect shall open the XIO_HANDLE by calling into the xio_open interface.]*/
            else if (xio_open(xioHandle, onOpenComplete, onBytesReceived, onIoError, mqttData) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: io_open failed");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

int mqtt_client_publish(MQTT_CLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle)
{
    int result;
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
    if (mqttData == NULL || msgHandle == NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_019: [If one of the parameters handle or msgHandle is NULL then mqtt_client_publish shall return a non-zero value.]*/
        result = __LINE__;
    }
    else
    {
        /*Codes_SRS_MQTT_CLIENT_07_021: [mqtt_client_publish shall get the message information from the MQTT_MESSAGE_HANDLE.]*/
        const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
        if (payload == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_020: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
            LOG(mqttData->logFunc, LOG_LINE, "Error: mqttmessage_getApplicationMsg failed");
            result = __LINE__;
        }
        else
        {
            BUFFER_HANDLE publishPacket = mqtt_codec_publish(mqttmessage_getQosType(msgHandle), mqttmessage_getIsDuplicateMsg(msgHandle),
                mqttmessage_getIsRetained(msgHandle), mqttmessage_getPacketId(msgHandle), mqttmessage_getTopicName(msgHandle), payload->message, payload->length);
            if (publishPacket == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_020: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_publish failed");
                result = __LINE__;
            }
            else
            {
                mqttData->packetState = PUBLISH_TYPE;

                /*Codes_SRS_MQTT_CLIENT_07_022: [On success mqtt_client_publish shall send the MQTT SUBCRIBE packet to the endpoint.]*/
                if (sendPacketItem(mqttData, BUFFER_u_char(publishPacket), BUFFER_length(publishPacket)) != 0)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_020: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
                    LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_client_publish send failed");
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }
                BUFFER_delete(publishPacket);
            }
        }
    }
    return result;
}

int mqtt_client_subscribe(MQTT_CLIENT_HANDLE handle, uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count)
{
    int result;
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
    if (mqttData == NULL || subscribeList == NULL || count == 0)
    {
        /*Codes_SRS_MQTT_CLIENT_07_013: [If any of the parameters handle, subscribeList is NULL or count is 0 then mqtt_client_subscribe shall return a non-zero value.]*/
        result = __LINE__;
    }
    else
    {
        BUFFER_HANDLE subPacket = mqtt_codec_subscribe(packetId, subscribeList, count);
        if (subPacket == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_014: [If any failure is encountered then mqtt_client_subscribe shall return a non-zero value.]*/
            LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_subscribe failed");
            result = __LINE__;
        }
        else
        {
            mqttData->packetState = SUBSCRIBE_TYPE;

            /*Codes_SRS_MQTT_CLIENT_07_015: [On success mqtt_client_subscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
            LOG(mqttData->logFunc, LOG_LINE, "MQTT Subscribe");
            if (sendPacketItem(mqttData, BUFFER_u_char(subPacket), BUFFER_length(subPacket)) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_014: [If any failure is encountered then mqtt_client_subscribe shall return a non-zero value.]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_client_subscribe send failed");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
            BUFFER_delete(subPacket);
        }
    }
    return result;
}

int mqtt_client_unsubscribe(MQTT_CLIENT_HANDLE handle, uint16_t packetId, const char** unsubscribeList, size_t count)
{
    int result;
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
    if (mqttData == NULL || unsubscribeList == NULL || count == 0)
    {
        /*Codes_SRS_MQTT_CLIENT_07_016: [If any of the parameters handle, unsubscribeList is NULL or count is 0 then mqtt_client_unsubscribe shall return a non-zero value.]*/
        result = __LINE__;
    }
    else
    {
        BUFFER_HANDLE unsubPacket = mqtt_codec_unsubscribe(packetId, unsubscribeList, count);
        if (unsubPacket == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_017: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
            LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_unsubscribe failed");
            result = __LINE__;
        }
        else
        {
            mqttData->packetState = UNSUBSCRIBE_TYPE;

            /*Codes_SRS_MQTT_CLIENT_07_018: [On success mqtt_client_unsubscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
            LOG(mqttData->logFunc, LOG_LINE, "MQTT unsubscribe");
            if (sendPacketItem(mqttData, BUFFER_u_char(unsubPacket), BUFFER_length(unsubPacket)) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_017: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.].]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_client_unsubscribe send failed");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
            BUFFER_delete(unsubPacket);
        }
    }
    return result;
}

int mqtt_client_disconnect(MQTT_CLIENT_HANDLE handle)
{
    int result;
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
    if (mqttData == NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_010: [If the parameters handle is NULL then mqtt_client_disconnect shall return a non-zero value.]*/
        result = __LINE__;
    }
    else
    {
        mqttData->packetState = DISCONNECT_TYPE;
        BUFFER_HANDLE disconnectPacket = mqtt_codec_disconnect();
        if (disconnectPacket == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_011: [If any failure is encountered then mqtt_client_disconnect shall return a non-zero value.]*/
            LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_client_disconnect failed");
            result = __LINE__;
        }
        else
        {
            mqttData->packetState = DISCONNECT_TYPE;

            /*Codes_SRS_MQTT_CLIENT_07_012: [On success mqtt_client_disconnect shall send the MQTT DISCONNECT packet to the endpoint.]*/
            if (sendPacketItem(mqttData, BUFFER_u_char(disconnectPacket), BUFFER_length(disconnectPacket)) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_011: [If any failure is encountered then mqtt_client_disconnect shall return a non-zero value.]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_client_disconnect send failed");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
            BUFFER_delete(disconnectPacket);
        }
    }
    return result;
}

void mqtt_client_dowork(MQTT_CLIENT_HANDLE handle)
{
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
    /*Codes_SRS_MQTT_CLIENT_07_023: [If the parameter handle is NULL then mqtt_client_dowork shall do nothing.]*/
    if (mqttData != NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_024: [mqtt_client_dowork shall call the xio_dowork function to complete operations.]*/
        xio_dowork(mqttData->xioHandle);

        /*Codes_SRS_MQTT_CLIENT_07_025: [mqtt_client_dowork shall retrieve the the last packet send value and ...]*/
        if (mqttData->socketConnected && mqttData->clientConnected)
        {
            uint64_t current_ms;
            (void)tickcounter_get_current_ms(mqttData->packetTickCntr, &current_ms);
            if ((((current_ms - mqttData->packetSendTimeMs) / 1000) + KEEP_ALIVE_BUFFER_SEC) > mqttData->keepAliveInterval)
            {
                /*Codes_SRS_MQTT_CLIENT_07_026: [if this value is greater than the MQTT KeepAliveInterval then it shall construct an MQTT PINGREQ packet.]*/
                BUFFER_HANDLE pingPacket = mqtt_codec_ping();
                if (pingPacket != NULL)
                {
                    (void)sendPacketItem(mqttData, BUFFER_u_char(pingPacket), BUFFER_length(pingPacket));
                    BUFFER_delete(pingPacket);
                }
            }
        }
    }
}

void mqtt_client_set_trace(MQTT_CLIENT_HANDLE handle, bool traceOn, bool rawBytesOn)
{
    MQTT_CLIENT* mqttData = (MQTT_CLIENT*)handle;
    if (mqttData != NULL)
    {
        mqttData->logTrace = traceOn;
        mqttData->rawBytesTrace = rawBytesOn;
    }
}
