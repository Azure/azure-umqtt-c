// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "mqtt_message.h"
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
};
static bool g_fail_alloc_calls;

static const char* TEST_SUBSCRIPTION_TOPIC = "subTopic";
static const BYTE TEST_PACKET_ID = (BYTE)0x1234;
static const char* TEST_TOPIC_NAME = "topic Name";
static const unsigned char* TEST_MESSAGE = (const unsigned char*)"Message to send";
static const int TEST_MSG_LEN = sizeof(TEST_MESSAGE)/sizeof(TEST_MESSAGE[0]);

typedef struct TEST_COMPLETE_DATA_INSTANCE_TAG
{
    unsigned char* dataHeader;
    size_t Length;
} TEST_COMPLETE_DATA_INSTANCE;

#ifdef CPP_UNITTEST
template <> static std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString < QOS_VALUE >(const QOS_VALUE & qosValue)
{
    std::wstring result;
    switch (qosValue)
    {
        case DELIVER_AT_LEAST_ONCE: result = L"Deliver_At_Least_Once";
            break;
        case DELIVER_EXACTLY_ONCE: result = L"Deliver_Exactly_Once";
            break;
        case DELIVER_AT_MOST_ONCE: result = L"Deliver_At_Most_Once";
            break;
        default:
        case DELIVER_FAILURE: result = L"Deliver_Failure";
            break;
    }
    return result;
}

/*template <> static std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString < PACKET_ID >(const PACKET_ID & qosValue)
{
    std::wstring result;
    switch (qosValue)
    {
    case DELIVER_AT_LEAST_ONCE: result = L"Deliver_At_Least_Once";
        break;
    case DELIVER_EXACTLY_ONCE: result = L"Deliver_Exactly_Once";
        break;
    case DELIVER_AT_MOST_ONCE: result = L"Deliver_At_Most_Once";
        break;
    default:
    case DELIVER_FAILURE: result = L"Deliver_Failure";
        break;
    }
    return result;
}*/
#endif

static int PACKET_ID_Compare(PACKET_ID left, PACKET_ID right)
{
    return left != right;
}

static void PACKET_ID_ToString(char* string, size_t bufferSize, PACKET_ID val)
{
    sprintf_s(string, bufferSize, "%d", val);
}

static int BYTE_Compare(BYTE left, BYTE right)
{
    return left != right;
}

static void BYTE_ToString(char* string, size_t bufferSize, BYTE val)
{
    sprintf_s(string, bufferSize, "%d", val);
}

static int QOS_VALUE_Compare(QOS_VALUE left, QOS_VALUE right)
{
    return left != right;
}

static void QOS_VALUE_ToString(char* string, size_t bufferSize, QOS_VALUE val)
{
    switch (val)
    {
        case DELIVER_AT_LEAST_ONCE:
            strcpy_s(string, bufferSize, "Deliver_At_Least_Once");
            break;
        case DELIVER_EXACTLY_ONCE:
            strcpy_s(string, bufferSize, "Deliver_Exactly_Once");
            break;
        case DELIVER_AT_MOST_ONCE:
            strcpy_s(string, bufferSize, "Deliver_At_Most_Once");
            break;
        default:
        case DELIVER_FAILURE:
            strcpy_s(string, bufferSize, "Deliver_Failure");
            break;
    }
}

TYPED_MOCK_CLASS(mqtt_message_mocks, CGlobalMock)
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

    MOCK_STATIC_METHOD_2(, int, mallocAndStrcpy_s, char**, destination, const char*, source)
        size_t len = strlen(source);
        *destination = (char*)BASEIMPLEMENTATION::gballoc_malloc(len+1);
        strcpy(*destination, source);
    MOCK_METHOD_END(int, 0);
};

extern "C"
{
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_message_mocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_message_mocks, , void, gballoc_free, void*, ptr);

    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_message_mocks, , int, mallocAndStrcpy_s, char**, destination, const char*, source);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(mqtt_message_unittests)

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
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    if (!MicroMockReleaseMutex(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not release test serialization mutex.");
    }
}

/* Test_SRS_MQTTMESSAGE_07_001:[If the parameters topicName is NULL, appMsg is NULL, or appMsgLength is zero then mqttmessage_create shall return NULL.] */
TEST_FUNCTION(mqttmessage_create_appMsgLength_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, 0);

    // assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();
}

/* Test_SRS_MQTTMESSAGE_07_001:[If the parameters topicName is NULL, appMsg is NULL, or appMsgLength is zero then mqttmessage_create shall return NULL.] */
TEST_FUNCTION(mqttmessage_create_applicationMsg_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, NULL, 0);

    // assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();
}

/* Test_SRS_MQTTMESSAGE_07_001:[If the parameters topicName is NULL, appMsg is NULL, or appMsgLength is zero then mqttmessage_create shall return NULL.] */
TEST_FUNCTION(mqttmessage_create_Topicname_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, NULL, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);

    // assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();
}

/* Test_SRS_MQTTMESSAGE_07_002: [mqttmessage_create shall allocate and copy the topicName and appMsg parameters.]*/
/* Test_SRS_MQTTMESSAGE_07_004: [If mqttmessage_create succeeds the it shall return a NON-NULL MQTT_MESSAGE_HANDLE value.] */
TEST_FUNCTION(mqttmessage_create_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
}

/* Test_SRS_MQTTMESSAGE_07_006: [mqttmessage_destroyMessage shall free all resources associated with the MQTT_MESSAGE_HANDLE value] */
TEST_FUNCTION(mqttmessage_destroyMessage_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

    // act
    mqttmessage_destroyMessage(handle);

    // assert
    mocks.AssertActualAndExpectedCalls();
}

/* Test_SRS_MQTTMESSAGE_07_005: [If the handle parameter is NULL then mqttmessage_destroyMessage shall do nothing] */
TEST_FUNCTION(mqttmessage_destroyMessage_handle_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    mqttmessage_destroyMessage(NULL);

    // assert
}

/* Test_SRS_MQTTMESSAGE_07_008: [mqttmessage_clone shall create a new MQTT_MESSAGE_HANDLE with data content identical of the handle value.] */
TEST_FUNCTION(mqttmessage_clone_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    MQTT_MESSAGE_HANDLE cloneHandle = mqttmessage_clone(handle);

    // assert
    ASSERT_IS_NOT_NULL(cloneHandle);

    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
    mqttmessage_destroyMessage(cloneHandle);
}

/* Test_SRS_MQTTMESSAGE_07_007: [If handle parameter is NULL then mqttmessage_clone shall return NULL.] */
TEST_FUNCTION(mqttmessage_clone_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    MQTT_MESSAGE_HANDLE cloneHandle = mqttmessage_clone(NULL);

    // assert
    ASSERT_IS_NULL(cloneHandle);

    mocks.AssertActualAndExpectedCalls();
}

/* Test_SRS_MQTTMESSAGE_07_010: [If handle is NULL then mqttmessage_getPacketId shall return 0.] */
TEST_FUNCTION(mqttmessage_getPacketId_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    PACKET_ID packetId = mqttmessage_getPacketId(NULL);

    // assert
    ASSERT_ARE_EQUAL(int, 0, packetId);
}

/* Test_SRS_MQTTMESSAGE_07_011: [mqttmessage_getPacketId shall return the packetId value contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_getPacketId_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    // act
    PACKET_ID packetId = mqttmessage_getPacketId(handle);

    // assert
    ASSERT_ARE_EQUAL(int, TEST_PACKET_ID, packetId);

    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
}

/* Test_SRS_MQTTMESSAGE_07_012: [If handle is NULL then mqttmessage_getTopicName shall return a NULL string.] */
TEST_FUNCTION(mqttmessage_getTopicName_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    const char* topicName = mqttmessage_getTopicName(NULL);

    // assert
    ASSERT_IS_NULL(topicName);
}

/* Test_SRS_MQTTMESSAGE_07_013: [mqttmessage_getTopicName shall return the topicName contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_getTopicName_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    // act
    const char* topicName = mqttmessage_getTopicName(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_TOPIC_NAME, topicName);

    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
}

/* Test_SRS_MQTTMESSAGE_07_014: [If handle is NULL then mqttmessage_getQosType shall return the default DELIVER_AT_MOST_ONCE value.] */
TEST_FUNCTION(mqttmessage_getQosType_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    QOS_VALUE value = mqttmessage_getQosType(NULL);

    // assert
    ASSERT_ARE_EQUAL(QOS_VALUE, DELIVER_AT_MOST_ONCE, value);
}

/* Test_SRS_MQTTMESSAGE_07_015: [mqttmessage_getQosType shall return the QOS Type value contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_getQosType_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    // act
    QOS_VALUE value = mqttmessage_getQosType(handle);

    // assert
    ASSERT_ARE_EQUAL(QOS_VALUE, DELIVER_AT_LEAST_ONCE, value);

    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
}

/* Test_SRS_MQTTMESSAGE_07_016: [If handle is NULL then mqttmessage_getIsDuplicateMsg shall return false.] */
TEST_FUNCTION(mqttmessage_getIsDuplicateMsg_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    bool value = mqttmessage_getIsDuplicateMsg(NULL);

    // assert
    ASSERT_IS_FALSE(value);
}

TEST_FUNCTION(mqttmessage_setIsDuplicateMsg_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    int value = mqttmessage_setIsDuplicateMsg(NULL, false);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, value);
}

/* Test_SRS_MQTTMESSAGE_07_017: [mqttmessage_getIsDuplicateMsg shall return the isDuplicateMsg value contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_set_and_get_IsDuplicateMsg_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    // act
    int value = mqttmessage_setIsDuplicateMsg(handle, true);

    bool dupMsg = mqttmessage_getIsDuplicateMsg(handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, value);
    ASSERT_IS_TRUE(dupMsg);

    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
}

/* Test_SRS_MQTTMESSAGE_07_018: [If handle is NULL then mqttmessage_getIsRetained shall return false.] */
TEST_FUNCTION(mqttmessage_getIsRetained_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    bool value = mqttmessage_getIsRetained(NULL);

    // assert
    ASSERT_IS_FALSE(value);
}

TEST_FUNCTION(mqttmessage_setIsRetained_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    int value = mqttmessage_setIsRetained(NULL, false);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, value);
}

/* Test_SRS_MQTTMESSAGE_07_019: [mqttmessage_getIsRetained shall return the isRetained value contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_set_and_get_IsRetained_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    // act
    int value = mqttmessage_setIsRetained(handle, true);

    bool retainMsg = mqttmessage_getIsRetained(handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, value);
    ASSERT_IS_TRUE(retainMsg);

    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
}

/* Test_SRS_MQTTMESSAGE_07_020: [If handle is NULL or if msgLen is 0 then mqttmessage_applicationMsg shall return NULL.] */
TEST_FUNCTION(mqttmessage_getApplicationMsg_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(NULL);

    // assert
    ASSERT_IS_NULL(payload);
}

/* Test_SRS_MQTTMESSAGE_07_021: [mqttmessage_getApplicationMsg shall return the applicationMsg value contained in MQTT_MESSAGE_HANDLE handle and the length of the appMsg in the msgLen parameter.] */
TEST_FUNCTION(mqttmessage_getApplicationMsg_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    mocks.ResetAllCalls();

    // act
    const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(handle);

    // assert
    ASSERT_IS_NOT_NULL(payload);
    ASSERT_ARE_EQUAL(int, 0, memcmp(payload->message, TEST_MESSAGE, TEST_MSG_LEN) );

    mocks.AssertActualAndExpectedCalls();

    mqttmessage_destroyMessage(handle);
}

END_TEST_SUITE(mqtt_message_unittests)
