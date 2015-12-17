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
static const unsigned char* TEST_BUFFER_U_CHAR = (const unsigned char*)0x19;

static bool g_operationCallbackInvoked;
static bool g_msgRecvCallbackInvoked;
static bool g_fail_alloc_calls;
static uint64_t g_current_ms;
ON_PACKET_COMPLETE_CALLBACK g_packetComplete;
typedef struct TEST_COMPLETE_DATA_INSTANCE_TAG
{
    MQTT_CLIENT_ACTION_RESULT actionResult;
    void* msgInfo;
} TEST_COMPLETE_DATA_INSTANCE;

TYPED_MOCK_CLASS(mqtt_client_mocks, CGlobalMock)
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

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_connect, const MQTT_CLIENT_OPTIONS*, mqttOptions)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_7(, BUFFER_HANDLE, mqtt_codec_publish, QOS_VALUE, qosValue, bool, duplicateMsg, bool, serverRetain, uint16_t, packetId, const char*, topicName, const uint8_t*, msgBuffer, size_t, appLen)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_3(, BUFFER_HANDLE, mqtt_codec_subscribe, uint16_t, packetId, SUBSCRIBE_PAYLOAD*, payloadList, size_t, payloadCount)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_3(, BUFFER_HANDLE, mqtt_codec_unsubscribe, uint16_t, packetId, const char**, unsubscribeTopic, size_t, payloadCount)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_0(, BUFFER_HANDLE, mqtt_codec_disconnect)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_0(, BUFFER_HANDLE, mqtt_codec_ping)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishAck, uint16_t, packetId)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishRecieved, uint16_t, packetId)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);
    
    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishRelease, uint16_t, packetId)
    MOCK_METHOD_END(BUFFER_HANDLE, TEST_BUFFER_HANDLE);

    MOCK_STATIC_METHOD_1(, BUFFER_HANDLE, mqtt_codec_publishComplete, uint16_t, packetId)
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
        unsigned char* resBuff = (unsigned char*)TEST_BUFFER_U_CHAR;
        if (s != TEST_BUFFER_HANDLE)
        {
            resBuff = BASEIMPLEMENTATION::BUFFER_u_char(s);
        }
    MOCK_METHOD_END(unsigned char*, resBuff);

    MOCK_STATIC_METHOD_1(, size_t, BUFFER_length, BUFFER_HANDLE, s)
        size_t len = 11;
        if (s != TEST_BUFFER_HANDLE)
        {
            len = BASEIMPLEMENTATION::BUFFER_length(s);
        }
    MOCK_METHOD_END(size_t, 11);

    MOCK_STATIC_METHOD_1(, void, BUFFER_delete, BUFFER_HANDLE, s)
    MOCK_VOID_METHOD_END();

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
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , void, gballoc_free, void*, ptr);

    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_connect, const MQTT_CLIENT_OPTIONS*, mqttOptions);
    DECLARE_GLOBAL_MOCK_METHOD_7(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_publish, QOS_VALUE, qosValue, bool, duplicateMsg, bool, serverRetain, uint16_t, packetId, const char*, topicName, const uint8_t*, msgBuffer, size_t, appLen);
    DECLARE_GLOBAL_MOCK_METHOD_0(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_disconnect);
    DECLARE_GLOBAL_MOCK_METHOD_0(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_ping);
    DECLARE_GLOBAL_MOCK_METHOD_3(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_subscribe, uint16_t, packetId, SUBSCRIBE_PAYLOAD*, payloadList, size_t, payloadCount);

    DECLARE_GLOBAL_MOCK_METHOD_3(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_unsubscribe, uint16_t, packetId, const char**, unsubscribeTopic, size_t, payloadCount);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_publishAck, uint16_t, packetId);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_publishRecieved, uint16_t, packetId);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_publishRelease, uint16_t, packetId);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , BUFFER_HANDLE, mqtt_codec_publishComplete, uint16_t, packetId);

    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_client_mocks, , MQTTCODEC_HANDLE, mqtt_codec_create, ON_PACKET_COMPLETE_CALLBACK, packetComplete, void*, callContext);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , void, mqtt_codec_destroy, MQTTCODEC_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_3(mqtt_client_mocks, , int, mqtt_codec_bytesReceived, MQTTCODEC_HANDLE, handle, const void*, buffer, size_t, size);

    DECLARE_GLOBAL_MOCK_METHOD_4(mqtt_client_mocks, , int, xio_open, XIO_HANDLE, handle, ON_BYTES_RECEIVED, on_bytes_received, ON_IO_STATE_CHANGED, on_io_state_changed, void*, callback_context);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , void, xio_dowork, XIO_HANDLE, ioHandle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , int, xio_close, XIO_HANDLE, ioHandle);
    DECLARE_GLOBAL_MOCK_METHOD_5(mqtt_client_mocks, , int, xio_send, XIO_HANDLE, handle, const void*, buffer, size_t, size, ON_SEND_COMPLETE, on_send_complete, void*, callback_context);

    DECLARE_GLOBAL_MOCK_METHOD_0(mqtt_client_mocks, , int, platform_init);
    DECLARE_GLOBAL_MOCK_METHOD_0(mqtt_client_mocks, , void, platform_deinit);

    DECLARE_GLOBAL_MOCK_METHOD_0(mqtt_client_mocks, , TICK_COUNTER_HANDLE, tickcounter_create);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , int, tickcounter_reset, TICK_COUNTER_HANDLE, tick_counter);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , void, tickcounter_destroy, TICK_COUNTER_HANDLE, tick_counter);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_client_mocks, , int, tickcounter_get_current_ms, TICK_COUNTER_HANDLE, tick_counter, uint64_t*, current_ms);

    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , size_t, BUFFER_length, BUFFER_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , void, BUFFER_delete, BUFFER_HANDLE, s);

    DECLARE_GLOBAL_MOCK_METHOD_5(mqtt_client_mocks, , MQTT_MESSAGE_HANDLE, mqttmessage_create, uint16_t, packetId, const char*, topicName, QOS_VALUE, qosValue, const uint8_t*, appMsg, size_t, appLength);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , void, mqttmessage_destroyMessage, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , MQTT_MESSAGE_HANDLE, mqttmessage_clone, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , uint16_t, mqttmessage_getPacketId, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , const char*, mqttmessage_getTopicName, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , QOS_VALUE, mqttmessage_getQosType, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , bool, mqttmessage_getIsDuplicateMsg, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , bool, mqttmessage_getIsRetained, MQTT_MESSAGE_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_client_mocks, , int, mqttmessage_setIsDuplicateMsg, MQTT_MESSAGE_HANDLE, handle, bool, duplicateMsg);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_client_mocks, , int, mqttmessage_setIsRetained, MQTT_MESSAGE_HANDLE, handle, bool, retainMsg);

    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_client_mocks, , const APP_PAYLOAD*, mqttmessage_getApplicationMsg, MQTT_MESSAGE_HANDLE, handle);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

#define TEST_CONTEXT ((const void*)0x4242)

BEGIN_TEST_SUITE(mqtt_client_unittests)

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
    (void)options;
    (void)format;
}

static void TestRecvCallback(MQTT_MESSAGE_HANDLE msgHandle, void* context)
{
    (void)msgHandle;
    (void)context;
    g_msgRecvCallbackInvoked = true;
}

static void TestOpCallback(MQTT_CLIENT_ACTION_RESULT actionResult, void* msgInfo, void* context)
{
    (void)actionResult;
    (void)msgInfo;
    (void)context;
}

static void SetupMqttLibOptions(MQTT_CLIENT_OPTIONS* options, const char* clientId,
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
TEST_FUNCTION(mqtt_client_init_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange
    EXPECTED_CALL(mocks, platform_init());
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, mqtt_codec_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    MQTT_CLIENT_HANDLE result = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);

    // assert
    ASSERT_IS_NOT_NULL(result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(result);
}

/*Codes_SRS_MQTT_CLIENT_07_001: [If the parameters ON_MQTT_MESSAGE_RECV_CALLBACK is NULL then mqttclient_init shall return NULL.]*/
TEST_FUNCTION(mqtt_client_init_ON_MQTT_MESSAGE_RECV_CALLBACK_NULL_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange

    // act
    MQTT_CLIENT_HANDLE result = mqtt_client_init(NULL, TestOpCallback, NULL, PrintLogFunction);

    // assert
    ASSERT_IS_NULL(result);
}

/*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
TEST_FUNCTION(mqtt_client_init_Platform_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange
    EXPECTED_CALL(mocks, platform_init()).SetReturn(__LINE__);

    // act
    MQTT_CLIENT_HANDLE result = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);

    // assert
    ASSERT_IS_NULL(result);
}

/*Codes_SRS_MQTT_CLIENT_07_004: [If the parameter handle is NULL then function mqtt_client_deinit shall do nothing.]*/
TEST_FUNCTION(mqtt_client_deinit_handle_NULL_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange
    EXPECTED_CALL(mocks, platform_deinit());

    // act
    mqtt_client_deinit(NULL);

    // assert
}

/*Codes_SRS_MQTT_CLIENT_07_005: [mqtt_client_deinit shall deallocate all memory allocated in this unit.]*/
TEST_FUNCTION(mqtt_client_deinit_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, TEST_USERNAME, TEST_PASSWORD, TEST_KEEP_ALIVE_INTERVAL, false, true, DELIVER_AT_MOST_ONCE);
    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);

    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, tickcounter_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, gballoc_free(mqttHandle));
    EXPECTED_CALL(mocks, platform_deinit());
    EXPECTED_CALL(mocks, mqtt_codec_destroy(IGNORED_PTR_ARG));

    // act
    mqtt_client_deinit(mqttHandle);

    // assert
}

/*SRS_MQTT_CLIENT_07_006: [If any of the parameters handle, ioHandle, or mqttOptions are NULL then mqtt_client_connect shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_connect_MQTT_CLIENT_HANDLE_NULL_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, TEST_USERNAME, TEST_PASSWORD, TEST_KEEP_ALIVE_INTERVAL, false, true, DELIVER_AT_MOST_ONCE);

    // act
    int result = mqtt_client_connect(NULL, TEST_IO_HANDLE, &mqttOptions);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

/*SRS_MQTT_CLIENT_07_006: [If any of the parameters handle, ioHandle, or mqttOptions are NULL then mqtt_client_connect shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_connect_XIO_HANDLE_NULL_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, TEST_USERNAME, TEST_PASSWORD, TEST_KEEP_ALIVE_INTERVAL, false, true, DELIVER_AT_MOST_ONCE);

    // act
    int result = mqtt_client_connect(mqttHandle, NULL, &mqttOptions);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*SRS_MQTT_CLIENT_07_006: [If any of the parameters handle, ioHandle, or mqttOptions are NULL then mqtt_client_connect shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_connect_MQTT_CLIENT_OPTIONS_NULL_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange

    // act
    int result = mqtt_client_connect(mqttHandle, TEST_IO_HANDLE, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_connect_xio_open_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, TEST_USERNAME, TEST_PASSWORD, TEST_KEEP_ALIVE_INTERVAL, false, true, DELIVER_AT_MOST_ONCE);

    EXPECTED_CALL(mocks, xio_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, mqtt_codec_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mqtt_codec_destroy(IGNORED_PTR_ARG));

    // act
    int result = mqtt_client_connect(mqttHandle, TEST_IO_HANDLE, &mqttOptions);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_connect_mqtt_codec_create_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, TEST_USERNAME, TEST_PASSWORD, TEST_KEEP_ALIVE_INTERVAL, false, true, DELIVER_AT_MOST_ONCE);

    EXPECTED_CALL(mocks, mqtt_codec_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn((MQTTCODEC_HANDLE)NULL);

    // act
    int result = mqtt_client_connect(mqttHandle, TEST_IO_HANDLE, &mqttOptions);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_connect_mqtt_codec_connect_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, TEST_USERNAME, TEST_PASSWORD, TEST_KEEP_ALIVE_INTERVAL, false, true, DELIVER_AT_MOST_ONCE);

    EXPECTED_CALL(mocks, xio_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, mqtt_codec_connect(&mqttOptions)).SetReturn((BUFFER_HANDLE)NULL);
    EXPECTED_CALL(mocks, tickcounter_create());
    EXPECTED_CALL(mocks, mqtt_codec_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mqtt_codec_destroy(IGNORED_PTR_ARG));

    // act
    int result = mqtt_client_connect(mqttHandle, TEST_IO_HANDLE, &mqttOptions);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_009: [On success mqtt_client_connect shall send the MQTT CONNECT to the endpoint.]*/
TEST_FUNCTION(mqtt_client_connect_succeeds)
{
    // arrange
    mqtt_client_mocks mocks; 

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, TEST_USERNAME, TEST_PASSWORD, TEST_KEEP_ALIVE_INTERVAL, false, true, DELIVER_AT_MOST_ONCE);

    EXPECTED_CALL(mocks, xio_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mqtt_codec_connect(&mqttOptions));
    EXPECTED_CALL(mocks, tickcounter_create());
    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mqtt_codec_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_delete(TEST_BUFFER_HANDLE));

    // act
    int result = mqtt_client_connect(mqttHandle, TEST_IO_HANDLE, &mqttOptions);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_013: [If any of the parameters handle, subscribeList is NULL or count is 0 then mqtt_client_subscribe shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_subscribe_handle_NULL_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange

    // act
    int result = mqtt_client_subscribe(NULL, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

/*Codes_SRS_MQTT_CLIENT_07_013: [If any of the parameters handle, subscribeList is NULL or count is 0 then mqtt_client_subscribe shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_subscribe_subscribeList_NULL_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange

    mocks.ResetAllCalls();

    // act
    int result = mqtt_client_subscribe(mqttHandle, TEST_PACKET_ID, NULL, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_013: [If any of the parameters handle, subscribeList is NULL or count is 0 then mqtt_client_subscribe shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_subscribe_count_0_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // Arrange

    mocks.ResetAllCalls();

    // act
    int result = mqtt_client_subscribe(mqttHandle, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 0);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_015: [On success mqtt_client_subscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
TEST_FUNCTION(mqtt_client_subscribe_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, mqtt_codec_subscribe(TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2));
    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_length(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_delete(TEST_BUFFER_HANDLE));

    // act
    int result = mqtt_client_subscribe(mqttHandle, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_016: [If any of the parameters handle, unsubscribeList is NULL or count is 0 then mqtt_client_unsubscribe shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_unsubscribe_handle_NULL_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange

    // act
    int result = mqtt_client_unsubscribe(NULL, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
}

/*Codes_SRS_MQTT_CLIENT_07_016: [If any of the parameters handle, unsubscribeList is NULL or count is 0 then mqtt_client_unsubscribe shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_unsubscribe_unsubscribeList_NULL_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // act
    int result = mqtt_client_unsubscribe(mqttHandle, TEST_PACKET_ID, NULL, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_016: [If any of the parameters handle, unsubscribeList is NULL or count is 0 then mqtt_client_unsubscribe shall return a non-zero value.]*/
TEST_FUNCTION(mqtt_client_unsubscribe_count_0_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // act
    int result = mqtt_client_unsubscribe(mqttHandle, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 0);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_018: [On success mqtt_client_unsubscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
TEST_FUNCTION(mqtt_client_unsubscribe_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, mqtt_codec_unsubscribe(TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2));
    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_length(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_delete(TEST_BUFFER_HANDLE));

    // act
    int result = mqtt_client_unsubscribe(mqttHandle, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

TEST_FUNCTION(mqtt_client_publish_handle_NULL_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange

    // act
    int result = mqtt_client_publish(NULL, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
}

TEST_FUNCTION(mqtt_client_publish_MQTT_MESSAGE_HANDLE_NULL_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    // act
    int result = mqtt_client_publish(mqttHandle, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

TEST_FUNCTION(mqtt_client_publish_mqtt_codec_publish_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, mqttmessage_getPacketId(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getTopicName(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getQosType(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getIsDuplicateMsg(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getIsRetained(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getApplicationMsg(TEST_MESSAGE_HANDLE));
    EXPECTED_CALL(mocks, mqtt_codec_publish(DELIVER_AT_MOST_ONCE, true, true, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .SetReturn((BUFFER_HANDLE)NULL);

    // act
    int result = mqtt_client_publish(mqttHandle, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

TEST_FUNCTION(mqtt_client_publish_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    STRICT_EXPECTED_CALL(mocks, mqttmessage_getApplicationMsg(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getQosType(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getIsDuplicateMsg(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getIsRetained(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getPacketId(TEST_MESSAGE_HANDLE));
    STRICT_EXPECTED_CALL(mocks, mqttmessage_getTopicName(TEST_MESSAGE_HANDLE));
    EXPECTED_CALL(mocks, mqtt_codec_publish(DELIVER_AT_MOST_ONCE, true, true, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_length(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_delete(TEST_BUFFER_HANDLE));

    // act
    int result = mqtt_client_publish(mqttHandle, TEST_MESSAGE_HANDLE);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

TEST_FUNCTION(mqtt_client_disconnect_handle_NULL_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    // Arrange

    // act
    int result = mqtt_client_disconnect(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(mqtt_client_disconnect_mqtt_codec_NULL_fail)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, mqtt_codec_disconnect()).SetReturn((BUFFER_HANDLE)NULL);

    // act
    int result = mqtt_client_disconnect(mqttHandle);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

TEST_FUNCTION(mqtt_client_disconnect_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, mqtt_codec_disconnect());
    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_length(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_delete(TEST_BUFFER_HANDLE));

    // act
    int result = mqtt_client_disconnect(mqttHandle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_023: [If the parameter handle is NULL then mqtt_client_dowork shall do nothing.]*/
TEST_FUNCTION(mqtt_client_dowork_ping_handle_NULL_fails)
{
    // arrange
    mqtt_client_mocks mocks;

    mocks.ResetAllCalls();

    // act
    mqtt_client_dowork(NULL);

    // assert
    mocks.AssertActualAndExpectedCalls();

    // cleanup
}

/*Codes_SRS_MQTT_CLIENT_07_024: [mqtt_client_dowork shall call the xio_dowork function to complete operations.]*/
/*Codes_SRS_MQTT_CLIENT_07_025: [mqtt_client_dowork shall retrieve the the last packet send value and ...]*/
/*Codes_SRS_MQTT_CLIENT_07_026: [if this value is greater than the MQTT KeepAliveInterval then it shall construct an MQTT PINGREQ packet.]*/
TEST_FUNCTION(mqtt_client_dowork_ping_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    g_current_ms = TEST_KEEP_ALIVE_INTERVAL*1000;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_dowork(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mqtt_codec_ping());
    STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_length(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_delete(TEST_BUFFER_HANDLE));

    // act
    mqtt_client_dowork(mqttHandle);

    // assert
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}

/*Codes_SRS_MQTT_CLIENT_07_024: [mqtt_client_dowork shall call the xio_dowork function to complete operations.]*/
/*Codes_SRS_MQTT_CLIENT_07_025: [mqtt_client_dowork shall retrieve the the last packet send value and ...]*/
TEST_FUNCTION(mqtt_client_dowork_no_ping_succeeds)
{
    // arrange
    mqtt_client_mocks mocks;

    g_current_ms = (TEST_KEEP_ALIVE_INTERVAL-5)*1000;

    MQTT_CLIENT_HANDLE mqttHandle = mqtt_client_init(TestRecvCallback, TestOpCallback, NULL, PrintLogFunction);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, tickcounter_get_current_ms(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, xio_dowork(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mqtt_codec_ping());
    STRICT_EXPECTED_CALL(mocks, BUFFER_u_char(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_length(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(mocks, BUFFER_delete(TEST_BUFFER_HANDLE));

    // act
    mqtt_client_dowork(mqttHandle);

    // assert
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_client_deinit(mqttHandle);
}
END_TEST_SUITE(mqtt_client_unittests)
