// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DATA_BYTE_UTIL_H
#define DATA_BYTE_UTIL_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

#include "mqttconst.h"

extern uint8_t byteutil_readByte(uint8_t** buffer);
extern uint16_t byteutil_readInt(uint8_t** buffer);
extern char* byteutil_readUTF(uint8_t** buffer);

extern void byteutil_writeByte(uint8_t** buffer, uint8_t value);
extern void byteutil_writeInt(uint8_t** buffer, uint16_t value);
extern void byteutil_writeUTF(uint8_t** buffer, const char* stringData, uint16_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DATA_BYTE_UTIL_H
