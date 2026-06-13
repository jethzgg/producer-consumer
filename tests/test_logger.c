#include "common.h"
#include "logger.h"
#include "test_support.h"

void test_logger(void) {
    ASSERT_EQ_LONG(APP_ERROR, logger_init((LogMode)99));
    ASSERT_EQ_LONG(APP_SUCCESS, logger_init(LOG_MODE_SUMMARY));
    logger_event("Test", "hidden event");
    logger_destroy();

    ASSERT_EQ_LONG(APP_SUCCESS, logger_init(LOG_MODE_DETAILED));
    logger_event(NULL, "ignored");
    logger_event("Test", NULL);
    logger_event("Test", "visible event %d", 1);
    logger_destroy();
    logger_event("Test", "ignored after destroy");
    logger_destroy();
}
