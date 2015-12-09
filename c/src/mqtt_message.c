// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "mqtt_message.h"
#include "gballoc.h"

typedef struct MQTT_MESSAGE_INFORMATION_TAG
{
    BYTE packetId;
    char* topicName;
    QOS_VALUE qosInfo;
    size_t appLength;
    BYTE* appMsg;
    bool duplicateMsg;
    bool retainMsg;
} MQTT_MESSAGE_INFORMATION;

MQTT_MESSAGE_HANDLE mqttmsg_createMessage(BYTE packetId, const char* topicName, QOS_VALUE qosValue, const BYTE* appMsg, size_t appLength, bool duplicateMsg, bool retainMsg)
{
    MQTT_MESSAGE_INFORMATION* result;
    if (topicName == NULL || appMsg == NULL || appLength == 0)
    {
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(MQTT_MESSAGE_INFORMATION));
        if (result != NULL)
        {
            if (mallocAndStrcpy_s(&result->topicName, topicName) != 0)
            {
                free(result);
                result = NULL;
            }
            else
            {
                result->appLength = appLength;
                result->appMsg = malloc(appLength);
                if (result->appMsg == NULL)
                {
                    free(result->topicName);
                    free(result);
                    result = NULL;
                }
                else
                {
                    memcpy(result->appMsg, appMsg, appLength);
                    result->packetId = packetId;
                    result->duplicateMsg = duplicateMsg;
                    result->retainMsg = retainMsg;
                    result->qosInfo = qosValue;
                }
            }
        }
    }
    return (MQTT_MESSAGE_HANDLE)result;
}

void mqttmsg_destroyMessage(MQTT_MESSAGE_HANDLE handle)
{
    MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
    if (msgInfo != NULL)
    {
        free(msgInfo->topicName);
        free(msgInfo->appMsg);
        free(handle);
    }
}

MQTT_MESSAGE_HANDLE mqttmsg_clone(MQTT_MESSAGE_HANDLE handle)
{
    MQTT_MESSAGE_INFORMATION* result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = malloc(sizeof(MQTT_MESSAGE_INFORMATION));
        if (mallocAndStrcpy_s(&result->topicName, msgInfo->topicName) != 0)
        {
            free(result);
            result = NULL;
        }
        else
        {
            result->appMsg = malloc(msgInfo->appLength);
            if (result->appMsg == NULL)
            {
                free(result->topicName);
                free(result);
                result = NULL;
            }
            else
            {
                memcpy(result->appMsg, msgInfo->appMsg, msgInfo->appLength);
                result->packetId = msgInfo->packetId;
                result->duplicateMsg = msgInfo->duplicateMsg;
                result->retainMsg = msgInfo->retainMsg;
                result->qosInfo = msgInfo->qosInfo;
            }
        }
    }
    return (MQTT_MESSAGE_HANDLE)result;
}

BYTE mqttmsg_getPacketId(MQTT_MESSAGE_HANDLE handle)
{
    BYTE result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->packetId;
    }
    return result;
}

const char* mqttmsg_getTopicName(MQTT_MESSAGE_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->topicName;
    }
    return result;
}

QOS_VALUE mqttmsg_getQosType(MQTT_MESSAGE_HANDLE handle)
{
    QOS_VALUE result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->qosInfo;
    }
    return result;
}

bool mqttmsg_isDuplicateMsg(MQTT_MESSAGE_HANDLE handle)
{
    bool result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->duplicateMsg;
    }
    return result;
}

bool mqttmsg_isRetained(MQTT_MESSAGE_HANDLE handle)
{
    bool result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->retainMsg;
    }
    return result;
}

const BYTE* mqttmsg_applicationMsg(MQTT_MESSAGE_HANDLE handle, size_t* msgLen)
{
    const BYTE* result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->appMsg;
        if (msgLen != NULL)
        {
            *msgLen = msgInfo->appLength;
        }
    }
    return result;
}
