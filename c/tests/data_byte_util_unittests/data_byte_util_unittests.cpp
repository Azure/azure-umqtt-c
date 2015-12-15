// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "data_byte_util.h"

#define TEST_INTEGER_VALUE      45522
static uint8_t TEST_BYTE_ONE = (uint8_t)0xB1;
static uint8_t TEST_BYTE_TWO = (uint8_t)0xD2;

static const char* TEST_STRING_VALUE = "Test String Value";

TYPED_MOCK_CLASS(data_byte_util_mocks, CGlobalMock)
{
public:
};

extern "C"
{
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(data_byte_util_unittests)

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
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    if (!MicroMockReleaseMutex(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not release test serialization mutex.");
    }
}

TEST_FUNCTION(byteutil_readByte_success)
{
    // arrange
    data_byte_util_mocks mocks;

    uint8_t data[1] = { 0 };
    data[0] = 0x13;
    uint8_t* iterator = data;

    // act
    uint8_t result = byteutil_readByte(&iterator);

    // assert
    ASSERT_ARE_EQUAL(uint8_t, 0x13, result);
}

TEST_FUNCTION(byteutil_readByte_fail)
{
    // arrange
    data_byte_util_mocks mocks;

    // act
    uint8_t result = byteutil_readByte(NULL);

    // assert
    ASSERT_ARE_EQUAL(uint8_t, 0, result);
}

TEST_FUNCTION(byteutil_writeByte_success)
{
    // arrange
    data_byte_util_mocks mocks;

    uint8_t data[1] = { 0 };
    uint8_t* iterator = data;

    // act
    byteutil_writeByte(&iterator, 0x13);

    // assert
    ASSERT_ARE_EQUAL(uint8_t, 0x13, data[0]);
}

TEST_FUNCTION(byteutil_readInt_success)
{
    // arrange
    data_byte_util_mocks mocks;

    uint8_t data[2] = { TEST_BYTE_ONE, TEST_BYTE_TWO };
    uint8_t* iterator = data;

    // act
    int result = byteutil_readInt(&iterator);

    // assert
    ASSERT_ARE_EQUAL(int, TEST_INTEGER_VALUE, result);
}

TEST_FUNCTION(byteutil_readInt_fail)
{
    // arrange
    data_byte_util_mocks mocks;

    // act
    int result = byteutil_readInt(NULL);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
}

TEST_FUNCTION(byteutil_writeInt_success)
{
    // arrange
    data_byte_util_mocks mocks;

    uint8_t data[2] = { 0 };
    uint8_t* iterator = data;

    // act
    byteutil_writeInt(&iterator, TEST_INTEGER_VALUE);

    // assert
    ASSERT_ARE_EQUAL(uint8_t, data[0], TEST_BYTE_ONE);
    ASSERT_ARE_EQUAL(uint8_t, data[1], TEST_BYTE_TWO);
}

TEST_FUNCTION(byteutil_writeInt_fail)
{
    // arrange
    data_byte_util_mocks mocks;

    // act
    byteutil_writeInt(NULL, TEST_INTEGER_VALUE);

    // assert
}

TEST_FUNCTION(byteutil_readUTF_success)
{
    // arrange
    data_byte_util_mocks mocks;

    uint8_t data[] = { 0x00, 0x11, 0x54, 0x65, 0x73, 0x74, 0x20, 0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x20, 0x56, 0x61, 0x6c, 0x75, 0x65 };
    uint8_t* iterator = data;

    // act
    char* result = byteutil_readUTF(&iterator);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, result);
}

TEST_FUNCTION(byteutil_readUTF_zero_string_success)
{
    // arrange
    data_byte_util_mocks mocks;

    uint8_t data[19] = { 0 };
    uint8_t* iterator = data;

    // act
    char* result = byteutil_readUTF(&iterator);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(byteutil_readUTF_fail)
{
    // arrange
    data_byte_util_mocks mocks;

    // act
    char* result = byteutil_readUTF(NULL);

    // assert
    ASSERT_IS_NULL(result);
}

END_TEST_SUITE(data_byte_util_unittests)
