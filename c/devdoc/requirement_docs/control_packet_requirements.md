#control_packet Requirements

##Overview

control_packet is the library that encapsulates an handling of the control packet  

##Exposed API

```C
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
```

##ctrlpacket_connect
```
extern int ctrlpacket_connect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, const MQTTCLIENT_OPTIONS* mqttOptions);
```
**SRS_CONTROL_PACKET_07_001: [**If the parameters ioSendFn or mqttOptions is NULL then ctrlpacket_connect shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_002: [**ctrlpacket_connect shall constuct the MQTT variable header and shall return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_003: [**ctrlpacket_connect shall constuct the CONNECT payload and shall return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_004: [**ctrlpacket_connect shall call the ioSendFn callback to send the data buffer.**]**
**SRS_CONTROL_PACKET_07_057: [**If the ioSendFn function fails then ctrlpacket_connect shall return a non-zero value on failure.**]**
**SRS_CONTROL_PACKET_07_005: [**ctrlpacket_connect shall call clean all data that has been allocated.**]**  
**SRS_CONTROL_PACKET_07_006: [**ctrlpacket_connect shall return zero on successful completion.**]**  
**SRS_CONTROL_PACKET_07_007: [**ctrlpacket_connect shall return a non-zero value on any failure.**]**  

##ctrlpacket_disconnect
```
extern int ctrlpacket_disconnect(CTRL_PACKET_IO_SEND ioSendFn, void* callContext);
```
**SRS_CONTROL_PACKET_07_008: [**If the parameters ioSendFn is NULL then ctrlpacket_disconnect shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_009: [**ctrlpacket_disconnect shall construct the MQTT DISCONNECT packet.**]**  
**SRS_CONTROL_PACKET_07_010: [**ctrlpacket_disconnect shall call the ioSendFn callback and return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_011: [**ctrlpacket_disconnect shall return a zero value on successful completion.**]**  

##ctrlpacket_publish
```
extern int ctrlpacket_publish(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, QOS_VALUE qosValue, bool duplicateMsg, bool serverRetain, int packetId, const char* topicName, const BYTE* msgBuffer, size_t buffLen);
```
**SRS_CONTROL_PACKET_07_051: [**If the parameters ioSendFn, topicName, or msgBuffer is NULL then ctrlpacket_publish shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_052: [**ctrlpacket_publish shall construct the MQTT variable header and shall return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_053: [**ctrlpacket_publish shall use the function constructFixedHeader to create the MQTT PUBLISh fixed packet.**]**  
**SRS_CONTROL_PACKET_07_054: [**ctrlpacket_publish shall return a non-zero value on any failure.**]**  
**SRS_CONTROL_PACKET_07_055: [**ctrlpacket_publish shall call the ioSendFn callback and return a non-zero value on failure.**]**   
**SRS_CONTROL_PACKET_07_056: [**ctrlpacket_publish shall return zero on successful completion.**]**  

##ctrlpacket_publishAck
```
extern int ctrlpacket_publishAck(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
```
**SRS_CONTROL_PACKET_07_012: [**If the parameters ioSendFn is NULL then ctrlpacket_publishAck shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_013: [**ctrlpacket_publishAck construct the MQTT PUBACK reply and shall return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_014: [**ctrlpacket_publishAck shall return a zero value on successful completion.**]**  
**SRS_CONTROL_PACKET_07_015: [**ctrlpacket_publishAck shall call the ioSendFn callback and return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_016: [**if constructPublishReply fails then ctrlpacket_publishAck shall return a non-zero value.**]**  

##ctrlpacket_publishRecieved
```
extern int ctrlpacket_publishRecieved(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
```
**SRS_CONTROL_PACKET_07_017: [**If the parameters ioSendFn is NULL then ctrlpacket_publishRecieved shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_018: [**ctrlpacket_publishRecieved shall construct the MQTT PUBREC reply and shall return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_019: [**ctrlpacket_publishRecieved shall return a zero value on successful completion.**]**  
**SRS_CONTROL_PACKET_07_020: [**ctrlpacket_publishRecieved shall call the ioSendFn callback and return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_021: [**if constructPublishReply fails then ctrlpacket_publishAck shall return a non-zero value.**]**  

##ctrlpacket_publishRelease
```
extern int ctrlpacket_publishRelease(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
```
**SRS_CONTROL_PACKET_07_022: [**If the parameters ioSendFn is NULL then ctrlpacket_publishRelease shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_023: [**ctrlpacket_publishRelease shall use constructPublishReply to construct the MQTT PUBREL reply.**]**  
**SRS_CONTROL_PACKET_07_024: [**ctrlpacket_publishRelease shall return a zero value on successful completion.**]**  
**SRS_CONTROL_PACKET_07_025: [**ctrlpacket_publishRelease shall call the ioSendFn callback and return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_026: [**if constructPublishReply fails then ctrlpacket_publishRelease shall return a non-zero value.**]**  

##ctrlpacket_publishComplete
```
extern int ctrlpacket_publishComplete(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId);
```
**SRS_CONTROL_PACKET_07_027: [**If the parameters ioSendFn is NULL then ctrlpacket_publishComplete shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_028: [**ctrlpacket_publishComplete shall use constructPublishReply to construct the MQTT PUBCOMP reply.**]**  
**SRS_CONTROL_PACKET_07_029: [**ctrlpacket_publishComplete shall return a zero value on successful completion.**]**  
**SRS_CONTROL_PACKET_07_030: [**ctrlpacket_publishComplete shall call the ioSendFn callback and return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_031: [**if constructPublishReply fails then ctrlpacket_publishComplete shall return a non-zero value.**]**  

##ctrlpacket_subscribe
```
extern int ctrlpacket_subscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, SUBSCRIBE_PAYLOAD* payloadList, size_t payloadCount);
```
**SRS_CONTROL_PACKET_07_045: [**If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_Subscribe shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_046: [**ctrlpacket_Subscribe shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_047: [**ctrlpacket_Subscribe shall use the function addListItemToSubscribePacket to create the MQTT SUBSCRIBE payload packet.**]**  
**SRS_CONTROL_PACKET_07_048: [**ctrlpacket_Subscribe shall use the function constructFixedHeader to create the MQTT SUBSCRIBE fixed packet.**]**  
**SRS_CONTROL_PACKET_07_049: [**ctrlpacket_Subscribe shall call the ioSendFn callback and return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_050: [**ctrlpacket_Subscribe shall return a zero value on successful completion.**]**  


##ctrlpacket_unsubscribe
```
extern int ctrlpacket_unsubscribe(CTRL_PACKET_IO_SEND ioSendFn, void* callContext, int packetId, const char** payloadList, size_t payloadCount);
```
**SRS_CONTROL_PACKET_07_038: [**If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_unsubscribe shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_039: [**ctrlpacket_unsubscribe shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_040: [**ctrlpacket_unsubscribe shall use the function addListItemToUnsubscribePacket to create the MQTT UNSUBSCRIBE payload packet.**]**  
**SRS_CONTROL_PACKET_07_041: [**ctrlpacket_unsubscribe shall use the function constructFixedHeader to create the MQTT UNSUBSCRIBE fixed packet.**]**  
**SRS_CONTROL_PACKET_07_042: [**ctrlpacket_unsubscribe shall call the ioSendFn callback and return a non-zero value on failure.**]**  
**SRS_CONTROL_PACKET_07_043: [**ctrlpacket_unsubscribe shall return a zero value on successful completion.**]**  
**SRS_CONTROL_PACKET_07_044: [**ctrlpacket_unsubscribe shall return a non-zero value on any error encountered.**]**
 
##ctrlpacket_ping
```
extern int ctrlpacket_ping(CTRL_PACKET_IO_SEND ioSendFn, void* callContext);
```
**SRS_CONTROL_PACKET_07_032: [**If the parameters ioSendFn is NULL then ctrlpacket_ping shall return a non-zero value.**]**  
**SRS_CONTROL_PACKET_07_033: [**ctrlpacket_ping shall construct the MQTT PING packet.**]**  
**SRS_CONTROL_PACKET_07_034: [**ctrlpacket_ping shall return a zero value on successful completion.**]**  
**SRS_CONTROL_PACKET_07_035: [**ctrlpacket_publishComplete shall call the ioSendFn callback and return a non-zero value on failure.**]**
