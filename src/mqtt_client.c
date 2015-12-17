// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "gballoc.h"

#include "mqtt_client.h"
#include "mqtt_codec.h"
#include "platform.h"
#include "tickcounter.h"

#define KEEP_ALIVE_BUFFER_SEC           2
#define VARIABLE_HEADER_OFFSET          2
#define RETAIN_FLAG_MASK                0x1
#define QOS_LEAST_ONCE_FLAG_MASK        0x2
#define QOS_EXACTLY_ONCE_FLAG_MASK      0x4
#define DUPLICATE_FLAG_MASK             0x8

typedef struct MQTT_CLIENT_DATA_INSTANCE_TAG
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
} MQTT_CLIENT_DATA_INSTANCE;



uint16_t byteutil_readInt(uint8_t** buffer)
{
    uint16_t result = 0;
    if (buffer != NULL)
    {
        uint8_t* iterator = *buffer;
        result = 256 * ((uint8_t)(*iterator)) + (uint8_t)(*(iterator + 1));
        *buffer += 2; // Move the ptr
    }
    return result;
}

char* byteutil_readUTF(uint8_t** buffer)
{
    char* result = NULL;
    if (buffer != NULL)
    {
        // Get the length of the string
        int len = byteutil_readInt(buffer);
        if (len > 0)
        {
            result = (char*)malloc(len + 1);
            if (result != NULL)
            {
                (void)memcpy(result, *buffer, len);
                result[len] = '\0';
                *buffer += len;
            }
        }
    }
    return result;
}

uint8_t byteutil_readByte(uint8_t** buffer)
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
    (void)context;
    (void)send_result;
}

static int sendPacketItem(MQTT_CLIENT_DATA_INSTANCE* clientData, const int8_t* data, size_t length)
{
    (void)tickcounter_get_current_ms(clientData->packetTickCntr, &clientData->packetSendTimeMs);
    int result = xio_send(clientData->ioHandle, data, length, sendComplete, clientData);
    if (result != 0)
    {
        clientData->logFunc(0, "%d: Failure sending control packet data", result);
        result = __LINE__;
    }
    return result;
}

static void stateChanged(void* context, IO_STATE new_io_state, IO_STATE previous_io_state)
{
    MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)context;
    if (mqttData != NULL)
    {
    }
}

static void recvCompleteCallback(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData)
{
    (void)context;
    (void)packet;
    (void)flags;
    (void)headerData;
}

MQTT_CLIENT_HANDLE mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* callbackCtx, LOGGER_LOG logger)
{
    MQTT_CLIENT_DATA_INSTANCE* result;
    /*Codes_SRS_MQTT_CLIENT_07_001: [If the parameters ON_MQTT_MESSAGE_RECV_CALLBACK is NULL then mqttclient_init shall return NULL.]*/
    if (msgRecv == NULL)
    {
        result = NULL;
    }
    else
    {
        if (platform_init() != 0)
        {
            /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
            result = NULL;
        }
        else
        {
            result = malloc(sizeof(MQTT_CLIENT_DATA_INSTANCE));
            if (result == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
                LOG(logger, LOG_LINE, "Allocation Failure: MQTT_CLIENT_DATA_INSTANCE");
            }
            else
            {
                /*Codes_SRS_MQTT_CLIENT_07_003: [mqttclient_init shall allocate MQTTCLIENT_DATA_INSTANCE and return the MQTTCLIENT_HANDLE on success.]*/
                result->ioHandle = NULL;
                result->packetState = UNKNOWN_TYPE;
                result->fnOperationCallback = opCallback;
                result->ctx = callbackCtx;
                result->logFunc = logger;
                result->qosValue = DELIVER_AT_MOST_ONCE;
                result->keepAliveInterval = 0;
                result->codec_handle = mqtt_codec_create(recvCompleteCallback, result);
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
        MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)handle;
        tickcounter_destroy(mqttData->packetTickCntr);
        mqtt_codec_destroy(mqttData->codec_handle);
        mqttData->codec_handle = NULL;
        free(handle);
    }
    platform_deinit();
}

int mqtt_client_connect(MQTT_CLIENT_HANDLE handle, XIO_HANDLE ioHandle, MQTT_CLIENT_OPTIONS* mqttOptions)
{
    int result;
    /*SRS_MQTT_CLIENT_07_006: [If any of the parameters handle, ioHandle, or mqttOptions are NULL then mqtt_client_connect shall return a non-zero value.]*/
    if (handle == NULL || mqttOptions == NULL)
    {
        result = __LINE__;
    }
    else
    {
        MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)handle;
        if (ioHandle == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
            LOG(mqttData->logFunc, LOG_LINE, "Error: mqttcodec_connect failed");
            result = __LINE__;
        }
        else
        {
            mqttData->ioHandle = ioHandle;
            mqttData->packetState = UNKNOWN_TYPE;
            mqttData->qosValue = mqttOptions->qualityOfServiceValue;
            mqttData->keepAliveInterval = mqttOptions->keepAliveInterval;
            mqttData->codec_handle = mqtt_codec_create(recvCompleteCallback, mqttData);
            if (mqttData->codec_handle == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: mqttcodec_init failed");
                result = __LINE__;
            }
            else
            {
                /*Codes_SRS_MQTT_CLIENT_07_008: [mqtt_client_connect shall open the XIO_HANDLE by calling into the xio_open interface.]*/
                if (xio_open(ioHandle, mqtt_codec_bytesReceived, stateChanged, mqttData) != 0)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                    LOG(mqttData->logFunc, LOG_LINE, "Error: io_open failed");
                    mqtt_codec_destroy(mqttData->codec_handle);
                    mqttData->codec_handle = NULL;
                    result = __LINE__;
                }
                else
                {
                    mqttData->packetState = CONNECT_TYPE;
                    mqttData->packetTickCntr = tickcounter_create();

                    BUFFER_HANDLE connPacket = mqtt_codec_connect(mqttOptions);
                    if (connPacket == NULL)
                    {
                        /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                        LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_connect failed");
                        mqtt_codec_destroy(mqttData->codec_handle);
                        mqttData->codec_handle = NULL;
                        result = __LINE__;
                    }
                    else
                    {
                        /*Codes_SRS_MQTT_CLIENT_07_009: [On success mqtt_client_connect shall send the MQTT CONNECT to the endpoint.]*/
                        if (sendPacketItem(mqttData, BUFFER_u_char(connPacket), BUFFER_length(connPacket) ) != 0)
                        {
                            /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                            LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_connect failed");
                            mqtt_codec_destroy(mqttData->codec_handle);
                            mqttData->codec_handle = NULL;
                            result = __LINE__;
                        }
                        else
                        {
                            result = 0;
                        }
                        BUFFER_delete(connPacket);
                    }
                }
            }
        }
    }
    return result;
}

int mqtt_client_publish(MQTT_CLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle)
{
    int result;
    MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)handle;
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
            result = __LINE__;
        }
        else
        {
            BUFFER_HANDLE publishPacket = mqtt_codec_publish(mqttmessage_getQosType(msgHandle), mqttmessage_getIsDuplicateMsg(msgHandle),
                mqttmessage_getIsRetained(msgHandle), mqttmessage_getPacketId(msgHandle), mqttmessage_getTopicName(msgHandle), payload->message, payload->messageLength);
            if (publishPacket == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_020: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
                LOG(mqttData->logFunc, LOG_LINE, "Error: mqtt_codec_publish failed");
                result = __LINE__;
            }
            else
            {
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

int mqtt_client_subscribe(MQTT_CLIENT_HANDLE handle, uint8_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count)
{
    int result;
    MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)handle;
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
            /*Codes_SRS_MQTT_CLIENT_07_015: [On success mqtt_client_subscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
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

int mqtt_client_unsubscribe(MQTT_CLIENT_HANDLE handle, uint8_t packetId, const char** unsubscribeList, size_t count)
{
    int result;
    MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)handle;
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
            /*Codes_SRS_MQTT_CLIENT_07_018: [On success mqtt_client_unsubscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
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
    MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)handle;
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
    MQTT_CLIENT_DATA_INSTANCE* mqttData = (MQTT_CLIENT_DATA_INSTANCE*)handle;
    /*Codes_SRS_MQTT_CLIENT_07_023: [If the parameter handle is NULL then mqtt_client_dowork shall do nothing.]*/
    if (mqttData != NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_024: [mqtt_client_dowork shall call the xio_dowork function to complete operations.]*/
        xio_dowork(mqttData->ioHandle);

        /*Codes_SRS_MQTT_CLIENT_07_025: [mqtt_client_dowork shall retrieve the the last packet send value and ...]*/
        (void)tickcounter_get_current_ms(mqttData->packetTickCntr, &mqttData->packetSendTimeMs);
        if ( ( (mqttData->packetSendTimeMs/1000)+ KEEP_ALIVE_BUFFER_SEC) > mqttData->keepAliveInterval)
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
