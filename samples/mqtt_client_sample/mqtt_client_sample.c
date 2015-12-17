// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "mqtt_client_sample.h"
#include "mqtt_client.h"
#include "socketio.h"

#include <stdarg.h>
#include <stdio.h>

#ifdef _CRT_DBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _CRT_DBG_MAP_ALLOC

static const char* HOSTNAME = "test.mosquitto.org";
static unsigned int sent_messages = 0;

#define PORT_NUM_UNENCRYPTED        1883
#define PORT_NUM_ENCRYPTED          8883
#define PORT_NUM_ENCRYPTED_CERT     8884

#define DEFAULT_MSG_TO_SEND         1

static void PrintLogFunction(unsigned int options, char* format, ...)
{
    va_list args;
    va_start(args, format);
    (void)vprintf(format, args);
    va_end(args);

    if (options & LOG_LINE)
    {
        (void)printf("\r\n");
    }
}

static void OnRecvCallback(MQTT_MESSAGE_HANDLE msgHandle, void* context)
{

}

static int OnOperationComplete(MQTT_CLIENT_ACTION_RESULT actionResult, void* context)
{
    return 0;
}

void mqtt_client_sample_run()
{
#ifdef _CRT_DBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif
}
