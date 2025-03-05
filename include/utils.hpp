#pragma once 

#include <string>
#include <unistd.h>

// Set FD_CLOEXEC flag on a file descriptor
int set_cloexec_or_close(int fd);
// Create a temporary file with O_CLOEXEC flag
int create_tmpfile_cloexec(std::string &tmpname);

// Create an anonymous file suitable for mmap
int os_create_anonymous_file(off_t size);