#include "ErrorGui.hpp"
#include "ipc/TriPlayer.hpp"
#include "PlayerGui.hpp"
#include "TriOverlay.hpp"

TriOverlay::TriOverlay() : tsl::Overlay() {
    this->triInitialized = false;
}

void TriOverlay::initServices() {
    // Attempt to connect to TriPlayer
    this->triInitialized = TriPlayer::initialize();
}

void TriOverlay::exitServices() {
    if (this->triInitialized) {
        TriPlayer::exit();
        this->triInitialized = false;
    }
}

std::unique_ptr<tsl::Gui> TriOverlay::loadInitialGui() {
    // Show error frame if service failed to initialize
    if (!this->triInitialized) {
        return std::make_unique<ErrorGui>();
    }

    // Otherwise proceed to normal (player) frame
    return std::make_unique<PlayerGui>();
}