#pragma once

#include "game_launcher/core/game_status.h" // Include the status definitions

namespace game_launcher::core {

// Interface for components that want to receive game status updates
// from the core IPC service.
class IGameStatusListener {
public:
    virtual ~IGameStatusListener() = default; // Virtual destructor is important for interfaces

    // Called by the IIPCService implementation whenever a game's status changes.
    // This method should be implemented to be thread-safe or handle marshalling
    // to the correct thread (e.g., UI thread for CEF) if necessary.
    virtual void OnGameStatusUpdate(const GameStatusUpdate& update) = 0;
};

} // namespace game_launcher::core
