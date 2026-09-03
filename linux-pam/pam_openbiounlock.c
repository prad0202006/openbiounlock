#define _GNU_SOURCE
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>

static const char *socket_path = "/var/run/openbiounlock.sock";

static int wait_for_io(int fd, short events) {
    struct pollfd descriptor = { .fd = fd, .events = events };
    int result;
    do { result = poll(&descriptor, 1, 35000); } while (result < 0 && errno == EINTR);
    return result == 1 && (descriptor.revents & events) != 0 ? 0 : -1;
}

static int write_all(int fd, const char *data, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        if (wait_for_io(fd, POLLOUT) != 0) return -1;
        ssize_t written = write(fd, data + offset, length - offset);
        if (written <= 0) { if (errno == EINTR) continue; return -1; }
        offset += (size_t)written;
    }
    return 0;
}

static int read_line(int fd, char *buffer, size_t capacity) {
    size_t length = 0;
    while (length + 1 < capacity) {
        if (wait_for_io(fd, POLLIN) != 0) return -1;
        char byte;
        ssize_t received = read(fd, &byte, 1);
        if (received == 0) return -1;
        if (received < 0) { if (errno == EINTR) continue; return -1; }
        if (byte == '\n') { buffer[length] = '\0'; return 0; }
        buffer[length++] = byte;
    }
    return -1;
}

static int response_is_authorized(const char *response) {
    const char *type = strstr(response, "\"type\":\"authorized\"");
    const char *authorized = strstr(response, "\"authorized\":true");
    return type != NULL && authorized != NULL;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    (void)flags;
    const char *username = NULL;
    if (pam_get_user(pamh, &username, NULL) != PAM_SUCCESS || username == NULL) return PAM_AUTH_ERR;
    for (int i = 0; i < argc - 1; ++i) if (strcmp(argv[i], "socket") == 0) socket_path = argv[++i];
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return PAM_AUTH_ERR;
    struct sockaddr_un address = { .sun_family = AF_UNIX };
    strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);
    int result = connect(fd, (struct sockaddr *)&address, sizeof(address));
    char request[512];
    char response[1024];
    if (result == 0) {
        int length = snprintf(request, sizeof(request), "{\"type\":\"authorize\",\"user\":\"%s\"}\n", username);
        result = length > 0 && (size_t)length < sizeof(request) && write_all(fd, request, (size_t)length) == 0 && read_line(fd, response, sizeof(response)) == 0 && response_is_authorized(response) ? 0 : -1;
    }
    close(fd);
    return result == 0 ? PAM_SUCCESS : PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SERVICE_ERR; }
