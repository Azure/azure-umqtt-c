// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "control_packet.h"
#include "lock.h"
#include "buffer_.h"
#include "list.h"
#include "data_byte_util.h"

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
    #include "data_byte_util.c"
};

typedef struct TEST_VALUE_DATA_INSTANCE_TAG
{
    unsigned char* dataTest;
    size_t dataLen;
} TEST_VALUE_DATA_INSTANCE;

#define TEST_HANDLE             0x11
#define TEST_CALL_CONTEXT       0x1235
static const LIST_ITEM_HANDLE TEST_LIST_ITEM_HANDLE = (LIST_ITEM_HANDLE)0x1236;
static const LIST_HANDLE TEST_LIST_HANDLE = (LIST_HANDLE)0x1237;
static const int TEST_PACKET_ID = 33112;

static const char* TEST_TOPIC_NAME = "topic Name";
static const unsigned char* TEST_MESSAGE = (const unsigned char*)"Message to send";
static const int TEST_MESSAGE_LEN = 15;
static const char* TEST_CLIENT_ID = "single_threaded_test";
static const char* TEST_LIST_ITEM_VALUE = "test_list_item_value";
static SUBSCRIBE_PAYLOAD TEST_SUBSCRIBE_PAYLOAD[] = { { "subTopic1", DELIVER_AT_LEAST_ONCE },{ "subTopic2", DELIVER_EXACTLY_ONCE } };
static const char* TEST_UNSUBSCRIPTION_TOPIC[] = { "subTopic1", "subTopic2" };


//SUBCRIPTION_LIST_VALUE g_subListValue = { TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE };
static bool g_fail_alloc_calls;
static int g_test_Io_Send_Result = 0;
static bool g_callbackInvoked;

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
        case PACKET_TYPE_ERROR:  result = L"ERROR";
        default:
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

static int bool_Compare(bool left, bool right)
{
    return left != right;
}

static void bool_ToString(char* string, size_t bufferSize, bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}

TYPED_MOCK_CLASS(control_packet_mocks, CGlobalMock)
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

    MOCK_STATIC_METHOD_1(, BYTE, byteutil_readByte, BYTE**, buffer)
    MOCK_METHOD_END(BYTE, BASEIMPLEMENTATION::byteutil_readByte(buffer));

    MOCK_STATIC_METHOD_2(, void, byteutil_writeByte, char**, buffer, char, value)
        BASEIMPLEMENTATION::byteutil_writeByte(buffer, value);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_1(, BYTE, byteutil_readInt, BYTE**, buffer)
    MOCK_METHOD_END(BYTE, BASEIMPLEMENTATION::byteutil_readInt(buffer));

    MOCK_STATIC_METHOD_2(, void, byteutil_writeInt, char**, buffer, int, value)
        BASEIMPLEMENTATION::byteutil_writeInt(buffer, value);
    MOCK_VOID_METHOD_END();

    MOCK_STATIC_METHOD_3(, void, byteutil_writeUTF, char**, buffer, const char*, stringData, size_t, len)
        BASEIMPLEMENTATION::byteutil_writeUTF(buffer, stringData, len);
    MOCK_VOID_METHOD_END();
};

extern "C"
{
    DECLARE_GLOBAL_MOCK_METHOD_1(control_packet_mocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(control_packet_mocks, , void, gballoc_free, void*, ptr);

    DECLARE_GLOBAL_MOCK_METHOD_0(control_packet_mocks, , BUFFER_HANDLE, BUFFER_new);
    DECLARE_GLOBAL_MOCK_METHOD_1(control_packet_mocks, , void, BUFFER_delete, BUFFER_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_3(control_packet_mocks, , int, BUFFER_build, BUFFER_HANDLE, handle, const unsigned char*, source, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_2(control_packet_mocks, , int, BUFFER_enlarge, BUFFER_HANDLE, handle, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_2(control_packet_mocks, , int, BUFFER_pre_build, BUFFER_HANDLE, handle, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_2(control_packet_mocks, , int, BUFFER_prepend, BUFFER_HANDLE, handle1, BUFFER_HANDLE, handle2);
    DECLARE_GLOBAL_MOCK_METHOD_1(control_packet_mocks, , unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle);
    DECLARE_GLOBAL_MOCK_METHOD_1(control_packet_mocks, , size_t, BUFFER_length, BUFFER_HANDLE, handle);

    DECLARE_GLOBAL_MOCK_METHOD_1(control_packet_mocks, , BYTE, byteutil_readByte, BYTE**, buffer);
    DECLARE_GLOBAL_MOCK_METHOD_2(control_packet_mocks, , void, byteutil_writeByte, char**, buffer, char, value);
    DECLARE_GLOBAL_MOCK_METHOD_1(control_packet_mocks, , int, byteutil_readInt, BYTE**, buffer);
    DECLARE_GLOBAL_MOCK_METHOD_2(control_packet_mocks, , void, byteutil_writeInt, char**, buffer, int, value);
    DECLARE_GLOBAL_MOCK_METHOD_3(control_packet_mocks, , void, byteutil_writeUTF, char**, buffer, const char*, stringData, size_t, len);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

#define TEST_CONTEXT ((const void*)0x4242)

BEGIN_TEST_SUITE(control_packet_unittests)

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
    g_test_Io_Send_Result = 0;
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

static int TestIoSend(const BYTE* data, size_t length, void* callContext)
{
    TEST_VALUE_DATA_INSTANCE* testData = (TEST_VALUE_DATA_INSTANCE*)callContext;
    if (testData != NULL)
    {
        if (memcmp(testData->dataTest, data, length) == 0)
        {
            g_callbackInvoked = true;
        }
    }
    return g_test_Io_Send_Result;
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

/* connect_packet_create */
/* Tests_SRS_CONTROL_PACKET_07_001: [If the parameters ioSendFn is NULL or mqttOptions is NULL then ctrlpacket_connect shall return a non-zero value.] */
/* Tests_SRS_CONTROL_PACKET_07_007: [ctrlpacket_connect shall return a non-zero value on any failure.] */
/* Tests_SRS_CONTROL_PACKET_07_005: [ctrlpacket_connect shall call clean all data that has been allocated.] */
TEST_FUNCTION(ctrlpacket_connect_CTRL_PACKET_IO_SEND_NULL_fail)
{
    // arrange
    control_packet_mocks mocks;
    MQTTCLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, "testuser", "testpassword", 20, false, true, DELIVER_AT_MOST_ONCE);

    // act
    int result = ctrlpacket_connect(NULL, &mqttOptions, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_001: [If the parameters ioSendFn is NULL or mqttOptions is NULL then ctrlpacket_connect shall return a non-zero value.] */
/* Tests_SRS_CONTROL_PACKET_07_007: [ctrlpacket_connect shall return a non-zero value on any failure.] */
TEST_FUNCTION(ctrlpacket_connect_MQTTCLIENT_OPTIONS_NULL_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_connect(TestIoSend, NULL, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_007: [ctrlpacket_connect shall return a non-zero value on any failure.] */
TEST_FUNCTION(ctrlpacket_connect_BUFFER_enlarge_fail)
{
    // arrange
    control_packet_mocks mocks;
    MQTTCLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, "testuser", "testpassword", 20, false, true, DELIVER_AT_MOST_ONCE);

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    int result = ctrlpacket_connect(TestIoSend, NULL, &mqttOptions);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_004: [ctrlpacket_connect shall call the ioSendFn callback and return a non-zero value on failure.] */
/* Tests_SRS_CONTROL_PACKET_07_007: [ctrlpacket_connect shall return a non-zero value on any failure.] */
/* Tests_SRS_CONTROL_PACKET_07_005: [ctrlpacket_connect shall call clean all data that has been allocated.] */
TEST_FUNCTION(ctrlpacket_connect_ioSendFn_fail)
{
    // arrange
    control_packet_mocks mocks;
    MQTTCLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, "testuser", "testpassword", 20, false, true, DELIVER_AT_MOST_ONCE);

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, byteutil_writeByte(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);

    g_test_Io_Send_Result = __LINE__;

    // act
    int result = ctrlpacket_connect(TestIoSend, NULL, &mqttOptions);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_002: [ctrlpacket_connect shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
/* Tests_SRS_CONTROL_PACKET_07_003: [ctrlpacket_connect shall use the function constructConnPayload to created the CONNECT payload and shall return a non-zero value on failure.] */
/* Tests_SRS_CONTROL_PACKET_07_006: [ctrlpacket_connect shall return zero on successfull completion.] */
/* Tests_SRS_CONTROL_PACKET_07_005: [ctrlpacket_connect shall call clean all data that has been allocated.] */
TEST_FUNCTION(ctrlpacket_connect_succeeds)
{
    // arrange
    control_packet_mocks mocks;
    MQTTCLIENT_OPTIONS mqttOptions = { 0 };
    SetupMqttLibOptions(&mqttOptions, TEST_CLIENT_ID, NULL, NULL, "testuser", "testpassword", 20, false, true, DELIVER_AT_MOST_ONCE);

    unsigned char CONNECT_VALUE[] = { 0x10, 0x38, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54, 0x04, 0xc2, 0x00, 0x14, 0x00, 0x14, 0x73, 0x69, \
        0x6e, 0x67, 0x6c, 0x65, 0x5f, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x65, 0x64, 0x5f, 0x74, 0x65, 0x73, 0x74, 0x00, 0x08, 0x74, \
        0x65, 0x73, 0x74, 0x75, 0x73, 0x65, 0x72, 0x00, 0x0c, 0x74, 0x65, 0x73, 0x74, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64 };
    TEST_VALUE_DATA_INSTANCE testData = { CONNECT_VALUE, sizeof(CONNECT_VALUE) / sizeof(CONNECT_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, byteutil_writeByte(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);

    // act
    int result = ctrlpacket_connect(TestIoSend, &testData, &mqttOptions);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_008: [If the parameters ioSendFn is NULL then ctrlpacket_disconnect shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_disconnect_ioSendFn_NULL_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_disconnect(NULL, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_010: [ctrlpacket_disconnect shall call the ioSendFn callback and return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_disconnect_ioSendFn_fails)
{
    // arrange
    control_packet_mocks mocks;

    g_test_Io_Send_Result = __LINE__;

    // act
    int result = ctrlpacket_disconnect(TestIoSend, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_009: [ctrlpacket_disconnect shall construct the MQTT DISCONNECT packet.] */
/* Tests_SRS_CONTROL_PACKET_07_011: [ctrlpacket_disconnect shall return a zero value on successful completion.] */
TEST_FUNCTION(ctrlpacket_disconnect_succeed)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char DISCONNECT_VALUE[] = { 0xE0, 0x00 };
    TEST_VALUE_DATA_INSTANCE testData = { DISCONNECT_VALUE, sizeof(DISCONNECT_VALUE) / sizeof(DISCONNECT_VALUE[0]) };

    // act
    int result = ctrlpacket_disconnect(TestIoSend, &testData);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_032: [If the parameters ioSendFn is NULL then ctrlpacket_ping shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_ping_ioSendFn_NULL_Fails)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_ping(NULL, (void*)TEST_CALL_CONTEXT);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_035: [ctrlpacket_ping shall call the ioSendFn callback and return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_ping_ioSendFn_returns_error_fails)
{
    // arrange
    control_packet_mocks mocks;

    g_test_Io_Send_Result = __LINE__;

    // act
    int result = ctrlpacket_ping(TestIoSend, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_033: [ctrlpacket_ping shall construct the MQTT PING packet.] */
/* Tests_SRS_CONTROL_PACKET_07_034: [ctrlpacket_ping shall return a zero value on successful completion.] */
TEST_FUNCTION(ctrlpacket_ping_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PING_VALUE[] = { 0xC0, 0x00 };
    TEST_VALUE_DATA_INSTANCE testData = { PING_VALUE, sizeof(PING_VALUE) / sizeof(PING_VALUE[0]) };

    // act
    int result = ctrlpacket_ping(TestIoSend, &testData);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_051: [If the parameters ioSendFn, topicName, or msgBuffer is NULL then ctrlpacket_publish shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_ioSendFn_NULL_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_publish(NULL, NULL, DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, (const BYTE*)TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_051: [If the parameters ioSendFn, topicName, or msgBuffer is NULL then ctrlpacket_publish shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_topicName_NULL_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_publish(TestIoSend, NULL, DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, NULL, (const BYTE*)TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_051: [If the parameters ioSendFn, topicName, or msgBuffer is NULL then ctrlpacket_publish shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_msgBuffer_NULL_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_publish(TestIoSend, NULL, DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, NULL, 0);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_052: [ctrlpacket_publish shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_publish_construct_BUFFER_enlarge_fail)
{
    // arrange
    control_packet_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));

    // act
    int result = ctrlpacket_publish(TestIoSend, NULL, DELIVER_AT_MOST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, (const BYTE*)TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_054: [ctrlpacket_publish shall return a non-zero value on any failure.] */
TEST_FUNCTION(ctrlpacket_publish_BUFFER_enlarge_fails)
{
    // arrange
    control_packet_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_publish(TestIoSend, NULL, DELIVER_AT_LEAST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, (const BYTE*)TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

TEST_FUNCTION(ctrlpacket_publish_constructFixedHeader_fails)
{
    // arrange
    control_packet_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    g_test_Io_Send_Result = __LINE__;

    // act
    int result = ctrlpacket_publish(TestIoSend, NULL, DELIVER_AT_LEAST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, (const BYTE*)TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_055: [ctrlpacket_publish shall call the ioSendFn callback and return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_publish_ioSendFn_fails)
{
    // arrange
    control_packet_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    g_test_Io_Send_Result = __LINE__;
    // act
    int result = ctrlpacket_publish(TestIoSend, NULL, DELIVER_AT_LEAST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, (const BYTE*)TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_056: [ctrlpacket_publish shall return zero on successful completion.] */
TEST_FUNCTION(ctrlpacket_publish_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_VALUE[] = { 0x30, 0x1f, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x81, 0x58, 0x00, 0x0f, 0x4d, 0x65, \
        0x73, 0x73, 0x61, 0x67, 0x65, 0x20, 0x74, 0x6f, 0x20, 0x73, 0x65, 0x6e, 0x64 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_VALUE, sizeof(PUBLISH_VALUE) / sizeof(PUBLISH_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_publish(TestIoSend, &testData, DELIVER_AT_LEAST_ONCE, true, false, TEST_PACKET_ID, TEST_TOPIC_NAME, (const BYTE*)TEST_MESSAGE, TEST_MESSAGE_LEN);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_012: [If the parameters ioSendFn is NULL then ctrlpacket_publishAck shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_ack_ioSendFn_fail)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x40, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_ACK_VALUE, sizeof(PUBLISH_ACK_VALUE) / sizeof(PUBLISH_ACK_VALUE[0]) };

    // act
    int result = ctrlpacket_publishAck(NULL, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Test_SRS_CONTROL_PACKET_07_016: [if constructPublishReply fails then ctrlpacket_publishAck shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_ack_pre_build_fail)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x40, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_ACK_VALUE, sizeof(PUBLISH_ACK_VALUE) / sizeof(PUBLISH_ACK_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    int result = ctrlpacket_publishAck(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Test_SRS_CONTROL_PACKET_07_014: [ctrlpacket_publishAck shall return a zero value on successful completion.] */
/* Test_SRS_CONTROL_PACKET_07_013: [ctrlpacket_publishAck shall use constructPublishReply to construct the MQTT PUBACK reply.] */
/* Tests_SRS_CONTROL_PACKET_07_015: [ctrlpacket_publishAck shall call the ioSendFn callback and return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_publish_ack_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x40, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_ACK_VALUE, sizeof(PUBLISH_ACK_VALUE) / sizeof(PUBLISH_ACK_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_publishAck(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_017: [If the parameters ioSendFn is NULL then ctrlpacket_publishRecieved shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_recieved_ioSendFn_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_publishRecieved(NULL, NULL, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_021: [if constructPublishReply fails then ctrlpacket_publishAck shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_recieved_pre_build_fail)
{
    // arrange
    control_packet_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    int result = ctrlpacket_publishRecieved(TestIoSend, NULL, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_020: [ctrlpacket_publishRecieved shall call the ioSendFn callback and return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_publish_recieved_ioSendFn_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x50, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_ACK_VALUE, sizeof(PUBLISH_ACK_VALUE) / sizeof(PUBLISH_ACK_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    g_test_Io_Send_Result = __LINE__;

    // act
    int result = ctrlpacket_publishRecieved(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_018: [ctrlpacket_publishRecieved shall use constructPublishReply to construct the MQTT PUBREC reply.] */
/* Tests_SRS_CONTROL_PACKET_07_019: [ctrlpacket_publishRecieved shall return a zero value on successful completion.] */
TEST_FUNCTION(ctrlpacket_publish_recieved_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x50, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_ACK_VALUE, sizeof(PUBLISH_ACK_VALUE) / sizeof(PUBLISH_ACK_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_publishRecieved(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_022: [If the parameters ioSendFn is NULL then ctrlpacket_publishRelease shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_release_ioSendFn_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_publishRelease(NULL, NULL, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_026: [if constructPublishReply fails then ctrlpacket_publishRelease shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_release_pre_build_fail)
{
    // arrange
    control_packet_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    int result = ctrlpacket_publishRelease(TestIoSend, NULL, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_025: [ctrlpacket_publishRelease shall call the ioSendFn callback and return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_publish_release_ioSendFn_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x62, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_ACK_VALUE, sizeof(PUBLISH_ACK_VALUE) / sizeof(PUBLISH_ACK_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    g_test_Io_Send_Result = __LINE__;

    // act
    int result = ctrlpacket_publishRelease(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_023: [ctrlpacket_publishRelease shall use constructPublishReply to construct the MQTT PUBREL reply.] */
/* Tests_SRS_CONTROL_PACKET_07_024 : [ctrlpacket_publishRelease shall return a zero value on successful completion.] */
/* Tests_SRS_CONTROL_PACKET_07_024: [ctrlpacket_publishRelease shall return a zero value on successful completion.] */
TEST_FUNCTION(ctrlpacket_publish_release_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_ACK_VALUE[] = { 0x62, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_ACK_VALUE, sizeof(PUBLISH_ACK_VALUE) / sizeof(PUBLISH_ACK_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_publishRelease(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_027: [If the parameters ioSendFn is NULL then ctrlpacket_publishComplete shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_complete_ioSendFn_NULL_fail)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_publishComplete(NULL, NULL, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_031: [if constructPublishReply fails then ctrlpacket_publishComplete shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_publish_complete_pre_build_fail)
{
    // arrange
    control_packet_mocks mocks;

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1)
        .SetReturn(__LINE__);

    // act
    int result = ctrlpacket_publishComplete(TestIoSend, NULL, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_030: [ctrlpacket_publishComplete shall call the ioSendFn callback and return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_publish_complete_ioSendFn_fail)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_COMP_VALUE[] = { 0x70, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_COMP_VALUE, sizeof(PUBLISH_COMP_VALUE) / sizeof(PUBLISH_COMP_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    g_test_Io_Send_Result = __LINE__;

    // act
    int result = ctrlpacket_publishComplete(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_028: [ctrlpacket_publishComplete shall use constructPublishReply to construct the MQTT PUBCOMP reply.] */
/* Tests_SRS_CONTROL_PACKET_07_029: [ctrlpacket_publishComplete shall return a zero value on successful completion.] */
TEST_FUNCTION(ctrlpacket_publish_complete_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char PUBLISH_COMP_VALUE[] = { 0x70, 0x02, 0x81, 0x58 };
    TEST_VALUE_DATA_INSTANCE testData = { PUBLISH_COMP_VALUE, sizeof(PUBLISH_COMP_VALUE) / sizeof(PUBLISH_COMP_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, 4))
        .IgnoreArgument(1);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_publishComplete(TestIoSend, &testData, TEST_PACKET_ID);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_045: [If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_Subscribe shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_subscribe_ioSendFn_NULL_fails)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_subscribe(NULL, NULL, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_045: [If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_Subscribe shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_subscribe_subscribeList_NULL_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char SUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };
    TEST_VALUE_DATA_INSTANCE testData = { SUBSCRIBE_VALUE, sizeof(SUBSCRIBE_VALUE) / sizeof(SUBSCRIBE_VALUE[0]) };

    // act
    int result = ctrlpacket_subscribe(TestIoSend, &testData, TEST_PACKET_ID, NULL, 0);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_046: [ctrlpacket_Subscribe shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_subscribe_BUFFER_enlarge_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char SUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };
    TEST_VALUE_DATA_INSTANCE testData = { SUBSCRIBE_VALUE, sizeof(SUBSCRIBE_VALUE) / sizeof(SUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    int result = ctrlpacket_subscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_047: [ctrlpacket_Subscribe shall use the function addListItemToSubscribePacket to create the MQTT SUBSCRIBE payload packet.] */
TEST_FUNCTION(ctrlpacket_subscribe_addListItemsToSubscribePacket_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char SUBSCRIBE_VALUE[] = { 0x80, 0x1a, 0x81, 0x58, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x01, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32, 0x02 };
    TEST_VALUE_DATA_INSTANCE testData = { SUBSCRIBE_VALUE, sizeof(SUBSCRIBE_VALUE) / sizeof(SUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    int result = ctrlpacket_subscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_048: [ctrlpacket_Subscribe shall use the function constructFixedHeader to create the MQTT SUBSCRIBE fixed packet.] */
TEST_FUNCTION(ctrlpacket_subscribe_constructFixedHeader_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char SUBSCRIBE_VALUE[] = { 0x80, 0x1a, 0x81, 0x58, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x01, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32, 0x02 };
    TEST_VALUE_DATA_INSTANCE testData = { SUBSCRIBE_VALUE, sizeof(SUBSCRIBE_VALUE) / sizeof(SUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_subscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_050: [ctrlpacket_Subscribe shall return a zero value on successful completion.] */
TEST_FUNCTION(ctrlpacket_subscribe_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char SUBSCRIBE_VALUE[] = { 0x80, 0x1a, 0x81, 0x58, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x01, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32, 0x02 };
    TEST_VALUE_DATA_INSTANCE testData = { SUBSCRIBE_VALUE, sizeof(SUBSCRIBE_VALUE) / sizeof(SUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(5);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_subscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_SUBSCRIBE_PAYLOAD, 2);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_038: [If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_unsubscribe shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_unsubscribe_ioSendFn_NULL_fails)
{
    // arrange
    control_packet_mocks mocks;

    // act
    int result = ctrlpacket_unsubscribe(NULL, NULL, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_038: [If the parameters ioSendFn is NULL, payloadList is NULL, or payloadCount is Zero then ctrlpacket_unsubscribe shall return a non-zero value.] */
TEST_FUNCTION(ctrlpacket_unsubscribe_subscribeList_NULL_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };
    TEST_VALUE_DATA_INSTANCE testData = { UNSUBSCRIBE_VALUE, sizeof(UNSUBSCRIBE_VALUE) / sizeof(UNSUBSCRIBE_VALUE[0]) };

    // act
    int result = ctrlpacket_unsubscribe(TestIoSend, &testData, TEST_PACKET_ID, NULL, 0);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_044: [ctrlpacket_unsubscribe shall return a non-zero value on any error encountered.] */
/* Tests_SRS_CONTROL_PACKET_07_039: [ctrlpacket_unsubscribe shall use the function constructVariableHeader to create the MQTT variable header and shall return a non-zero value on failure.] */
TEST_FUNCTION(ctrlpacket_unsubscribe_BUFFER_enlarge_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };
    TEST_VALUE_DATA_INSTANCE testData = { UNSUBSCRIBE_VALUE, sizeof(UNSUBSCRIBE_VALUE) / sizeof(UNSUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    int result = ctrlpacket_unsubscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_044: [ctrlpacket_unsubscribe shall return a non-zero value on any error encountered.] */
/* Tests_SRS_CONTROL_PACKET_07_040: [ctrlpacket_unsubscribe shall use the function addListItemToUnsubscribePacket to create the MQTT UNSUBSCRIBE payload packet.] */
TEST_FUNCTION(ctrlpacket_unsubscribe_addListItemToUnsubscribePacket_BUFFER_enlarge_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0x80, 0x0f, 0x81, 0x58, 0x00, 0x0a, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x4e, 0x61, 0x6d, 0x65, 0x00 };
    TEST_VALUE_DATA_INSTANCE testData = { UNSUBSCRIBE_VALUE, sizeof(UNSUBSCRIBE_VALUE) / sizeof(UNSUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new());
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG));

    // act
    int result = ctrlpacket_unsubscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_044: [ctrlpacket_unsubscribe shall return a non-zero value on any error encountered.] */
TEST_FUNCTION(ctrlpacket_unsubscribe_constructFixedHeader_fails)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0xa0, 0x18, 0x81, 0x58, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32 };
    TEST_VALUE_DATA_INSTANCE testData = { UNSUBSCRIBE_VALUE, sizeof(UNSUBSCRIBE_VALUE) / sizeof(UNSUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).SetReturn(__LINE__);

    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_unsubscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONTROL_PACKET_07_043: [ctrlpacket_unsubscribe shall return a zero value on successful completion.] */
TEST_FUNCTION(ctrlpacket_unsubscribe_succeeds)
{
    // arrange
    control_packet_mocks mocks;

    unsigned char UNSUBSCRIBE_VALUE[] = { 0xa0, 0x18, 0x81, 0x58, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x31, 0x00, 0x09, 0x73, 0x75, 0x62, 0x54, 0x6f, 0x70, 0x69, 0x63, 0x32 };
    TEST_VALUE_DATA_INSTANCE testData = { UNSUBSCRIBE_VALUE, sizeof(UNSUBSCRIBE_VALUE) / sizeof(UNSUBSCRIBE_VALUE[0]) };

    EXPECTED_CALL(mocks, BUFFER_new()).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_enlarge(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(3);
    EXPECTED_CALL(mocks, BUFFER_u_char(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(5);
    EXPECTED_CALL(mocks, BUFFER_length(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(4);
    EXPECTED_CALL(mocks, BUFFER_delete(IGNORED_PTR_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, BUFFER_pre_build(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    EXPECTED_CALL(mocks, BUFFER_prepend(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    EXPECTED_CALL(mocks, byteutil_writeUTF(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).ExpectedAtLeastTimes(2);
    EXPECTED_CALL(mocks, byteutil_writeInt(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    // act
    int result = ctrlpacket_unsubscribe(TestIoSend, &testData, TEST_PACKET_ID, TEST_UNSUBSCRIPTION_TOPIC, 2);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(g_callbackInvoked);
}

/* Tests_SRS_CONTROL_PACKET_07_036: [ctrlpacket_processControlPacketType shall retrieve the CONTROL_PACKET_TYPE from pktBytes variable.] */
/* Tests_SRS_CONTROL_PACKET_07_037: [if the parameter flags is Non_Null then ctrlpacket_processControlPacketType shall retrieve the flag value from the pktBytes variable.] */
TEST_FUNCTION(ctrlpacket_processControlPacketType_succeed)
{
    // arrange
    control_packet_mocks mocks;
    CONTROL_PACKET_TYPE packetType;

    int flags = 0;

    // act
    packetType = ctrlpacket_processControlPacketType(0x20, NULL);
    ASSERT_ARE_EQUAL(CONTROL_PACKET_TYPE, CONNACK_TYPE, packetType);

    packetType = ctrlpacket_processControlPacketType(0x40, NULL);
    ASSERT_ARE_EQUAL(CONTROL_PACKET_TYPE, PUBACK_TYPE, packetType);

    packetType = ctrlpacket_processControlPacketType(0x62, &flags);
    ASSERT_ARE_EQUAL(CONTROL_PACKET_TYPE, PUBREL_TYPE, packetType);
    ASSERT_ARE_EQUAL(int, 2, flags);

    packetType = ctrlpacket_processControlPacketType(0x70, NULL);
    ASSERT_ARE_EQUAL(CONTROL_PACKET_TYPE, PUBCOMP_TYPE, packetType);

    packetType = ctrlpacket_processControlPacketType(0x90, NULL);
    ASSERT_ARE_EQUAL(CONTROL_PACKET_TYPE, SUBACK_TYPE, packetType);

    packetType = ctrlpacket_processControlPacketType(0xB0, NULL);
    ASSERT_ARE_EQUAL(CONTROL_PACKET_TYPE, UNSUBACK_TYPE, packetType);

    packetType = ctrlpacket_processControlPacketType(0xD0, NULL);
    ASSERT_ARE_EQUAL(CONTROL_PACKET_TYPE, PINGRESP_TYPE, packetType);

    // assert
}

END_TEST_SUITE(control_packet_unittests)
