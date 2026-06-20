#ifndef SAILPUSH_CONSTANTS_H
#define SAILPUSH_CONSTANTS_H

// Shared path constants used across daemon components.
// Both NotificationManager (writer) and DbusInterface (reader) use this
// to construct the pending_open file path from their cachePath.
namespace SailPushPaths {
    inline constexpr const char *PENDING_OPEN = "/pending_open";
}

#endif // SAILPUSH_CONSTANTS_H
