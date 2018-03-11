// Print assertion messages over Segger RTT in debug builds

#if DEBUG == 1 && !defined(DISABLE_LOGGING_IN_FILE)

#ifdef __cplusplus
extern "C" {
#endif

#include "SEGGER_RTT.h"

#ifdef __cplusplus
}
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define log(msg)                                                               \
  SEGGER_RTT_WriteString(0, __FILE__ ":" TOSTRING(__LINE__) "\t" msg "\n");
#define logf(msg, ...)                                                         \
  SEGGER_RTT_printf(0, __FILE__ ":" TOSTRING(__LINE__) "\t" msg "\n",          \
                    __VA_ARGS__);

// Assertion failed: file:line
//     Message
//     expected: x actual: y
#define APP_ASSERT_EQUAL(actual, expected, ...)                                \
  if (actual != expected) {                                                    \
    SEGGER_RTT_printf(0,                                                       \
                      "Assertion failed: " __FILE__                            \
                      ":" TOSTRING(__LINE__) "\n\t" __VA_ARGS__                \
                                             "\n\texpected: %d actual: %d\n",  \
                      expected, actual);                                       \
  }

#define APP_ASSERT(expr, ...)                                                  \
  if (!(expr)) {                                                               \
    log(__FILE__ " " TOSTRING(__LINE__) " " __VA_ARGS__)                       \
  }

#else
// WARNING: expr will not be evaluated - if it is side effectey, this will break
// your production builds in mysterious ways
#define APP_ASSERT(expr, ...)                                                  \
  {}
#define APP_ASSERT_EQUAL(actual, expected, ...)                                \
  {}
#define log(msg)                                                               \
  {}
#define logf(msg, ...)                                                         \
  {}
#endif
