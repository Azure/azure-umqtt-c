# Mqtt_Client Requirements

##Overview

Mqtt_Client is the library that encapsulates the mqtt protocol 

##Exposed API

```C
typedef struct MQTTCLIENT_DATA_INSTANCE_TAG* MQTTCLIENT_HANDLE;

#define MQTTCLIENT_ACTION_VALUES    \
    MQTTCLIENT_ON_CONNACT,          \
    MQTTCLIENT_ON_SUBSCRIBE,        \
    MQTTCLIENT_ON_DISCONNECT

DEFINE_ENUM(MQTTCLIENT_ACTION_RESULT, MQTTCLIENT_ACTION_VALUES);

typedef int(*ON_MQTT_OPERATION_CALLBACK)(MQTTCLIENT_ACTION_RESULT actionResult, void* context);
typedef void(*ON_MQTT_MESSAGE_RECV_CALLBACK)(MQTT_MESSAGE_HANDLE msgHandle, void* context);

extern MQTTCLIENT_HANDLE mqttclient_init(LOGGER_LOG logger, ON_MQTT_OPERATION_CALLBACK opCallback, ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, void* callCtx);
extern void mqttclient_deinit(MQTTCLIENT_HANDLE handle);

extern int mqttclient_connect(MQTTCLIENT_HANDLE handle, XIO_HANDLE ioHandle, MQTTCLIENT_OPTIONS* mqttOptions);
extern void mqttclient_disconnect(MQTTCLIENT_HANDLE handle);

extern int mqttclient_subscribe(MQTTCLIENT_HANDLE handle, BYTE packetId, const char* subscribeTopic, QOS_VALUE qosValue);
extern int mqttclient_unsubscribe(MQTTCLIENT_HANDLE handle, BYTE packetId, const char* unsubscribeTopic, QOS_VALUE qosValue);

extern int mqttclient_publish(MQTTCLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle);

extern void mqttclient_dowork(MQTTCLIENT_HANDLE handle);

```

##MqttClient_Init
```
extern MQTTCLIENT_HANDLE mqttclient_init(LOGGER_LOG logger, ON_MQTT_OPERATION_CALLBACK opCallback, ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, void* callCtx);
```

##MqttClient_Deinit
```
extern void mqttclient_deinit(MQTTCLIENT_HANDLE handle);
```

##mqttclient_connect
```
extern int mqttclient_connect(MQTTCLIENT_HANDLE handle, XIO_HANDLE ioHandle, MQTTCLIENT_OPTIONS* mqttOptions);
```

##mqttclient_disconnect
```
extern void mqttclient_disconnect(MQTTCLIENT_HANDLE handle);
```

##mqttclient_subscribe
```
extern int mqttclient_subscribe(MQTTCLIENT_HANDLE handle, BYTE packetId, const char* subscribeTopic, QOS_VALUE qosValue);
```

##mqttclient_unsubscribe
```
extern int mqttclient_unsubscribe(MQTTCLIENT_HANDLE handle, BYTE packetId, const char* unsubscribeTopic, QOS_VALUE qosValue);
```

##mqttclient_publish
```
extern int mqttclient_publish(MQTTCLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle);
```

##mqttclient_dowork
```
extern void mqttclient_dowork(MQTTCLIENT_HANDLE handle);
```
