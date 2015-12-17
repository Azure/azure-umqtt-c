#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(mqtt_client_unittests, failedTestCount);
    return failedTestCount;
}
