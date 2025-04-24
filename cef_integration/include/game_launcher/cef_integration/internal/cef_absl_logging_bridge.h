// cef_integration/include/game_launcher/cef_integration/internal/cef_absl_logging_bridge.h
#ifndef GAME_LAUNCHER_CEF_INTEGRATION_INTERNAL_CEF_ABSL_LOGGING_BRIDGE_H_
#define GAME_LAUNCHER_CEF_INTEGRATION_INTERNAL_CEF_ABSL_LOGGING_BRIDGE_H_

// Purpose: Include necessary CEF headers and Abseil logging header,
// resolving macro conflicts between them.

// 1. Include essential CEF headers that might define logging macros.
#include "include/base/cef_logging.h" // Include the header that defines the macros

// 2. Undefine potentially conflicting macros from CEF's base/logging.h
//    These might clash with Abseil's log/log.h or other libraries.
#undef LOG
#undef VLOG
#undef DLOG
#undef LOG_IF
#undef VLOG_IF
#undef DLOG_IF
#undef CHECK
#undef DCHECK
#undef CHECK_EQ
#undef DCHECK_EQ
#undef CHECK_NE
#undef DCHECK_NE
#undef CHECK_LE
#undef DCHECK_LE
#undef CHECK_LT
#undef DCHECK_LT
#undef CHECK_GE
#undef DCHECK_GE
#undef CHECK_GT
#undef DCHECK_GT
// Add any other conflicting macros here if needed.

// 3. Now include the Abseil logging header.
#include "absl/log/log.h"

#endif // GAME_LAUNCHER_CEF_INTEGRATION_INTERNAL_CEF_ABSL_LOGGING_BRIDGE_H_
