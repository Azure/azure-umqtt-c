// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "data_byte_util.h"

uint8_t byteutil_readByte(uint8_t** buffer)
{
    uint8_t result = 0;
    if (buffer != NULL)
    {
        result = **buffer;
        (*buffer)++;
    }
    return result;
}

void byteutil_writeByte(uint8_t** buffer, uint8_t value)
{
    if (buffer != NULL)
    {
        **buffer = value;
        (*buffer)++;
    }
}

uint16_t byteutil_readInt(uint8_t** buffer)
{
    uint16_t result = 0;
    if (buffer != NULL)
    {
        uint8_t* iterator = *buffer;
        result = 256 * ((uint8_t)(*iterator)) + (uint8_t)(*(iterator + 1));
        *buffer += 2; // Move the ptr
    }
    return result;
}

void byteutil_writeInt(uint8_t** buffer, uint16_t value)
{
    if (buffer != NULL)
    {
        **buffer = (char)(value / 256);
        (*buffer)++;
        **buffer = (char)(value % 256);
        (*buffer)++;
    }
}

char* byteutil_readUTF(uint8_t** buffer)
{
    char* result = NULL;
    if (buffer != NULL)
    {
        // Get the length of the string
        int len = byteutil_readInt(buffer);
        if (len > 0)
        {
            result = (char*)malloc(len+1);
            if (result != NULL)
            {
                (void)memcpy(result, *buffer, len);
                result[len] = '\0';
                *buffer += len;
            }
        }
    }
    return result;
}

void byteutil_writeUTF(uint8_t** buffer, const char* stringData, uint16_t len)
{
    if (buffer != NULL)
    {
        byteutil_writeInt(buffer, len);
        (void)memcpy(*buffer, stringData, len);
        *buffer += len;
    }
}
