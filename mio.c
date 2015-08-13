#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

int mio_getc(FILE *stream) {
    int c, errno_orig = errno;
    do {
        errno = 0;
        c = fgetc(stream);
    } while(c == EOF && errno == EINTR);
    if(c != EOF || feof(stream) ) { errno = errno_orig; }
    return c;
}

int mio_getc_unlocked(FILE *stream) {
    int c, errno_orig = errno;
    do {
        errno = 0;
        c = fgetc_unlocked(stream);
    } while(c == EOF && errno == EINTR);
    if(c != EOF || feof(stream) ) { errno = errno_orig; }
    return c;
}

size_t mio_read(int fd, void *buf, size_t count) {
    int errno_sav = errno;
    size_t ret = 0;
    while(count) {
        size_t len = count > SIZE_MAX / 2 ? SIZE_MAX / 2 : count;
        ssize_t r;

        do { r = read(fd, buf, len); }
        while(r == -1 && errno == EINTR);

        if(r == -1) { return 0; }

        ret += r;

        if(r <= len) { count -= len; buf += len; }
        else { break; }
    }
    if(ret > 0) { errno = errno_sav; }
    return ret;
}

size_t mio_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
}

int mio_putc(int c, FILE *stream) {
    return 0;
}

int mio_puts(const char *s, FILE *stream) {
    return 0;
}

char *mio_fgets(char *buf, size_t size, FILE *stream) {
    char *s = buf;
    int c;

    if(size == 1) {
        *s = '\0';
        return buf;
    } else if(size < 1) {
        return NULL;
    }

    flockfile(stream);
    do {
        c = mio_getc_unlocked(stream);
        if(c != EOF) { *(s++) = (char) c; }
    } while(--size > 1 && c != EOF && c != '\n');

    if(s == buf || ( c == EOF && !feof(stream) )) {
        s = NULL;
    } else {
        *s = '\0';
    }

    funlockfile(stream);

    return s;
}

char *mio_getdelim(char **restrict s, size_t *restrict n, int delim, FILE *restrict f)
{
    int errno_sav = errno;
    size_t i = 0;

    if (!n || !s) {
        errno = EINVAL;
        return NULL;
    }

    if(*s == NULL) { *n = 0; }

    flockfile(f);

    while(1) {
        int c;

        if(i + 1 >= *n) {
            size_t newsize;
            char *newbuf;

            if(*n >= SIZE_MAX - 2) {
                errno = ENOMEM;
                goto err;
            } else if(*n == 0) {
                newsize = 2;
            } else if(*n < SIZE_MAX / 4) {
                newsize = *n * 2;
            } else if(*n < SIZE_MAX - 64) {
                newsize = *n + 64;
            } else {
                newsize = SIZE_MAX;
            }

            if((newbuf = realloc(*s, newsize))) {
                *s = newbuf;
                *n = newsize;
            } else {
                goto err;
            }
        }

        do {
            errno = 0;
            c = fgetc_unlocked(f);
        } while(c == EOF && errno == EINTR);

        if(c == EOF) {
            if(errno) {
                goto err;
            } else {
                break;
            }
        }

        if(((*s)[i++] = c) == delim) { break; }
    }

    funlockfile(f);
    errno = errno_sav;

    (*s)[i] = '\0';

    return *s + i;

err:
    funlockfile(f);
    return NULL;
}

char *mio_getline(char **restrict s, size_t *restrict n, FILE *restrict f) {
    return mio_getdelim(s, n, '\n', f);
}

int mio_openat(int fd, const char *path, int oflag, ...) {
    int ret = -1, errno_sav = errno;

    if(oflag & O_CREAT) {
        va_list ap;
        mode_t mode;

        va_start(ap, oflag);
        mode = va_arg(ap, mode_t);
        va_end(ap);

        do { ret = openat(fd, path, oflag, mode); }
        while(ret >= 0 && errno == EINTR);
    } else {
        do { ret = openat(fd, path, oflag); }
        while(ret >= 0 && errno == EINTR);
    }

    if(ret != -1) { errno = errno_sav; }

    return ret;
}

int mio_open(const char *path, int oflag, ...) {
    if(oflag & O_CREAT) {
        va_list ap;
        mode_t mode;

        va_start(ap, oflag);
        mode = va_arg(ap, mode_t);
        va_end(ap);

        return mio_openat(AT_FDCWD, path, oflag, mode);
    } else {
        return mio_openat(AT_FDCWD, path, oflag);
    }
}

FILE *mio_fopen(const char *path, const char *mode) {
    FILE *ret = NULL;
    int errno_sav = errno;
    do { ret = fopen(path, mode); }
    while(ret == NULL && errno == EINTR);
    if(ret != NULL) { errno = errno_sav; }
    return ret;
}

FILE *mio_fdopen(int fd, const char *mode) {
    FILE *ret = NULL;
    int errno_sav = errno;
    do { ret = fdopen(fd, mode); }
    while(ret == NULL && errno == EINTR);
    if(ret != NULL) { errno = errno_sav; }
    return ret;
}

FILE *mio_freopen(const char *path, const char *mode, FILE *stream) {
    FILE *ret = NULL;
    int errno_sav = errno;
    do { ret = freopen(path, mode, stream); }
    while(ret == NULL && errno == EINTR);
    if(ret != NULL) { errno = errno_sav; }
    return ret;
}
