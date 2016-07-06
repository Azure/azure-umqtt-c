// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"

#ifdef __cplusplus
extern "C" {
#endif

    void* my_gballoc_malloc(size_t size)
    {
        return malloc(size);
    }

    void my_gballoc_free(void* ptr)
    {
        free(ptr);
    }

    int my_mallocAndStrcpy_s(char** destination, const char* source)
    {
        size_t len = strlen(source);
        *destination = (char*)malloc(len + 1);
        strcpy(*destination, source);
        return 0;
    }

#ifdef __cplusplus
}
#endif

#define ENABLE_MOCKS

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"

#undef ENABLE_MOCKS

#include "azure_umqtt_c/mqtt_message.h"

static bool g_fail_alloc_calls;

static const char* TEST_SUBSCRIPTION_TOPIC = "subTopic";
static const uint8_t TEST_PACKET_ID = (uint8_t)0x12;
static const char* TEST_TOPIC_NAME = "topic Name";
static const uint8_t* TEST_MESSAGE = (const uint8_t*)"Message to send";
static const int TEST_MSG_LEN = sizeof(TEST_MESSAGE)/sizeof(TEST_MESSAGE[0]);

typedef struct TEST_COMPLETE_DATA_INSTANCE_TAG
{
    unsigned char* dataHeader;
    size_t Length;
} TEST_COMPLETE_DATA_INSTANCE;

TEST_DEFINE_ENUM_TYPE(QOS_VALUE, QOS_VALUE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(QOS_VALUE, QOS_VALUE_VALUES);

TEST_MUTEX_HANDLE test_serialize_mutex;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(mqtt_message_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    g_fail_alloc_calls = false;
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/* Test_SRS_MQTTMESSAGE_07_001:[If the parameters topicName is NULL then mqttmessage_createMessage shall return NULL.] */
TEST_FUNCTION(mqttmessage_create_Topicname_NULL_fail)
{
    // arrange

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, NULL, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);

    // assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Test_SRS_MQTTMESSAGE_07_001:[If the parameters topicName is NULL then mqttmessage_create shall return NULL.] */
TEST_FUNCTION(mqttmessage_create_appMsgLength_NULL_succeed)
{
    // arrange
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TOPIC_NAME))
        .IgnoreArgument(1);

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, NULL, 0);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

/* Test_SRS_MQTTMESSAGE_07_002: [mqttmessage_create shall allocate and copy the topicName and appMsg parameters.]*/
/* Test_SRS_MQTTMESSAGE_07_004: [If mqttmessage_create succeeds the it shall return a NON-NULL MQTT_MESSAGE_HANDLE value.] */
TEST_FUNCTION(mqttmessage_create_succeed)
{
    // arrange
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TOPIC_NAME))
        .IgnoreArgument(1);
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    // act
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);

    // assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

/* Test_SRS_MQTTMESSAGE_07_006: [mqttmessage_destroyMessage shall free all resources associated with the MQTT_MESSAGE_HANDLE value] */
TEST_FUNCTION(mqttmessage_destroy_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    // act
    mqttmessage_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Test_SRS_MQTTMESSAGE_07_005: [If the handle parameter is NULL then mqttmessage_destroyMessage shall do nothing] */
TEST_FUNCTION(mqttmessage_destroy_handle_NULL_fail)
{
    // arrange

    // act
    mqttmessage_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Test_SRS_MQTTMESSAGE_07_008: [mqttmessage_clone shall create a new MQTT_MESSAGE_HANDLE with data content identical of the handle value.] */
TEST_FUNCTION(mqttmessage_clone_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_TOPIC_NAME));
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    // act
    MQTT_MESSAGE_HANDLE cloneHandle = mqttmessage_clone(handle);

    // assert
    ASSERT_IS_NOT_NULL(cloneHandle);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
    mqttmessage_destroy(cloneHandle);
}

/* Test_SRS_MQTTMESSAGE_07_007: [If handle parameter is NULL then mqttmessage_clone shall return NULL.] */
TEST_FUNCTION(mqttmessage_clone_handle_fails)
{
    // arrange

    // act
    MQTT_MESSAGE_HANDLE cloneHandle = mqttmessage_clone(NULL);

    // assert
    ASSERT_IS_NULL(cloneHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Test_SRS_MQTTMESSAGE_07_010: [If handle is NULL then mqttmessage_getPacketId shall return 0.] */
TEST_FUNCTION(mqttmessage_getPacketId_handle_fails)
{
    // arrange

    // act
    uint16_t packetId = mqttmessage_getPacketId(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, packetId);
}

/* Test_SRS_MQTTMESSAGE_07_011: [mqttmessage_getPacketId shall return the packetId value contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_getPacketId_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    // act
    uint16_t packetId = mqttmessage_getPacketId(handle);

    // assert
    ASSERT_ARE_EQUAL(int, TEST_PACKET_ID, packetId);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

/* Test_SRS_MQTTMESSAGE_07_012: [If handle is NULL then mqttmessage_getTopicName shall return a NULL string.] */
TEST_FUNCTION(mqttmessage_getTopicName_handle_fails)
{
    // arrange

    // act
    const char* topicName = mqttmessage_getTopicName(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(topicName);
}

/* Test_SRS_MQTTMESSAGE_07_013: [mqttmessage_getTopicName shall return the topicName contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_getTopicName_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_MOST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    // act
    const char* topicName = mqttmessage_getTopicName(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_TOPIC_NAME, topicName);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

/* Test_SRS_MQTTMESSAGE_07_014: [If handle is NULL then mqttmessage_getQosType shall return the default DELIVER_AT_MOST_ONCE value.] */
TEST_FUNCTION(mqttmessage_getQosType_handle_fails)
{
    // arrange

    // act
    QOS_VALUE value = mqttmessage_getQosType(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(QOS_VALUE, DELIVER_AT_MOST_ONCE, value);
}

/* Test_SRS_MQTTMESSAGE_07_015: [mqttmessage_getQosType shall return the QOS Type value contained in MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_getQosType_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    // act
    QOS_VALUE value = mqttmessage_getQosType(handle);

    // assert
    ASSERT_ARE_EQUAL(QOS_VALUE, DELIVER_AT_LEAST_ONCE, value);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

/* Test_SRS_MQTTMESSAGE_07_016: [If handle is NULL then mqttmessage_getIsDuplicateMsg shall return false.] */
TEST_FUNCTION(mqttmessage_getIsDuplicateMsg_handle_fails)
{
    // arrange

    // act
    bool value = mqttmessage_getIsDuplicateMsg(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(value);
}

/* Tests_SRS_MQTTMESSAGE_07_022: [If handle is NULL then mqttmessage_setIsDuplicateMsg shall return a non-zero value.] */
TEST_FUNCTION(mqttmessage_setIsDuplicateMsg_handle_fails)
{
    // arrange

    // act
    int value = mqttmessage_setIsDuplicateMsg(NULL, false);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, value);
}

/* Test_SRS_MQTTMESSAGE_07_017: [mqttmessage_getIsDuplicateMsg shall return the isDuplicateMsg value contained in MQTT_MESSAGE_HANDLE handle.] */
/* Test_SRS_MQTTMESSAGE_07_023: [mqttmessage_setIsDuplicateMsg shall store the duplicateMsg value in the MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_set_and_get_IsDuplicateMsg_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    // act
    int value = mqttmessage_setIsDuplicateMsg(handle, true);

    bool dupMsg = mqttmessage_getIsDuplicateMsg(handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, value);
    ASSERT_IS_TRUE(dupMsg);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

/* Test_SRS_MQTTMESSAGE_07_018: [If handle is NULL then mqttmessage_getIsRetained shall return false.] */
TEST_FUNCTION(mqttmessage_getIsRetained_handle_fails)
{
    // arrange

    // act
    bool value = mqttmessage_getIsRetained(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(value);
}

/* Tests_SRS_MQTTMESSAGE_07_024: [If handle is NULL then mqttmessage_setIsRetained shall return a non-zero value.] */
TEST_FUNCTION(mqttmessage_setIsRetained_handle_fails)
{
    // arrange

    // act
    int value = mqttmessage_setIsRetained(NULL, false);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, value);
}

/* Test_SRS_MQTTMESSAGE_07_019: [mqttmessage_getIsRetained shall return the isRetained value contained in MQTT_MESSAGE_HANDLE handle.] */
/* Test_SRS_MQTTMESSAGE_07_025: [mqttmessage_setIsRetained shall store the retainMsg value in the MQTT_MESSAGE_HANDLE handle.] */
TEST_FUNCTION(mqttmessage_set_and_get_IsRetained_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    // act
    int value = mqttmessage_setIsRetained(handle, true);

    bool retainMsg = mqttmessage_getIsRetained(handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, value);
    ASSERT_IS_TRUE(retainMsg);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

/* Test_SRS_MQTTMESSAGE_07_020: [If handle is NULL or if msgLen is 0 then mqttmessage_applicationMsg shall return NULL.] */
TEST_FUNCTION(mqttmessage_getApplicationMsg_handle_fails)
{
    // arrange

    // act
    const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(payload);
}

/* Test_SRS_MQTTMESSAGE_07_021: [mqttmessage_getApplicationMsg shall return the applicationMsg value contained in MQTT_MESSAGE_HANDLE handle and the length of the appMsg in the msgLen parameter.] */
TEST_FUNCTION(mqttmessage_getApplicationMsg_succeed)
{
    // arrange
    MQTT_MESSAGE_HANDLE handle = mqttmessage_create(TEST_PACKET_ID, TEST_TOPIC_NAME, DELIVER_AT_LEAST_ONCE, TEST_MESSAGE, TEST_MSG_LEN);
    umock_c_reset_all_calls();

    // act
    const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(handle);

    // assert
    ASSERT_IS_NOT_NULL(payload);
    ASSERT_ARE_EQUAL(int, 0, memcmp(payload->message, TEST_MESSAGE, TEST_MSG_LEN) );
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    mqttmessage_destroy(handle);
}

END_TEST_SUITE(mqtt_message_unittests)
