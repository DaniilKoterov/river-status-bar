#pragma once

#include <string>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "wayland/zwlr-layer-shell-wrapper.hpp"

class WaylandClientState {
  public:
    wl_display *display;
    wl_registry *registry;
    wl_output *output;
    wl_compositor *compositor;
    wl_subcompositor *sub_compositor;
    wl_shm *shm;
    zwlr_layer_shell_v1 *layer_shell;

    struct MonitorInfo {
        int refresh_rate;
        int width;
        int scale_factor;
    };

    MonitorInfo monitorInfo;

    WaylandClientState();
    ~WaylandClientState();

    struct wl_buffer *create_buffer(void **shm_data, int width, int height);

  private:
    // wl_registry handlers
    static void registry_global_handler(void *data,
                                        struct wl_registry *registry,
                                        uint32_t name, const char *interface,
                                        uint32_t version);
    static void registry_global_remove_handler(void *data,
                                               struct wl_registry *registry,
                                               uint32_t name);
    const struct wl_registry_listener registry_listener;

    // wl_output handlers
    static void output_geometry_handler(void *data, struct wl_output *output,
                                        int x, int y, int physical_width,
                                        int physical_height, int subpixel,
                                        const char *make, const char *model,
                                        int transform);
    static void output_mode_handler(void *data, struct wl_output *output,
                                    uint32_t flags, int width, int height,
                                    int refresh);
    static void output_done_handler(void *data, struct wl_output *output);
    static void output_scale_handler(void *data, struct wl_output *output,
                                     int factor);
    static void output_name_handler(void *data, struct wl_output *output,
                                    const char *name);
    static void output_description_handler(void *data, struct wl_output *output,
                                           const char *description);
    const struct wl_output_listener output_listener;

    // class util functions

//     // Set FD_CLOEXEC flag on a file descriptor
//     static int set_cloexec_or_close(int fd);
//     // Create a temporary file with O_CLOEXEC flag
//     static int create_tmpfile_cloexec(std::string &tmpname);
//     // Create an anonymous file suitable for mmap
//     static int os_create_anonymous_file(off_t size);
};