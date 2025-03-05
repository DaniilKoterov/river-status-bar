#include "wayland-state.hpp"
#include "utils.hpp"
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/mman.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

// create buffer
struct wl_buffer *WaylandClientState::create_buffer(void **shm_data, int width,
                                                    int height) {
    struct wl_shm_pool *pool = nullptr;
    int stride = width * 4; // 4 bytes per pixel in our ARGB8888 format.
    int size = stride * height;
    struct wl_buffer *buff = nullptr;

    int fd = os_create_anonymous_file(size);

    if (fd < 0) {
        SPDLOG_ERROR("failed to create buffer. size {}", size);
        return nullptr;
    }

    *shm_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (shm_data == MAP_FAILED) {
        SPDLOG_ERROR("mmap failed");
        close(fd);
        return nullptr;
    }

    pool = wl_shm_create_pool(this->shm, fd, size);
    buff = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
                                     WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);

    return buff;
}

// wl_registry handlers
void WaylandClientState::registry_global_handler(void *data,
                                                 struct wl_registry *registry,
                                                 uint32_t name,
                                                 const char *interface,
                                                 uint32_t version) {
    SPDLOG_DEBUG("interface: '{}', version: {}, name: {}", interface, version,
                 name);

    WaylandClientState *self = static_cast<WaylandClientState *>(data);

    if (strcmp(interface, "wl_compositor") == 0) {
        self->compositor = static_cast<wl_compositor *>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 6));
    } else if (strcmp(interface, "wl_shm") == 0) {
        self->shm = static_cast<wl_shm *>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (strcmp(interface, "zwlr_layer_shell_v1") == 0) {
        self->layer_shell = static_cast<zwlr_layer_shell_v1 *>(wl_registry_bind(
            registry, name, &zwlr_layer_shell_v1_interface, 4));
    } else if (strcmp(interface, "wl_output") == 0) {
        self->output = static_cast<wl_output *>(
            wl_registry_bind(registry, name, &wl_output_interface, 4));
    } else if (strcmp(interface, "wl_subcompositor") == 0) {
        self->sub_compositor = static_cast<wl_subcompositor *>(
            wl_registry_bind(registry, name, &wl_subcompositor_interface, 1));
    }
}

void WaylandClientState::registry_global_remove_handler(
    void *data, struct wl_registry *registry, uint32_t name) {
#pragma unused(data)
#pragma unused(registry)
    SPDLOG_DEBUG("removed: {}", name);
}

// wl_output handlers
void WaylandClientState::output_geometry_handler(
    void *data, struct wl_output *output, int x, int y, int physical_width,
    int physical_height, int subpixel, const char *make, const char *model,
    int transform) {
#pragma unused(data)
#pragma unused(output)
#pragma unused(x)
#pragma unused(y)
#pragma unused(physical_width)
#pragma unused(physical_height)
#pragma unused(subpixel)
#pragma unused(make)
#pragma unused(model)
#pragma unused(transform)
}

void WaylandClientState::output_mode_handler(void *data,
                                             struct wl_output *output,
                                             uint32_t flags, int width,
                                             int height, int refresh) {
#pragma unused(output)
    MonitorInfo *mon_info = static_cast<MonitorInfo *>(data);
    SPDLOG_DEBUG("Flags: {:#x}, Width: {}, Height: {}, Refresh Rate: {} Hz",
                 flags, width, height, refresh / 1000);
    // store the monitor width
    mon_info->width = width;
    // store the monitor refresh rate
    mon_info->refresh_rate = refresh / 1000;
}

void WaylandClientState::output_done_handler(void *data,
                                             struct wl_output *output) {
#pragma unused(data)
#pragma unused(output)
}

void WaylandClientState::output_scale_handler(void *data,
                                              struct wl_output *output,
                                              int factor) {
#pragma unused(output)
    MonitorInfo *mon_info = static_cast<MonitorInfo *>(data);
    SPDLOG_DEBUG("Display scale factor: {}", factor);
    mon_info->scale_factor = factor;
}

void WaylandClientState::output_name_handler(void *data,
                                             struct wl_output *output,
                                             const char *name) {
#pragma unused(data)
#pragma unused(output)
#pragma unused(name)
}

void WaylandClientState::output_description_handler(void *data,
                                                    struct wl_output *output,
                                                    const char *description) {
#pragma unused(data)
#pragma unused(output)
#pragma unused(description)
}

WaylandClientState::WaylandClientState()
    : registry_listener{.global = WaylandClientState::registry_global_handler,
                        .global_remove =
                            WaylandClientState::registry_global_remove_handler},
      output_listener{.geometry = WaylandClientState::output_geometry_handler,
                      .mode = WaylandClientState::output_mode_handler,
                      .done = WaylandClientState::output_done_handler,
                      .scale = WaylandClientState::output_scale_handler,
                      .name = WaylandClientState::output_name_handler,
                      .description =
                          WaylandClientState::output_description_handler} {

    SPDLOG_INFO("Global State Constructor");
    // initialize pointers to nullptr
    this->display = nullptr;
    this->registry = nullptr;
    this->output = nullptr;
    this->compositor = nullptr;
    this->sub_compositor = nullptr;
    this->shm = nullptr;
    this->layer_shell = nullptr;

    this->display = wl_display_connect(nullptr);
    if (!this->display) {
        SPDLOG_ERROR("can't connect to display");
    }
    SPDLOG_INFO("connected to display");

    this->registry = wl_display_get_registry(this->display);
    if (!this->registry) {
        // TODO: THROW ERROR
        SPDLOG_ERROR("can't get registry");
    }
    // pass this pointer so handler can set class variables
    wl_registry_add_listener(this->registry, &this->registry_listener, this);

    // wait for interfaces to be loaded
    wl_display_roundtrip(this->display);

    if (!this->compositor || !this->shm || !this->layer_shell ||
        !this->output || !this->sub_compositor) {
        // TODO: THROW ERROR
        SPDLOG_ERROR("missing required interfaces");
    }
    SPDLOG_INFO("required interfaces available");

    // pass MonitorInfo struct so handler can set it
    wl_output_add_listener(this->output, &this->output_listener,
                           &this->monitorInfo);
    // wait until monitor info is fetched
    wl_display_roundtrip(this->display);
}

WaylandClientState::~WaylandClientState() {
    SPDLOG_DEBUG("freeing global state");

    if (this->layer_shell) {
        zwlr_layer_shell_v1_destroy(this->layer_shell);
        this->layer_shell = nullptr;
    }

    if (this->shm) {
        wl_shm_destroy(this->shm);
        this->shm = nullptr;
    }

    if (this->sub_compositor) {
        wl_subcompositor_destroy(this->sub_compositor);
        this->sub_compositor = nullptr;
    }

    if (this->compositor) {
        wl_compositor_destroy(this->compositor);
        this->compositor = nullptr;
    }

    if (this->output) {
        wl_output_destroy(this->output);
        this->output = nullptr;
    }

    if (this->registry) {
        wl_registry_destroy(this->registry);
        this->registry = nullptr;
    }

    if (this->display) {
        wl_display_disconnect(display);
        display = nullptr;
    }
}