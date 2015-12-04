// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DATA_BYTE_UTIL_H
#define DATA_BYTE_UTIL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mqttconst.h"

extern BYTE byteutil_readByte(BYTE** buffer);
extern int byteutil_readInt(BYTE** buffer);
extern char* byteutil_readUTF(BYTE** buffer);

extern void byteutil_writeByte(char** buffer, char value);
extern void byteutil_writeInt(char** buffer, int value);
extern void byteutil_writeUTF(char** buffer, const char* stringData, size_t len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DATA_BYTE_UTIL_H
