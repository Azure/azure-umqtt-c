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
    /* Code_SRS_MQTTMESSAGE_07_001:[If the parameters topicName is NULL, appMsg is NULL, or appLength is zero then mqttmsg_createMessage shall return NULL.] */
    MQTT_MESSAGE_INFORMATION* result;
    if (topicName == NULL || appMsg == NULL || appLength == 0)
    {
        result = NULL;
    }
    else
    {
        /* Code_SRS_MQTTMESSAGE_07_002: [mqttmsg_createMessage shall allocate and copy the topicName and appMsg parameters.] */
        result = malloc(sizeof(MQTT_MESSAGE_INFORMATION));
        if (result != NULL)
        {
            if (mallocAndStrcpy_s(&result->topicName, topicName) != 0)
            {
                /* Code_SRS_MQTTMESSAGE_07_003: [If any memory allocation fails mqttmsg_createMessage shall free any allocated memory and return NULL.] */
                free(result);
                result = NULL;
            }
            else
            {
                /* Code_SRS_MQTTMESSAGE_07_002: [mqttmsg_createMessage shall allocate and copy the topicName and appMsg parameters.] */
                result->appLength = appLength;
                result->appMsg = malloc(appLength);
                if (result->appMsg == NULL)
                {
                    /* Code_SRS_MQTTMESSAGE_07_003: [If any memory allocation fails mqttmsg_createMessage shall free any allocated memory and return NULL.] */
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
    /* Code_SRS_MQTTMESSAGE_07_004: [If mqttmsg_createMessage succeeds the it shall return a NON-NULL MQTT_MESSAGE_HANDLE value.] */
    return (MQTT_MESSAGE_HANDLE)result;
}

void mqttmsg_destroyMessage(MQTT_MESSAGE_HANDLE handle)
{
    MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
    /* Code_SRS_MQTTMESSAGE_07_005: [If the handle parameter is NULL then mqttmsg_destroyMessage shall do nothing] */
    if (msgInfo != NULL)
    {
        /* Code_SRS_MQTTMESSAGE_07_006: [mqttmsg_destroyMessage shall free all resources associated with the MQTT_MESSAGE_HANDLE value] */
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
        /* Code_SRS_MQTTMESSAGE_07_007: [If handle parameter is NULL then mqttmsg_clone shall return NULL.] */
        result = NULL;
    }
    else
    {
        /* Code_SRS_MQTTMESSAGE_07_008: [mqttmsg_clone shall create a new MQTT_MESSAGE_HANDLE with data content identical of the handle value.] */
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = malloc(sizeof(MQTT_MESSAGE_INFORMATION));
        if (mallocAndStrcpy_s(&result->topicName, msgInfo->topicName) != 0)
        {
            /* Code_SRS_MQTTMESSAGE_07_009: [If any memory allocation fails mqttmsg_clone shall free any allocated memory and return NULL.] */
            free(result);
            result = NULL;
        }
        else
        {
            result->appMsg = malloc(msgInfo->appLength);
            if (result->appMsg == NULL)
            {
                /* Code_SRS_MQTTMESSAGE_07_009: [If any memory allocation fails mqttmsg_clone shall free any allocated memory and return NULL.] */
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
        /* Code_SRS_MQTTMESSAGE_07_010: [If handle is NULL then mqttmsg_getPacketId shall return 0.] */
        result = 0;
    }
    else
    {
        /* Code_SRS_MQTTMESSAGE_07_011: [mqttmsg_getPacketId shall return the packetId value contained in MQTT_MESSAGE_HANDLE handle.] */
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
        /* Code_SRS_MQTTMESSAGE_07_012: [If handle is NULL then mqttmsg_getTopicName shall return a NULL string.] */
        result = NULL;
    }
    else
    {
        /* Code_SRS_MQTTMESSAGE_07_013: [mqttmsg_getTopicName shall return the topicName contained in MQTT_MESSAGE_HANDLE handle.] */
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
        /* Code_SRS_MQTTMESSAGE_07_014: [If handle is NULL then mqttmsg_getQosType shall return the default DELIVER_AT_MOST_ONCE value.] */
        result = DELIVER_AT_MOST_ONCE;
    }
    else
    {
        /* Code_SRS_MQTTMESSAGE_07_015: [mqttmsg_getQosType shall return the QOS Type value contained in MQTT_MESSAGE_HANDLE handle.] */
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
        /* Code_SRS_MQTTMESSAGE_07_016: [If handle is NULL then mqttmsg_isDuplicateMsg shall return false.] */
        result = false;
    }
    else
    {
        /* Code_SRS_MQTTMESSAGE_07_017: [mqttmsg_isDuplicateMsg shall return the isDuplicateMsg value contained in MQTT_MESSAGE_HANDLE handle.] */
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
        /* Code_SRS_MQTTMESSAGE_07_018: [If handle is NULL then mqttmsg_isRetained shall return false.] */
        result = false;
    }
    else
    {
        /* Code_SRS_MQTTMESSAGE_07_019: [mqttmsg_isRetained shall return the isRetained value contained in MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->retainMsg;
    }
    return result;
}

const BYTE* mqttmsg_applicationMsg(MQTT_MESSAGE_HANDLE handle, size_t* msgLen)
{
    const BYTE* result;
    if (handle == NULL || msgLen == 0)
    {
        /* Code_SRS_MQTTMESSAGE_07_020: [If handle is NULL or if msgLen is 0 then mqttmsg_applicationMsg shall return NULL.] */
        result = NULL;
    }
    else
    {
        /* Test_SRS_MQTTMESSAGE_07_021: [mqttmsg_applicationMsg shall return the applicationMsg value contained in MQTT_MESSAGE_HANDLE handle and the length of the appMsg in the msgLen parameter.] */
        MQTT_MESSAGE_INFORMATION* msgInfo = (MQTT_MESSAGE_INFORMATION*)handle;
        result = msgInfo->appMsg;
        *msgLen = msgInfo->appLength;
    }
    return result;
}
