// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "data_byte_util.h"

BYTE byteutil_readByte(BYTE** buffer)
{
    BYTE value = **buffer;
    (*buffer)++;
    return value;
}

void byteutil_writeByte(char** buffer, char value)
{
    **buffer = value;
    (*buffer)++;
}

int byteutil_readInt(BYTE** buffer)
{
    BYTE* iterator = *buffer;
    int value = 256 * ((BYTE)(*iterator)) + (BYTE)(*(iterator + 1));
    *buffer += 2; // Move the ptr
    return value;
}

void byteutil_writeInt(char** buffer, int value)
{
    **buffer = (char)(value / 256);
    (*buffer)++;
    **buffer = (char)(value % 256);
    (*buffer)++;
}

char* byteutil_readUTF(BYTE** buffer)
{
    char* result = NULL;
    if (*buffer != NULL)
    {
        // Get the length of the string
        int len = byteutil_readInt(buffer);
        if (len > 0)
        {
            result = (char*)malloc(len+1);
            if (result != NULL)
            {
                memcpy(result, *buffer, len);
                result[len] = '\0';
                *buffer += len;
            }
        }
    }
    return result;
}

void byteutil_writeUTF(char** buffer, const char* stringData, size_t len)
{
    byteutil_writeInt(buffer, len);
    memcpy(*buffer, stringData, len);
    *buffer += len;
}
