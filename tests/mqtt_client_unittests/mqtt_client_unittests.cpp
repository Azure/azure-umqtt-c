// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// PUT NO INCLUDES BEFORE HERE !!!!
//
#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "tickcounter.h"
#include "lock.h"
#include "agenttime.h"
#include "buffer_.h"

#include "mqtt_client.h"
#include "mqtt_codec.h"
#include "mqtt_message.h"
#include "mqttconst.h"

#define GBALLOC_H
extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
    // if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble
    #define Lock(x) (LOCK_OK + gballocState - gballocState) // compiler warning about constant in if condition
    #define Unlock(x) (LOCK_OK + gballocState - gballocState)
    #define Lock_Init() (LOCK_HANDLE)0x42
    #define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
    #include "gballoc.c"
    #undef Lock
    #undef Unlock
    #undef Lock_Init
    #undef Lock_Deinit

    #include "buffer.c"
};

static const char* TEST_USERNAME = "testuser";
static const char* TEST_PASSWORD = "testpassword";
static const char* TEST_TOPIC_NAME = "topic Name";
static const uint8_t* TEST_MESSAGE = (const uint8_t*)"Message to send";
static const APP_PAYLOAD TEST_APP_PAYLOAD = { (uint8_t*)TEST_MESSAGE, 15 };
static const char* TEST_CLIENT_ID = "test_client_id";
static const char* TEST_SUBSCRIPTION_TOPIC = "subTopic";
static SUBSCRIBE_PAYLOAD TEST_SUBSCRIBE_PAYLOAD[] = { {"subTopic1", DELIVER_AT_LEAST_ONCE }, {"subTopic2", DELIVER_EXACTLY_ONCE } };
static const char* TEST_UNSUBSCRIPTION_TOPIC[] = { "subTopic1", "subTopic2" };

static const XIO_HANDLE TEST_IO_HANDLE = (XIO_HANDLE)0x11;
static const TICK_COUNTER_HANDLE TEST_COUNTER_HANDLE = (TICK_COUNTER_HANDLE)0x12;
static const MQTTCODEC_HANDLE TEST_MQTTCODEC_HANDLE = (MQTTCODEC_HANDLE)0x13;
static const MQTT_MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (MQTT_MESSAGE_HANDLE)0x14;
static BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x15;
static const int TEST_KEEP_ALIVE_INTERVAL = 20;
static const uint8_t TEST_PACKET_ID = (uint8_t)0x8158;

static bool g_operationCallbackInvoked;
static bool g_msgRecvCallbackInvoked;
static bool g_fail_alloc_calls;
static uint64_t g_current_ms;
ON_PACKET_COMPLETE_CALLBACK g_packetComplete;
typedef struct TEST_COMPLETE_DATA_INSTANCE_TAG
{
    MQTTCLIENT_ACTION_RESULT actionResult;
    void* msgInfo;
} TEST_COMPLETE_DATA_INSTANCE;

TYPED_MOCK_CLASS(mqttclient_mocks, CGlobalMock)
{
public:
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* ptr = NULL;
        if (!g_fail_alloc_calls)
        {
            ptr = BASEIMPLEMENTATION::gballoc_malloc(size);
        }
    MOCK_METHOD_END(void*, ptr);

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_connect, const MQTTCLIENT_OPTIONS*, mqttOptions)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_7(, BUFFER_HANDLE, mqtt_codec_publish, QOS_VALUE, qosValue, bool, duplicateMsg, bool, serverRetain, int, packetId, const char*, topicName, const uint8_t*, msgBuffer, size_t, appLen)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_3(, BUFFER_HANDLE, mqtt_codec_subscribe, int, packetId, SUBSCRIBE_PAYLOAD*, payloadList, size_t, payloadCount)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_3(, BUFFER_HANDLE, mqtt_codec_unsubscribe, int, packetId, const char**, unsubscribeTopic, size_t, payloadCount)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_0(, BUFFER_HANDLE, mqtt_codec_disconnect)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_0(, BUFFER_HANDLE, mqtt_codec_ping)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishAck, int, packetId)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishRecieved, int, packetId)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);
    
    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishRelease, int, packetId)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishComplete, int, packetId)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_2(, MQTTCODEC_HANDLE, mqtt_codec_create, ON_PACKET_COMPLETE_CALLBACK, packetComplete, void*, callContext)
        g_packetComplete = packetComplete;
    MOCK_METHOD_END(MQTTCODEC_HANDLE, TEST_MQTTCODEC_HANDLE);

    MOCK_STATIC_METHOD_1(, void, mqtt_codec_destroy, MQTTCODEC_HANDLE, handle)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, int, mqtt_codec_bytesReceived, MQTTCODEC_HANDLE, handle, const void*, buffer, size_t, size)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_4(, int, xio_open, XIO_HANDLE, handle, ON_BYTES_RECEIVED, on_bytes_received, ON_IO_STATE_CHANGED, on_io_state_changed, void*, callback_context)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, int, xio_close, XIO_HANDLE, ioHandle)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, void, xio_dowork, XIO_HANDLE, ioHandle)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_5(, int, xio_send, XIO_HANDLE, handle, const void*, buffer, size_t, size, ON_SEND_COMPLETE, on_send_complete, void*, callback_context)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_0(, int, platform_init)
    MOCK_METHOD_END(int, 0); 

    MOCK_STATIC_METHOD_0(, void, platform_deinit)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_0(, TICK_COUNTER_HANDLE, tickcounter_create)
    MOCK_METHOD_END(TICK_COUNTER_HANDLE, TEST_COUNTER_HANDLE);

    MOCK_STATIC_METHOD_1(, int, tickcounter_reset, TICK_COUNTER_HANDLE, tick_counter)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, void, tickcounter_destroy, TICK_COUNTER_HANDLE, tick_counter)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_2(, int, tickcounter_get_current_ms, TICK_COUNTER_HANDLE, tick_counter, uint64_t*, current_ms)
        *current_ms = g_current_ms;
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, unsigned char*, BUFFER_u_char, BUFFER_HANDLE, s)
    MOCK_METHOD_END(unsigned char*, BASEIMPLEMENTATION::BUFFER_u_char(s));

    MOCK_STATIC_METHOD_1(, size_t, BUFFER_length, BUFFER_HANDLE, s)
    MOCK_METHOD_END(size_t, BASEIMPLEMENTATION::BUFFER_length(s));

    MOCK_STATIC_METHOD_5(, MQTT_MESSAGE_HANDLE, mqttmessage_create, uint16_t, packetId, const char*, topicName, QOS_VALUE, qosValue, const uint8_t*, appMsg, size_t, appLength)
    MOCK_METHOD_END(MQTT_MESSAGE_HANDLE, TEST_MESSAGE_HANDLE);

    MOCK_STATIC_METHOD_1(, void, mqttmessage_destroyMessage, MQTT_MESSAGE_HANDLE, handle)
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, MQTT_MESSAGE_HANDLE, mqttmessage_clone, MQTT_MESSAGE_HANDLE, handle)
    MOCK_METHOD_END(MQTT_MESSAGE_HANDLE, TEST_MESSAGE_HANDLE);

    MOCK_STATIC_METHOD_1(, uint16_t, mqttmessage_getPacketId, MQTT_MESSAGE_HANDLE, handle)
    MOCK_METHOD_END(uint16_t, TEST_PACKET_ID);
    
    MOCK_STATIC_METHOD_1(, const char*, mqttmessage_getTopicName, MQTT_MESSAGE_HANDLE, handle)
    MOCK_METHOD_END(const char*, TEST_TOPIC_NAME);

    MOCK_STATIC_METHOD_1(, QOS_VALUE, mqttmessage_getQosType, MQTT_MESSAGE_HANDLE, handle)
    MOCK_METHOD_END(QOS_VALUE, DELIVER_AT_LEAST_ONCE);

    MOCK_STATIC_METHOD_1(, bool, mqttmessage_getIsDuplicateMsg, MQTT_MESSAGE_HANDLE, handle)
    MOCK_METHOD_END(bool, true);

    MOCK_STATIC_METHOD_1(, bool, mqttmessage_getIsRetained, MQTT_MESSAGE_HANDLE, handle)
    MOCK_METHOD_END(bool, true);

    MOCK_STATIC_METHOD_2(, int, mqttmessage_setIsDuplicateMsg, MQTT_MESSAGE_HANDLE, handle, bool, duplicateMsg)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_2(, int, mqttmessage_setIsRetained, MQTT_MESSAGE_HANDLE, handle, bool, retainMsg)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, const APP_PAYLOAD*, mqttmessage_getApplicationMsg, MQTT_MESSAGE_HANDLE, handle);
    MOCK_METHOD_END(const APP_PAYLOAD*, &TEST_APP_PAYLOAD);
};

extern "C"
{
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , void, gballoc_free, void*, ptr);

    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_connect, const MQTTCLIENT_OPTIONS*, mqttOptions);
    DECLARE_GLOBAL_MOCK_METHOD_7(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_publish, QOS_VALUE, qosValue, bool, duplicateMsg, bool, serverRetain, int, packetId, const char*, topicName, const uint8_t*, msgBuffer, size_t, appLen);
    DECLARE_GLOBAL_MOCK_METHOD_0(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_disconnect);
    DECLARE_GLOBAL_MOCK_METHOD_0(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_ping);
    DECLARE_GLOBAL_MOCK_METHOD_3(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_subscribe, int, packetId, SUBSCRIBE_PAYLOAD*, payloadList, size_t, payloadCount);

    DECLARE_GLOBAL_MOCK_METHOD_3(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_unsubscribe, int, packetId, const char**, unsubscribeTopic, size_t, payloadCount);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_publishAck, int, packetId);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_publishRecieved, int, packetId);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_publishRelease, int, packetId);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , BUFFER_HANDLE, mqtt_codec_publishComplete, int, packetId);

    DECLARE_GLOBAL_MOCK_METHOD_2(mqttclient_mocks, , MQTTCODEC_HANDLE, mqtt_codec_create, ON_PACKET_COMPLETE_CALLBACK, packetComplete, void*, callContext);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , void, mqtt_codec_destroy, MQTTCODEC_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_3(mqttclient_mocks, , int, mqtt_codec_bytesReceived, MQTTCODEC_HANDLE, handle, const void*, buffer, size_t, size);

    DECLARE_GLOBAL_MOCK_METHOD_4(mqttclient_mocks, , int, xio_open, XIO_HANDLE, handle, ON_BYTES_RECEIVED, on_bytes_received, ON_IO_STATE_CHANGED, on_io_state_changed, void*, callback_context);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , void, xio_dowork, XIO_HANDLE, ioHandle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , int, xio_close, XIO_HANDLE, ioHandle);
    DECLARE_GLOBAL_MOCK_METHOD_5(mqttclient_mocks, , int, xio_send, XIO_HANDLE, handle, const void*, buffer, size_t, size, ON_SEND_COMPLETE, on_send_complete, void*, callback_context);

    DECLARE_GLOBAL_MOCK_METHOD_0(mqttclient_mocks, , int, platform_init);
    DECLARE_GLOBAL_MOCK_METHOD_0(mqttclient_mocks, , void, platform_deinit);

    DECLARE_GLOBAL_MOCK_METHOD_0(mqttclient_mocks, , TICK_COUNTER_HANDLE, tickcounter_create);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , int, tickcounter_reset, TICK_COUNTER_HANDLE, tick_counter);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , void, tickcounter_destroy, TICK_COUNTER_HANDLE, tick_counter);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqttclient_mocks, , int, tickcounter_get_current_ms, TICK_COUNTER_HANDLE, tick_counter, uint64_t*, current_ms);

    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , size_t, BUFFER_length, BUFFER_HANDLE, handle);

    DECLARE_GLOBAL_MOCK_METHOD_5(mqttclient_mocks, , MQTT_MESSAGE_HANDLE, mqttmessage_create, uint16_t, packetId, const char*, topicName, QOS_VALUE, qosValue, const uint8_t*, appMsg, size_t, appLength);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , void, mqttmessage_destroyMessage, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , MQTT_MESSAGE_HANDLE, mqttmessage_clone, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , uint16_t, mqttmessage_getPacketId, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , const char*, mqttmessage_getTopicName, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , QOS_VALUE, mqttmessage_getQosType, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , bool, mqttmessage_getIsDuplicateMsg, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , bool, mqttmessage_getIsRetained, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqttclient_mocks, , int, mqttmessage_setIsDuplicateMsg, MQTT_MESSAGE_HANDLE, handle, bool, duplicateMsg);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqttclient_mocks, , int, mqttmessage_setIsRetained, MQTT_MESSAGE_HANDLE, handle, bool, retainMsg);

    DECLARE_GLOBAL_MOCK_METHOD_1(mqttclient_mocks, , const APP_PAYLOAD*, mqttmessage_getApplicationMsg, MQTT_MESSAGE_HANDLE, handle);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

#define TEST_CONTEXT ((const void*)0x4242)

BEGIN_TEST_SUITE(mqttclient_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = MicroMockCreateMutex();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    MicroMockDestroyMutex(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (!MicroMockAcquireMutex(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    g_fail_alloc_calls = false;
    g_current_ms = 0;
    g_packetComplete = NULL;
    g_operationCallbackInvoked = false;
    g_msgRecvCallbackInvoked = false;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    if (!MicroMockReleaseMutex(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not release test serialization mutex.");
    }
}

static void PrintLogFunction(unsigned int options, char* format, ...)
{
}

static int TestRecvCallback(MQTT_MESSAGE_HANDLE msgHandle, void* context)
{
    g_msgRecvCallbackInvoked = true;
    return 0;
}

static int TestOpCallback(MQTTCLIENT_ACTION_RESULT actionResult, void* msgInfo, void* context)
{
    return 1;
}

static int ctrlPacketIoSend(BUFFER_HANDLE dataHandle, void* callContext)
{

}

static void SetupMqttLibOptions(MQTTCLIENT_OPTIONS* options, const char* clientId,
    const char* willMsg,
    const char* willTopic,
    const char* username,
    const char* password,
    int keepAlive,
    bool messageRetain,
    bool cleanSession,
    QOS_VALUE qos)
{
    options->clientId = clientId;
    options->willMessage = willMsg;
    options->username = username;
    options->password = password;
    options->keepAliveInterval = keepAlive;
    options->useCleanSession = cleanSession;
    options->qualityOfServiceValue = qos;
}

/* mqttclient_connect */

END_TEST_SUITE(mqttclient_unittests)
