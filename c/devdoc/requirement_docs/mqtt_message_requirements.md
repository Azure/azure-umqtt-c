# Mqtt_Message Requirements

##Overview

Mqtt_Message is the library that encapsulates an MQTT message

##Exposed API

```C
typedef struct MQTT_MESSAGE_INFORMATION* MQTT_MESSAGE_HANDLE;

extern MQTT_MESSAGE_HANDLE mqttmessage_createMessage(BYTE packetId, const char* topicName, QOS_VALUE qosValue, const BYTE* appMsg, size_t appLength, bool duplicateMsg, bool retainMsg);
extern void mqttmessage_destroyMessage(MQTT_MESSAGE_HANDLE handle);
extern MQTT_MESSAGE_HANDLE mqttmessage_clone(MQTT_MESSAGE_HANDLE handle);

extern BYTE mqttmessage_getPacketId(MQTT_MESSAGE_HANDLE handle);
extern const char* mqttmessage_getTopicName(MQTT_MESSAGE_HANDLE handle);
extern QOS_VALUE mqttmessage_getQosType(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmessage_isDuplicateMsg(MQTT_MESSAGE_HANDLE handle);
extern bool mqttmessage_isRetained(MQTT_MESSAGE_HANDLE handle);
extern const BYTE* mqttmessage_applicationMsg(MQTT_MESSAGE_HANDLE handle, size_t* msgLen);
```

##mqttmessage_createMessage
```
MQTT_MESSAGE_HANDLE mqttmessage_createMessage(BYTE packetId, const char* topicName, QOS_VALUE qosValue, const BYTE* appMsg, size_t appLength, bool duplicateMsg, bool retainMsg)
```
**SRS_MQTTMESSAGE_07_001: [**If the parameters topicName is NULL, appMsg is NULL, or appLength is zero then mqttmessage_createMessage shall return NULL.**]**
**SRS_MQTTMESSAGE_07_002: [**mqttmessage_createMessage shall allocate and copy the topicName and appMsg parameters.**]**
**SRS_MQTTMESSAGE_07_003: [**If any memory allocation fails mqttmessage_createMessage shall free any allocated memory and return NULL.**]**  
**SRS_MQTTMESSAGE_07_004: [**If mqttmessage_createMessage succeeds the it shall return a NON-NULL MQTT_MESSAGE_HANDLE value.**]**
  
##mqttmessage_destroyMessage
```
extern void mqttmessage_destroyMessage(MQTT_MESSAGE_HANDLE handle)
```
**SRS_MQTTMESSAGE_07_005: [**If the handle parameter is NULL then mqttmessage_destroyMessage shall do nothing**]**
**SRS_MQTTMESSAGE_07_006: [**mqttmessage_destroyMessage shall free all resources associated with the MQTT_MESSAGE_HANDLE value**]**

##mqttmessage_clone
```
extern MQTT_MESSAGE_HANDLE mqttmessage_clone(MQTT_MESSAGE_HANDLE handle)
```
**SRS_MQTTMESSAGE_07_007: [**If handle parameter is NULL then mqttmessage_clone shall return NULL.**]**
**SRS_MQTTMESSAGE_07_008: [**mqttmessage_clone shall create a new MQTT_MESSAGE_HANDLE with data content identical of the handle value.**]**
**SRS_MQTTMESSAGE_07_009: [**If any memory allocation fails mqttmessage_clone shall free any allocated memory and return NULL.**]**

##mqttmessage_getPacketId
```
extern BYTE mqttmessage_getPacketId(MQTT_MESSAGE_HANDLE handle)
```
**SRS_MQTTMESSAGE_07_010: [**If handle is NULL then mqttmessage_getPacketId shall return 0.**]**
**SRS_MQTTMESSAGE_07_011: [**mqttmessage_getPacketId shall return the packetId value contained in MQTT_MESSAGE_HANDLE handle.**]**

##mqttmessage_getTopicName
```
extern const char* mqttmessage_getTopicName(MQTT_MESSAGE_HANDLE handle)
```
**SRS_MQTTMESSAGE_07_012: [**If handle is NULL then mqttmessage_getTopicName shall return a NULL string.**]**
**SRS_MQTTMESSAGE_07_013: [**mqttmessage_getTopicName shall return the topicName contained in MQTT_MESSAGE_HANDLE handle.**]**

##mqttmessage_getQosType
```
extern QOS_VALUE mqttmessage_getQosType(MQTT_MESSAGE_HANDLE handle)
```
**SRS_MQTTMESSAGE_07_014: [**If handle is NULL then mqttmessage_getQosType shall return the default DELIVER_AT_MOST_ONCE value.**]**
**SRS_MQTTMESSAGE_07_015: [**mqttmessage_getQosType shall return the QOS Type value contained in MQTT_MESSAGE_HANDLE handle.**]**

##mqttmessage_isDuplicateMsg
```
extern bool mqttmessage_isDuplicateMsg(MQTT_MESSAGE_HANDLE handle)
```
**SRS_MQTTMESSAGE_07_016: [**If handle is NULL then mqttmessage_isDuplicateMsg shall return false.**]**
**SRS_MQTTMESSAGE_07_017: [**mqttmessage_isDuplicateMsg shall return the isDuplicateMsg value contained in MQTT_MESSAGE_HANDLE handle.**]** 

##mqttmessage_isRetained
```
extern bool mqttmessage_isRetained(MQTT_MESSAGE_HANDLE handle)
```
**SRS_MQTTMESSAGE_07_018: [**If handle is NULL then mqttmessage_isRetained shall return false.**]**
**SRS_MQTTMESSAGE_07_019: [**mqttmessage_isRetained shall return the isRetained value contained in MQTT_MESSAGE_HANDLE handle.**]**

##mqttmessage_applicationMsg
```
extern const BYTE* mqttmessage_applicationMsg(MQTT_MESSAGE_HANDLE handle, size_t* msgLen)
```
**SRS_MQTTMESSAGE_07_020: [**If handle is NULL or if msgLen is 0 then mqttmessage_applicationMsg shall return NULL.**]**
**SRS_MQTTMESSAGE_07_021: [**mqttmessage_applicationMsg shall return the applicationMsg value contained in MQTT_MESSAGE_HANDLE handle and the length of the appMsg in the msgLen parameter.**]** 
