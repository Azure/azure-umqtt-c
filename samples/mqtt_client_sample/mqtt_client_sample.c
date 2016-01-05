// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "mqtt_client_sample.h"
#include "mqtt_client.h"
#include "socketio.h"
#include "platform.h"

#include <stdarg.h>
#include <stdio.h>

#ifdef _CRT_DBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _CRT_DBG_MAP_ALLOC

static const char* TOPIC_NAME_A = "msgA";
static const char* TOPIC_NAME_B = "msgB";
static const char* APP_NAME_A = "This is the app msg A.";
static const char* APP_NAME_B = "This is the app msg B.";
static const char* HOSTNAME = "test.mosquitto.org";
static unsigned int sent_messages = 0;

static uint16_t PACKET_ID_VALUE = 11;
static bool g_continue = true;

#define PORT_NUM_UNENCRYPTED        1883
#define PORT_NUM_ENCRYPTED          8883
#define PORT_NUM_ENCRYPTED_CERT     8884

#define DEFAULT_MSG_TO_SEND         1

static void PrintLogFunction(unsigned int options, char* format, ...)
{
    va_list args;
    va_start(args, format);
    (void)vprintf(format, args);
    va_end(args);

    if (options & LOG_LINE)
    {
        (void)printf("\r\n");
    }
}

static const char* QosToString(QOS_VALUE qosValue)
{
    switch (qosValue)
    {
        case DELIVER_AT_LEAST_ONCE: return "Deliver_At_Least_Once";
        case DELIVER_EXACTLY_ONCE: return "Deliver_Exactly_Once";
        case DELIVER_AT_MOST_ONCE: return "Deliver_At_Most_Once";
        case DELIVER_FAILURE: return "Deliver_Failure";
    }
    return "";
}

static void OnRecvCallback(MQTT_MESSAGE_HANDLE msgHandle, void* context)
{
    const APP_PAYLOAD* appMsg = mqttmessage_getApplicationMsg(msgHandle);

    PrintLogFunction(0, "Incoming Msg: Packet Id: %d\r\nQOS: %s\r\nTopic Name: %s\r\nIs Retained: %s\r\nIs Duplicate: %s\r\nApp Msg: ", mqttmessage_getPacketId(msgHandle),
        QosToString(mqttmessage_getQosType(msgHandle) ),
        mqttmessage_getTopicName(msgHandle),
        mqttmessage_getIsRetained(msgHandle) ? "true" : "fale",
        mqttmessage_getIsDuplicateMsg(msgHandle) ? "true" : "fale"
        );
    for (size_t index = 0; index < appMsg->length; index++)
    {
        PrintLogFunction(0, "0x%x", appMsg->message[index]);
    }
    PrintLogFunction(0, "\r\n");
}

static void OnCloseComplete(void* context)
{
    PrintLogFunction(LOG_LINE, "%d: On Close Connection failed", __LINE__);
}

static void OnOperationComplete(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_RESULT actionResult, const void* msgInfo, void* callbackCtx)
{
    (void)callbackCtx;
    switch (actionResult)
    {
        case MQTT_CLIENT_ON_CONNACK:
        {
            PrintLogFunction(LOG_LINE, "ConnAck function called");
            const CONNECT_ACK* connack = (CONNECT_ACK*)msgInfo;
            SUBSCRIBE_PAYLOAD subscribe[] = {
                { TOPIC_NAME_A, DELIVER_AT_MOST_ONCE },
                { TOPIC_NAME_B, DELIVER_EXACTLY_ONCE },
            };
            if (mqtt_client_subscribe(handle, PACKET_ID_VALUE++, subscribe, sizeof(subscribe) / sizeof(subscribe[0])) != 0)
            {
                PrintLogFunction(LOG_LINE, "%d: mqtt_client_subscribe failed", __LINE__);
                g_continue = false;
            }
            break;
        }
        case MQTT_CLIENT_ON_SUBSCRIBE_ACK:
        {
            const SUBSCRIBE_ACK* suback = (SUBSCRIBE_ACK*)msgInfo;
            MQTT_MESSAGE_HANDLE msg = mqttmessage_create(PACKET_ID_VALUE++, TOPIC_NAME_A, DELIVER_EXACTLY_ONCE, APP_NAME_A, strlen(APP_NAME_A));
            if (msg == NULL)
            {
                PrintLogFunction(LOG_LINE, "%d: mqttmessage_create failed", __LINE__);
                g_continue = false;
            }
            else
            {
                if (mqtt_client_publish(handle, msg))
                {
                    PrintLogFunction(LOG_LINE, "%d: mqtt_client_publish failed", __LINE__);
                    g_continue = false;
                }
                mqttmessage_destroy(msg);
            }
            // Now send a message that will get 
            break;
        }
        case MQTT_CLIENT_ON_PUBLISH_ACK:
        {
            const PUBLISH_ACK* puback = (PUBLISH_ACK*)msgInfo;
            break;
        }
        case MQTT_CLIENT_ON_PUBLISH_RECV:
        {
            const PUBLISH_ACK* puback = (PUBLISH_ACK*)msgInfo;
            break;
        }
        case MQTT_CLIENT_ON_PUBLISH_REL:
        {
            const PUBLISH_ACK* puback = (PUBLISH_ACK*)msgInfo;
            break;
        }
        case MQTT_CLIENT_ON_PUBLISH_COMP:
        {
            const PUBLISH_ACK* puback = (PUBLISH_ACK*)msgInfo;
            // Done so send disconnect
            mqtt_client_disconnect(handle);
            break;
        }
        case MQTT_CLIENT_ON_ERROR:
        case MQTT_CLIENT_ON_DISCONNECT:
            g_continue = false;
            break;
    }
}

void mqtt_client_sample_run()
{
    if (platform_init() != 0)
    {
        PrintLogFunction(LOG_LINE, "platform_init failed");
    }
    else
    {
        MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(OnRecvCallback, OnOperationComplete, NULL, PrintLogFunction);
        if (mqttHandle == NULL)
        {
            PrintLogFunction(LOG_LINE, "mqtt_client_init failed");
        }
        else
        {
            MQTT_CLIENT_OPTIONS options = { 0 };
            options.clientId = "azureiotclient";
            options.willMessage = NULL;
            options.username = NULL;
            options.password = NULL;
            options.keepAliveInterval = 10;
            options.useCleanSession = true;
            options.qualityOfServiceValue = DELIVER_AT_MOST_ONCE;

            SOCKETIO_CONFIG config = {"test.mosquitto.org", PORT_NUM_UNENCRYPTED, NULL};

            XIO_HANDLE xio = xio_create(socketio_get_interface_description(), &config, PrintLogFunction);
            if (xio == NULL)
            {
                PrintLogFunction(LOG_LINE, "xio_create failed");
            }
            else
            {
                if (mqtt_client_connect(mqttHandle, xio, &options) != 0)
                {
                    PrintLogFunction(LOG_LINE, "mqtt_client_connect failed");
                }
                else
                {
                    do
                    {
                        mqtt_client_dowork(mqttHandle);
                    } while (g_continue);
                }
                xio_close(xio, OnCloseComplete, NULL);
            }
            mqtt_client_deinit(mqttHandle);
        }
        platform_deinit();
    }

#ifdef _CRT_DBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif
}
