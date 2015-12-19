// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "mqtt_codec.h"
#include "buffer_.h"
#include "lock.h"

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

static bool g_fail_alloc_calls;
static bool g_callbackInvoked;
static CONTROL_PACKET_TYPE g_curr_packet_type;
static const char* TEST_SUBSCRIPTION_TOPIC = "subTopic";
static const char* TEST_CLIENT_ID = "single_threaded_test";
static const char* TEST_TOPIC_NAME = "topic Name";
static const uint8_t* TEST_MESSAGE = (const uint8_t*)"Message to send";
static const uint16_t TEST_MESSAGE_LEN = 15;
static SUBSCRIBE_PAYLOAD TEST_SUBSCRIBE_PAYLOAD[] = { { "subTopic1", DELIVER_AT_LEAST_ONCE },{ "subTopic2", DELIVER_EXACTLY_ONCE } };
static const char* TEST_UNSUBSCRIPTION_TOPIC[] = { "subTopic1", "subTopic2" };

#define TEST_HANDLE             0x11
#define TEST_PACKET_ID          0x1234
#define TEST_CALL_CONTEXT       0x1235
#define TEST_LIST_ITEM_HANDLE   0x1236
#define FIXED_HEADER_SIZE       2


typedef struct TEST_COMPLETE_DATA_INSTANCE_TAG
{
    unsigned char* dataHeader;
    size_t Length;
} TEST_COMPLETE_DATA_INSTANCE;

#ifdef CPP_UNITTEST
template <> static std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString < CONTROL_PACKET_TYPE >(const CONTROL_PACKET_TYPE & packet)
{
    std::wstring result;
    switch (packet)
    {
        case CONNECT_TYPE: result = L"CONNECT";
        case CONNACK_TYPE:  result = L"CONNACK";
        case PUBLISH_TYPE:  result = L"PUBLISH";
        case PUBACK_TYPE:  result = L"PUBACK";
        case PUBREC_TYPE:  result = L"PUBREC";
        case PUBREL_TYPE:  result = L"PUBREL";
        case SUBSCRIBE_TYPE:  result = L"SUBSCRIBE";
        case SUBACK_TYPE:  result = L"SUBACK";
        case UNSUBSCRIBE_TYPE:  result = L"UNSUBSCRIBE";
        case UNSUBACK_TYPE:  result = L"UNSUBACK";
        case PINGREQ_TYPE:  result = L"PINGREQ";
        case PINGRESP_TYPE:  result = L"PINGRESP";
        case DISCONNECT_TYPE:  result = L"DISCONNECT";
        default:
        case PACKET_TYPE_ERROR:
        case UNKNOWN_TYPE:
             result = L"UNKNOWN";
    }
    return result;
}
#endif

static int CONTROL_PACKET_TYPE_Compare(CONTROL_PACKET_TYPE left, CONTROL_PACKET_TYPE right)
{
    return (left != right);
}

static void CONTROL_PACKET_TYPE_ToString(char* string, size_t bufferSize, CONTROL_PACKET_TYPE val)
{
    (void)bufferSize;
    std::ostringstream o;
    o << val;
    strcpy(string, o.str().c_str());
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

TYPED_MOCK_CLASS(mqtt_codec_mocks, CGlobalMock)
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

    MOCK_STATIC_METHOD_0(, BUFFER_HANDLE, BUFFER_new)
        BUFFER_HANDLE result2 = BASEIMPLEMENTATION::BUFFER_new();
    MOCK_METHOD_END(BUFFER_HANDLE, result2);

    MOCK_STATIC_METHOD_3(, int, BUFFER_build, BUFFER_HANDLE, handle, const unsigned char*, source, size_t, size)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::BUFFER_build(handle, source, size));

    MOCK_STATIC_METHOD_2(, int, BUFFER_enlarge, BUFFER_HANDLE, handle, size_t, size)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::BUFFER_enlarge(handle, size));

    MOCK_STATIC_METHOD_2(, int, BUFFER_pre_build, BUFFER_HANDLE, handle, size_t, size)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::BUFFER_pre_build(handle, size));

    MOCK_STATIC_METHOD_2(, int, BUFFER_prepend, BUFFER_HANDLE, handle1, BUFFER_HANDLE, handle2)
    MOCK_METHOD_END(int, BASEIMPLEMENTATION::BUFFER_prepend(handle1, handle2));

    MOCK_STATIC_METHOD_1(, void, BUFFER_delete, BUFFER_HANDLE, s)
        BASEIMPLEMENTATION::BUFFER_delete(s);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, unsigned char*, BUFFER_u_char, BUFFER_HANDLE, s)
    MOCK_METHOD_END(unsigned char*, BASEIMPLEMENTATION::BUFFER_u_char(s) );

    MOCK_STATIC_METHOD_1(, size_t, BUFFER_length, BUFFER_HANDLE, s)
    MOCK_METHOD_END(size_t, BASEIMPLEMENTATION::BUFFER_length(s));
};

extern "C"
{
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_codec_mocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_codec_mocks, , void, gballoc_free, void*, ptr);

    DECLARE_GLOBAL_MOCK_METHOD_0(mqtt_codec_mocks, , BUFFER_HANDLE, BUFFER_new);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_codec_mocks, , void, BUFFER_delete, BUFFER_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_3(mqtt_codec_mocks, , int, BUFFER_build, BUFFER_HANDLE, handle, const unsigned char*, source, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_codec_mocks, , int, BUFFER_enlarge, BUFFER_HANDLE, handle, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_codec_mocks, , int, BUFFER_pre_build, BUFFER_HANDLE, handle, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_codec_mocks, , int, BUFFER_prepend, BUFFER_HANDLE, handle1, BUFFER_HANDLE, handle2);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_codec_mocks, , unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_codec_mocks, , size_t, BUFFER_length, BUFFER_HANDLE, handle);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(mqtt_codec_unittests)

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
    g_callbackInvoked = false;
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

static void TestOnCompleteCallback(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData)
{
    TEST_COMPLETE_DATA_INSTANCE* testData = (TEST_COMPLETE_DATA_INSTANCE*)context;
    if (testData != NULL)
    {
        if (packet == PINGRESP_TYPE)
        {
            g_callbackInvoked = true;
        }
        else if (testData->Length > 0 && testData->dataHeader != NULL)
        {
            if (memcmp(testData->dataHeader, BASEIMPLEMENTATION::BUFFER_u_char(headerData), testData->Length) == 0)
            {
                g_callbackInvoked = true;
            }
        }
    }
}

/* Tests_SRS_MQTT_CODEC_07_002: [On success mqtt_codec_create shall return a MQTTCODEC_HANDLE value.] */
TEST_FUNCTION(mqtt_codec_create_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));

    // act
    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, NULL);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    mqtt_codec_destroy(handle);
}

/* Tests_SRS_MQTT_CODEC_07_004: [mqtt_codec_destroy shall deallocate all memory that has been allocated by this object.] */
TEST_FUNCTION(mqtt_codec_destroy_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, NULL);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    mqtt_codec_destroy(handle);

    // assert
    mocks.AssertActualAndExpectedCalls();
}

/* Tests_SRS_MQTT_CODEC_07_003: [If the handle parameter is NULL then mqtt_codec_destroy shall do nothing.] */
TEST_FUNCTION(mqtt_codec_destroy_handle_NULL_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    // act
    mqtt_codec_destroy(NULL);

    // assert
}

/* Tests_SRS_MQTT_CODEC_07_008: [If the parameters mqttOptions is NULL then mqtt_codec_connect shall return a null value.] */
TEST_FUNCTION(mqtt_codec_connect_MQTTCLIENT_OPTIONS_NULL_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    // act
    BUFFER_HANDLE handle = mqtt_codec_connect(NULL);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_010: [If any error is encountered then mqtt_codec_connect shall return NULL.] */
TEST_FUNCTION(mqtt_codec_connect_BUFFER_enlarge_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, "testuser", "testpassword", 20, false, true, DELIVER_AT_MOST_ONCE);

    STRICT_EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_connect(&mqttOptions);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_009: [mqtt_codec_connect shall construct a BUFFER_HANDLE that represents a MQTT CONNECT packet.] */
TEST_FUNCTION(mqtt_codec_connect_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;
    MQTT_CLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, "testuser", "testpassword", 20, false, true, DELIVER_AT_MOST_ONCE);

    const unsigned char CONNECT_VALUE[] = { 0x10, 0x38, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54, 0x04, 0xc2, 0x00, 0x14, 0x00, 0x14, 0x73, 0x69, \
        0x6e, 0x67, 0x6c, 0x65, 0x5f, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x65, 0x64, 0x5f, 0x74, 0x65, 0x73, 0x74, 0x00, 0x08, 0x74, \
        0x65, 0x73, 0x74, 0x75, 0x73, 0x65, 0x72, 0x00, 0x0c, 0x74, 0x65, 0x73, 0x74, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64 };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_connect(&mqttOptions);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, CONNECT_VALUE, length));

    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Tests_SRS_MQTT_CODEC_07_011: [On success mqtt_codec_disconnect shall construct a BUFFER_HANDLE that represents a MQTT DISCONNECT packet.] */
TEST_FUNCTION(mqtt_codec_disconnect_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    const unsigned char DISCONNECT_VALUE[] = { 0xE0, 0x00 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_disconnect();

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, DISCONNECT_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Tests_SRS_MQTT_CODEC_07_012: [If any error is encountered mqtt_codec_disconnect shall return NULL.] */
TEST_FUNCTION(mqtt_codec_disconnect_BUFFER_enlarge_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    const unsigned char DISCONNECT_VALUE[] = { 0xE0, 0x00 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_disconnect();

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_022: [If any error is encountered mqtt_codec_ping shall return NULL.] */
TEST_FUNCTION(mqtt_codec_ping_BUFFER_enlarge_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    const unsigned char PING_VALUE[] = { 0xC0, 0x00 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_ping();

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_021: [On success mqtt_codec_ping shall construct a BUFFER_HANDLE that represents a MQTT PINGREQ packet.] */
TEST_FUNCTION(mqtt_codec_ping_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    const unsigned char PING_VALUE[] = { 0xC0, 0x00 };

    STRICT_EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_ping();

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, PING_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Tests_SRS_MQTT_CODEC_07_005: [If the parameters topicName, or msgBuffer is NULL or if buffLen is 0 then mqtt_codec_publish shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_topicName_NULL_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    // act
    BUFFER_HANDLE handle = mqtt_codec_publish(DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, NULL, TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_005: [If the parameters topicName, or msgBuffer is NULL or if buffLen is 0 then mqtt_codec_publish shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_msgBuffer_NULL_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    // act
    BUFFER_HANDLE handle = mqtt_codec_publish(DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, NULL, 0);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_005: [If the parameters topicName, or msgBuffer is NULL or if buffLen is 0 then mqtt_codec_publish shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_buffLen_0_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    // act
    BUFFER_HANDLE handle = mqtt_codec_publish(DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, NULL, TEST_MESSAGE, 0);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_CONTROL_PACKET_07_052: [mqtt_codec_publish shall constuct the MQTT variable header and shall return a non-zero value on failure.] */
TEST_FUNCTION(mqtt_codec_publish_construct_BUFFER_enlarge_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_publish(DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_006: [If any error is encountered then mqtt_codec_publish shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_BUFFER_enlarge_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publish(DELIVER_AT_LEAST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_006: [If any error is encountered then mqtt_codec_publish shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_constructFixedHeader_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publish(DELIVER_AT_LEAST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_007: [mqtt_codec_publish shall return a BUFFER_HANDLE that represents a MQTT PUBLISH message.] */
TEST_FUNCTION(mqtt_codec_publish_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    const unsigned char PUBLISH_VALUE[] = { 0x3a, 0x1f, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x12, 0x34, 0x00, 0x0f, 0x4d, 0x65, \
        0x73, 0x73, 0x61, 0x67, 0x65, 0x20, 0x74, 0x6f, 0x20, 0x73, 0x65, 0x6e, 0x64 };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_publish(DELIVER_AT_LEAST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, TEST_MESSAGE, TEST_MESSAGE_LEN);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, PUBLISH_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Tests_SRS_MQTT_CODEC_07_013: [On success mqtt_codec_publishAck shall return a BUFFER_HANDLE representation of a MQTT PUBACK packet.] */
TEST_FUNCTION(mqtt_codec_publish_ack_pre_build_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x40, 0x02, 0x81, 0x58 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishAck(TEST_PACKET_ID);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Tests_SRS_MQTT_CODEC_07_014 : [If any error is encountered then mqtt_codec_publishAck shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_ack_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x40, 0x02, 0x12, 0x34 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishAck(TEST_PACKET_ID);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, PUBLISH_ACK_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Codes_SRS_MQTT_CODEC_07_016 : [If any error is encountered then mqtt_codec_publishRecieved shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_received_pre_build_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    STRICT_EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishReceived(TEST_PACKET_ID);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_015: [On success mqtt_codec_publishRecieved shall return a BUFFER_HANDLE representation of a MQTT PUBREC packet.] */
TEST_FUNCTION(mqtt_codec_publish_received_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x50, 0x02, 0x12, 0x34 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishReceived(TEST_PACKET_ID);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, PUBLISH_ACK_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Codes_SRS_MQTT_CODEC_07_018 : [If any error is encountered then mqtt_codec_publishRelease shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_release_pre_build_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishRelease(TEST_PACKET_ID);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_017: [On success mqtt_codec_publishRelease shall return a BUFFER_HANDLE representation of a MQTT PUBREL packet.] */
TEST_FUNCTION(mqtt_codec_publish_release_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x62, 0x02, 0x12, 0x34 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishRelease(TEST_PACKET_ID);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, PUBLISH_ACK_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Codes_SRS_MQTT_CODEC_07_020 : [If any error is encountered then mqtt_codec_publishComplete shall return NULL.] */
TEST_FUNCTION(mqtt_codec_publish_complete_pre_build_fail)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishComplete(TEST_PACKET_ID);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_019: [On success mqtt_codec_publishComplete shall return a BUFFER_HANDLE representation of a MQTT PUBCOMP packet.] */
TEST_FUNCTION(mqtt_codec_publish_complete_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char PUBLISH_COMP_VALUE[] = { 0x70, 0x02, 0x12, 0x34 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);

    // act
    BUFFER_HANDLE handle = mqtt_codec_publishComplete(TEST_PACKET_ID);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, PUBLISH_COMP_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Codes_SRS_MQTT_CODEC_07_023: [If the parameters subscribeList is NULL or if count is 0 then mqtt_codec_subscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_subscribe_subscribeList_NULL_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    // act
    BUFFER_HANDLE handle = mqtt_codec_subscribe(TEST_PACKET_ID, NULL, 0);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_025: [If any error is encountered then mqtt_codec_subscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_subscribe_BUFFER_enlarge_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_subscribe(TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_025: [If any error is encountered then mqtt_codec_subscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_subscribe_addListItemsToSubscribePacket_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_subscribe(TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_025: [If any error is encountered then mqtt_codec_subscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_subscribe_constructFixedHeader_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

    // act
    BUFFER_HANDLE handle = mqtt_codec_subscribe(TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_026: [mqtt_codec_subscribe shall return a BUFFER_HANDLE that represents a MQTT SUBSCRIBE message.]*/
/* Codes_SRS_MQTT_CODEC_07_024: [mqtt_codec_subscribe shall iterate through count items in the subscribeList.] */
TEST_FUNCTION(mqtt_codec_subscribe_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char SUBSCRIBE_VALUE[] = { 0x82, 0x1a, 0x12, 0x34, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x01, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32, 0x02 };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_subscribe(TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, SUBSCRIBE_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Codes_SRS_MQTT_CODEC_07_027: [If the parameters unsubscribeList is NULL or if count is 0 then mqtt_codec_unsubscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_unsubscribe_subscribeList_NULL_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };

    // act
    BUFFER_HANDLE handle = mqtt_codec_unsubscribe(TEST_PACKET_ID, NULL, 0);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_029: [If any error is encountered then mqtt_codec_unsubscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_unsubscribe_BUFFER_enlarge_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_unsubscribe(TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_029: [If any error is encountered then mqtt_codec_unsubscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_unsubscribe_addListItemToUnsubscribePacket_BUFFER_enlarge_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_unsubscribe(TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_029: [If any error is encountered then mqtt_codec_unsubscribe shall return NULL.] */
TEST_FUNCTION(mqtt_codec_unsubscribe_constructFixedHeader_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0xa0, 0x18, 0x81, 0x58, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32 };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

    // act
    BUFFER_HANDLE handle = mqtt_codec_unsubscribe(TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_IS_NULL(handle);
}

/* Codes_SRS_MQTT_CODEC_07_030: [mqtt_codec_unsubscribe shall return a BUFFER_HANDLE that represents a MQTT SUBSCRIBE message.] */
TEST_FUNCTION(mqtt_codec_unsubscribe_succeeds)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0xa2, 0x18, 0x12, 0x34, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32 };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    BUFFER_HANDLE handle = mqtt_codec_unsubscribe(TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    unsigned char* data = BASEIMPLEMENTATION::BUFFER_u_char(handle);
    size_t length = BUFFER_length(handle);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data, UNSUBSCRIBE_VALUE, length));

    // cleanup
    mocks.AssertActualAndExpectedCalls();
    BASEIMPLEMENTATION::BUFFER_delete(handle);
}

/* Codes_SRS_MQTT_CODEC_07_031: [If the parameters handle or buffer is NULL then mqtt_codec_bytesReceived shall return a non-zero value.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_MQTTCODEC_HANDLE_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char CONNACK_RESP[] = { 0x20, 0x2, 0x1, 0x0 };

    // act
    mqtt_codec_bytesReceived(NULL, CONNACK_RESP, 1);

    // assert
}

/* Codes_SRS_MQTT_CODEC_07_031: [If the parameters handle or buffer is NULL then mqtt_codec_bytesReceived shall return a non-zero value.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_buffer_NULL_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, NULL);

    mocks.ResetAllCalls();

    // act
    mqtt_codec_bytesReceived(handle, NULL, 1);
    
    // assert
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

/* Codes_SRS_MQTT_CODEC_07_032: [If the parameters size is zero then mqtt_codec_bytesReceived shall return a non-zero value.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_buffer_Len_0_fails)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char CONNACK_RESP[] = { 0x20, 0x2, 0x1, 0x0 };

    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, NULL);

    mocks.ResetAllCalls();

    // act
    mqtt_codec_bytesReceived(handle, CONNACK_RESP, 0);

    // assert
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

/* Codes_SRS_MQTT_CODEC_07_033: [mqtt_codec_bytesReceived constructs a sequence of bytes into the corresponding MQTT packets and on success returns zero.] */
/* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_connack_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char CONNACK_RESP[] = { 0x20, 0x2, 0x1, 0x0 };
    unsigned char TEST_CONNACK_VAR[] = { 0x1, 0x0 };
    size_t length = sizeof(CONNACK_RESP) / sizeof(CONNACK_RESP[0]);

    TEST_COMPLETE_DATA_INSTANCE testData = { 0 };
    testData.dataHeader = CONNACK_RESP + FIXED_HEADER_SIZE;
    testData.Length = 2;

    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, &testData);

    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    g_curr_packet_type = CONNACK_TYPE;

    // act
    for (size_t index = 0; index < length; index++)
    {
        // Send 1 byte at a time
        mqtt_codec_bytesReceived(handle, CONNACK_RESP+index, 1);
    }

    // assert
    ASSERT_IS_TRUE(g_callbackInvoked);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

/* Codes_SRS_MQTT_CODEC_07_033: [mqtt_codec_bytesReceived constructs a sequence of bytes into the corresponding MQTT packets and on success returns zero.] */
/* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_puback_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char PUBACK_RESP[] = { 0x40, 0x2, 0x12, 0x34 };
    unsigned char TEST_PUBACK_VAR[] = { 0x12, 0x34 };
    size_t length = sizeof(PUBACK_RESP) / sizeof(PUBACK_RESP[0]);

    TEST_COMPLETE_DATA_INSTANCE testData = { 0 };
    testData.dataHeader = PUBACK_RESP + FIXED_HEADER_SIZE;
    testData.Length = length - FIXED_HEADER_SIZE;

    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, &testData);

    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedTimesExactly(2);
    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    g_curr_packet_type = PUBACK_TYPE;

    // act
    for (size_t index = 0; index < length; index++)
    {
        // Send 1 byte at a time
        mqtt_codec_bytesReceived(handle, PUBACK_RESP + index, 1);
    }

    // assert
    ASSERT_IS_TRUE(g_callbackInvoked);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

/* Codes_SRS_MQTT_CODEC_07_033: [mqtt_codec_bytesReceived constructs a sequence of bytes into the corresponding MQTT packets and on success returns zero.] */
/* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_pingresp_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    unsigned char PINGRESP_RESP[] = { 0xD0, 0x0 };
    size_t length = sizeof(PINGRESP_RESP) / sizeof(PINGRESP_RESP[0]);

    TEST_COMPLETE_DATA_INSTANCE testData = { 0 };

    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, &testData);

    mocks.ResetAllCalls();
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    g_curr_packet_type = PINGRESP_TYPE;

    // act
    for (size_t index = 0; index < length; index++)
    {
        // Send 1 byte at a time
        mqtt_codec_bytesReceived(handle, PINGRESP_RESP + index, 1);
    }

    // assert
    ASSERT_IS_TRUE(g_callbackInvoked);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

/* Codes_SRS_MQTT_CODEC_07_033: [mqtt_codec_bytesReceived constructs a sequence of bytes into the corresponding MQTT packets and on success returns zero.] */
/* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_publish_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    g_curr_packet_type = PUBLISH_TYPE;

    //                            1    2     3     4     T     o     p     i     c     10    11    d     a     t     a     sp    M     s     g
    unsigned char PUBLISH[] = { 0x3F, 0x11, 0x00, 0x06, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x12, 0x34, 0x64, 0x61, 0x74, 0x61, 0x20, 0x4d, 0x73, 0x67 };
    size_t length = sizeof(PUBLISH) / sizeof(PUBLISH[0]);
    TEST_COMPLETE_DATA_INSTANCE testData = { 0 };
    testData.dataHeader = PUBLISH + FIXED_HEADER_SIZE;
    testData.Length = length - FIXED_HEADER_SIZE;

    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, &testData);

    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedTimesExactly(testData.Length);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedTimesExactly(testData.Length);
    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    for (size_t index = 0; index < length; index++)
    {
        // Send 1 byte at a time
        mqtt_codec_bytesReceived(handle, PUBLISH + index, 1);
    }

    // assert
    ASSERT_IS_TRUE(g_callbackInvoked);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

/* Codes_SRS_MQTT_CODEC_07_033: [mqtt_codec_bytesReceived constructs a sequence of bytes into the corresponding MQTT packets and on success returns zero.] */
/* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_suback_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    g_curr_packet_type = SUBACK_TYPE;

    unsigned char SUBACK_RESP[] = { 0x90, 0x5, 0x12, 0x34, 0x01, 0x80, 0x02 };
    size_t length = sizeof(SUBACK_RESP) / sizeof(SUBACK_RESP[0]);

    TEST_COMPLETE_DATA_INSTANCE testData = { 0 };
    testData.dataHeader = SUBACK_RESP + FIXED_HEADER_SIZE;
    testData.Length = length - FIXED_HEADER_SIZE;
    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, &testData);

    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedTimesExactly(testData.Length);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedTimesExactly(testData.Length);
    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    for (size_t index = 0; index < length; index++)
    {
        // Send 1 byte at a time
        mqtt_codec_bytesReceived(handle, SUBACK_RESP + index, 1);
    }

    // assert
    ASSERT_IS_TRUE(g_callbackInvoked);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

/* Codes_SRS_MQTT_CODEC_07_033: [mqtt_codec_bytesReceived constructs a sequence of bytes into the corresponding MQTT packets and on success returns zero.] */
/* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
TEST_FUNCTION(mqtt_codec_bytesReceived_unsuback_succeed)
{
    // arrange
    mqtt_codec_mocks mocks;

    g_curr_packet_type = UNSUBACK_TYPE;

    unsigned char UNSUBACK_RESP[] = { 0xB0, 0x5, 0x12, 0x34, 0x01, 0x80, 0x02 };
    size_t length = sizeof(UNSUBACK_RESP) / sizeof(UNSUBACK_RESP[0]);

    TEST_COMPLETE_DATA_INSTANCE testData = { 0 };
    testData.dataHeader = UNSUBACK_RESP + FIXED_HEADER_SIZE;
    testData.Length = length - FIXED_HEADER_SIZE;
    MQTTCODEC_HANDLE handle = mqtt_codec_create(TestOnCompleteCallback, &testData);

    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedTimesExactly(1);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedTimesExactly(testData.Length);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedTimesExactly(testData.Length);
    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    for (size_t index = 0; index < length; index++)
    {
        // Send 1 byte at a time
        mqtt_codec_bytesReceived(handle, UNSUBACK_RESP + index, 1);
    }

    // assert
    ASSERT_IS_TRUE(g_callbackInvoked);
    mocks.AssertActualAndExpectedCalls();

    // cleanup
    mqtt_codec_destroy(handle);
}

END_TEST_SUITE(mqtt_codec_unittests)
