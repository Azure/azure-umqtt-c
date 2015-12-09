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

static int BYTE_Compare(BYTE left, BYTE right)
{
    return left != right;
}

static void BYTE_ToString(char* string, size_t bufferSize, BYTE val)
{
    sprintf_s(string, bufferSize, "%d", val);
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

    //MOCK_STATIC_METHOD_2(, int, BUFFER_pre_build, BUFFER_HANDLE, handle, size_t, size)
    //MOCK_METHOD_END(int, BASEIMPLEMENTATION::BUFFER_pre_build(handle, size));

    //MOCK_STATIC_METHOD_2(, int, BUFFER_prepend, BUFFER_HANDLE, handle1, BUFFER_HANDLE, handle2)
    //MOCK_METHOD_END(int, BASEIMPLEMENTATION::BUFFER_prepend(handle1, handle2));

    //MOCK_STATIC_METHOD_1(, void, BUFFER_delete, BUFFER_HANDLE, s)
    //    BASEIMPLEMENTATION::BUFFER_delete(s);
    //MOCK_VOID_METHOD_END();

    //MOCK_STATIC_METHOD_1(, unsigned char*, BUFFER_u_char, BUFFER_HANDLE, s)
    //MOCK_METHOD_END(unsigned char*, BASEIMPLEMENTATION::BUFFER_u_char(s) );

    //MOCK_STATIC_METHOD_1(, size_t, BUFFER_length, BUFFER_HANDLE, s)
    //MOCK_METHOD_END(size_t, BASEIMPLEMENTATION::BUFFER_length(s));
};

extern "C"
{
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_message_mocks, , void*, gballoc_malloc, size_t, size);
    DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_message_mocks, , void, gballoc_free, void*, ptr);

    DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_message_mocks, , int, mallocAndStrcpy_s, char**, destination, const char*, source);

    //DECLARE_GLOBAL_MOCK_METHOD_0(mqtt_message_mocks, , BUFFER_HANDLE, BUFFER_new);
    //DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_message_mocks, , void, BUFFER_delete, BUFFER_HANDLE, handle);
    //DECLARE_GLOBAL_MOCK_METHOD_3(mqtt_message_mocks, , int, BUFFER_build, BUFFER_HANDLE, handle, const unsigned char*, source, size_t, size);
    //DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_message_mocks, , int, BUFFER_pre_build, BUFFER_HANDLE, handle, size_t, size);
    //DECLARE_GLOBAL_MOCK_METHOD_2(mqtt_message_mocks, , int, BUFFER_prepend, BUFFER_HANDLE, handle1, BUFFER_HANDLE, handle2);
    //DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_message_mocks, , unsigned char*, BUFFER_u_char, BUFFER_HANDLE, handle);
    //DECLARE_GLOBAL_MOCK_METHOD_1(mqtt_message_mocks, , size_t, BUFFER_length, BUFFER_HANDLE, handle);
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

// TODO: Mallocs fail

TEST_FUNCTION(mqttmsg_createMessage_appLength_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmsg_createMessage(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, 0, true, true);

    // assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();
}

TEST_FUNCTION(mqttmsg_createMessage_applicationMsg_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmsg_createMessage(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, NULL, 0, true, true);

    // assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();
}

TEST_FUNCTION(mqttmsg_createMessage_Topicname_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmsg_createMessage(TEST_PACKET_ID, NULL, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN, true, true);

    // assert
    ASSERT_IS_NULL(handle);
    mocks.AssertActualAndExpectedCalls();
}

TEST_FUNCTION(mqttmsg_createMessage_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmsg_createMessage(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN, true, true);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    mocks.AssertActualAndExpectedCalls();

    mqttmsg_destroyMessage(handle);
}

TEST_FUNCTION(mqttmsg_destroyMessage_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmsg_createMessage(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN, true, true);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

    // act
    mqttmsg_destroyMessage(handle);

    // assert
    mocks.AssertActualAndExpectedCalls();
}

TEST_FUNCTION(mqttmsg_destroyMessage_handle_NULL_fail)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    mqttmsg_destroyMessage(NULL);

    // assert
}

TEST_FUNCTION(mqttmsg_clone_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmsg_createMessage(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN, true, true);
    mocks.ResetAllCalls();

    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_PTR_ARG));
    EXPECTED_CALL(mocks, mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // act
    MQTT_MESSAGE_HANDLE cloneHandle = mqttmsg_clone(handle);

    // assert
    ASSERT_IS_NOT_NULL(cloneHandle);

    mocks.AssertActualAndExpectedCalls();

    mqttmsg_destroyMessage(handle);
    mqttmsg_destroyMessage(cloneHandle);
}

TEST_FUNCTION(mqttmsg_clone_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    mocks.ResetAllCalls();

    // act
    MQTT_MESSAGE_HANDLE cloneHandle = mqttmsg_clone(NULL);

    // assert
    ASSERT_IS_NULL(cloneHandle);

    mocks.AssertActualAndExpectedCalls();
}

TEST_FUNCTION(mqttmsg_getPacketId_handle_fails)
{
    // arrange
    mqtt_message_mocks mocks;

    // act
    BYTE packetId = mqttmsg_getPacketId(NULL);

    // assert
    ASSERT_ARE_EQUAL(BYTE, 0, packetId);
}

TEST_FUNCTION(mqttmsg_getPacketId_succeed)
{
    // arrange
    mqtt_message_mocks mocks;

    MQTT_MESSAGE_HANDLE handle = mqttmsg_createMessage(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN, true, true);
    mocks.ResetAllCalls();

    // act
    BYTE packetId = mqttmsg_getPacketId(handle);

    // assert
    ASSERT_ARE_EQUAL(BYTE, TEST_PACKET_ID, packetId);

    mocks.AssertActualAndExpectedCalls();

    mqttmsg_destroyMessage(handle);
}

END_TEST_SUITE(mqtt_message_unittests)
