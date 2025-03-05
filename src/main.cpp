#include "wayland-state.hpp"
#include "wayland/zwlr-layer-shell.h"
#include <csetjmp>
#include <csignal>
#include <iostream>
#include <spdlog/spdlog.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

// TODO: MOVE STATUS BAR TO CLASS
void zwlr_layer_surface_configure_handler(
    void *data, struct zwlr_layer_surface_v1 *layer_surface, uint32_t serial,
    uint32_t width, uint32_t height) {
#pragma unused(data)
#pragma unused(width)
#pragma unused(height)
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
}

void zwlr_layer_surface__closed_handler(
    void *data, struct zwlr_layer_surface_v1 *layer_surface) {
#pragma unused(data)
#pragma unused(layer_surface)
}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = zwlr_layer_surface_configure_handler,
    .closed = zwlr_layer_surface__closed_handler};

// Define an enumeration for signals
enum class SignalType {
    Interrupt = SIGINT,
    Terminate = SIGTERM,
    SegFault = SIGSEGV,
    Unknown = -1
};

// Convert integer signal to enum using static_cast
SignalType getSignalType(int signal) {
    switch (static_cast<SignalType>(signal)) { // Cast int to enum class
    case SignalType::Interrupt:
        return SignalType::Interrupt;
    case SignalType::Terminate:
        return SignalType::Terminate;
    case SignalType::SegFault:
        return SignalType::SegFault;
    default:
        return SignalType::Unknown;
    }
}

// Global flag for signal handling
volatile sig_atomic_t stop = 0;

// Signal handler function
void handle_signals(int signal) {
    SignalType sigType = getSignalType(signal);
    switch (sigType) {
    case SignalType::Interrupt:
        SPDLOG_WARN("Handling SIGINT (Interrupt Signal)");
        break;
    case SignalType::Terminate:
        SPDLOG_WARN("Handling SIGTERM (Termination Signal)");
        break;
    case SignalType::SegFault:
        SPDLOG_WARN("Handling SIGSEGV (Segmentation Fault)");
        break;
    default:
        SPDLOG_WARN("Handling Unknown Signal");
        break;
    }

    stop = 1;
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    // Register the signal handler
    std::signal(SIGINT, handle_signals);

    WaylandClientState waylandState;

    // parent surface of the status bar
    struct wl_surface *status_bar_surface = nullptr;

    status_bar_surface = wl_compositor_create_surface(waylandState.compositor);
    if (!status_bar_surface) {
        SPDLOG_ERROR("failed to create surface");
        return -1;
    }
    SPDLOG_INFO("created status bar surface");

    // mark status bar as non interactble region
    struct wl_region *empty_region =
        wl_compositor_create_region(waylandState.compositor);
    wl_surface_set_input_region(status_bar_surface, empty_region);
    wl_region_destroy(empty_region); // Clean up after setting

    // const GlobalState::MonitorInfo &mon_info = waylandState.monitorInfo;
    // work out status bar width based on physical width and scale factor
    // int status_bar_width = mon_info.width / mon_info.scale_factor;
    int status_bar_width = 2560;
    // TODO: DON'T HARDCODE HEIGHT
    int status_bar_height = 20;

    wl_surface_set_buffer_scale(status_bar_surface, 2);

    zwlr_layer_surface_v1 *layer_surface = nullptr;
    layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        waylandState.layer_shell, status_bar_surface, nullptr,
        ZWLR_LAYER_SHELL_V1_LAYER_TOP, "river_status_bar");

    zwlr_layer_surface_v1_set_size(layer_surface, status_bar_width / 2,
                                   status_bar_height / 2);
    zwlr_layer_surface_v1_set_anchor(layer_surface,
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface,
                                             status_bar_height / 2);
    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener,
                                       nullptr);

    wl_surface_commit(status_bar_surface);
    wl_display_roundtrip(waylandState.display);

    void *status_bar_shm_data = nullptr;
    struct wl_buffer *status_bar_buff = nullptr;
    status_bar_buff = waylandState.create_buffer(
        &status_bar_shm_data, status_bar_width, status_bar_height);

    int buff_size = status_bar_width * status_bar_height;
    uint32_t *pixel = static_cast<uint32_t *>(status_bar_shm_data);
    for (int i = 0; i < buff_size; ++i) {
        // ARGB
        pixel[i] = 0xFFFF0000;
    }

    wl_surface_attach(status_bar_surface, status_bar_buff, 0, 0);
    wl_surface_commit(status_bar_surface);

    int col_idx = 0;
    uint32_t colours[] = {0xFF00FF00, 0xFF0000FF, 0xFFFF0000};

    while (!stop) {
        // Dispatch any pending events (non-blocking)
        wl_display_dispatch_pending(waylandState.display);

        // Flush any pending requests to the server
        wl_display_flush(waylandState.display);

        // Change color every 3 seconds
        static auto last_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_time)
                .count() >= 3) {
            std::cout << now - last_time << "\n";
            for (int i = 0; i < buff_size; ++i) {
                // ARGB
                pixel[i] = colours[col_idx];
            }

            // double buffered - causes lots of allocations (that are freed) in valgrind
            wl_surface_damage_buffer(status_bar_surface, 0, 0, status_bar_width,
                                     status_bar_height);
            wl_surface_attach(status_bar_surface, status_bar_buff, 0, 0);
            wl_surface_commit(status_bar_surface);
            col_idx = ++col_idx % 3;
            last_time = now;
        }
        // SPDLOG_INFO("SLEEPING");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // clean up status bar buffer
    if (status_bar_buff) {
        wl_buffer_destroy(status_bar_buff);
    }
    // clean up layer surface
    if (layer_surface) {
        zwlr_layer_surface_v1_destroy(layer_surface);
        layer_surface = nullptr;
    }
    // clean up parent surface
    if (status_bar_surface) {
        wl_surface_destroy(status_bar_surface);
        status_bar_surface = nullptr;
    }
    return 0;
}
