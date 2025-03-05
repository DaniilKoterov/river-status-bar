#include "utils.hpp"
#include <fcntl.h>
#include <spdlog/spdlog.h>

// Set FD_CLOEXEC flag on a file descriptor
int set_cloexec_or_close(int fd) {
    if (fd == -1)
        return -1;

    long flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        close(fd);
        return -1;
    }

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
        close(fd);
        return -1;
    }

    return fd;
}

// Create a temporary file with O_CLOEXEC flag
int create_tmpfile_cloexec(std::string &tmpname) {
    int fd;

#ifdef HAVE_MKOSTEMP
    fd = mkostemp(tmpname.data(), O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname.c_str());
#else
    fd = mkstemp(tmpname.data());
    if (fd >= 0) {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname.c_str());
    }
#endif

    return fd;
}

// Create an anonymous file suitable for mmap
int os_create_anonymous_file(off_t size) {
    const std::string template_path = "/tutorial-shared-XXXXXX";
    const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");

    if (!xdg_runtime_dir) {
        errno = ENOENT;
        throw std::runtime_error("XDG_RUNTIME_DIR not set");
    }

    std::string name = std::string(xdg_runtime_dir) + template_path;

    int fd = create_tmpfile_cloexec(name);
    if (fd < 0) {
        throw std::runtime_error("Failed to create temporary file");
    }

    if (ftruncate(fd, size) < 0) {
        close(fd);
        throw std::runtime_error("Failed to set file size");
    }

    return fd;
}
