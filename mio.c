/*
 * Copyright 2015 Andrew Gregory <andrew.gregory.8@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Project URL: http://github.com/andrewgregory/mio.c
 */

#ifndef MIO_C
#define MIO_C

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

int mio_getc_unlocked(FILE *stream) {
    int c, errno_orig = errno;
    do {
        errno = 0;
        c = fgetc_unlocked(stream);
    } while(c == EOF && errno == EINTR);
    if(errno == EINTR || errno == 0 ) { errno = errno_orig; }
    return c;
}

int mio_getc(FILE *stream) {
    int c;
    flockfile(stream);
    c = mio_getc_unlocked(stream);
    funlockfile(stream);
    return c;
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

char *mio_getdelim(char **restrict s, size_t *restrict n, int delim,
        FILE *restrict f) {
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

int mio_pselect(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
        fd_set *restrict errorfds, const struct timespec *restrict timeout,
        const sigset_t *restrict sigmask) {
    int ret, errno_sav = errno;
    do { ret = pselect(nfds, readfds, writefds, errorfds, timeout, sigmask); }
    while(ret == -1 && errno == EINTR);
    if(ret != -1) { errno = errno_sav; }
    return ret;
}
int mio_select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
        fd_set *restrict errorfds, struct timeval *restrict timeout) {
    int ret, errno_sav = errno;
    do { ret = select(nfds, readfds, writefds, errorfds, timeout); }
    while(ret == -1 && errno == EINTR);
    if(ret != -1) { errno = errno_sav; }
    return ret;
}

FILE *mio_fopenat(int dirfd, const char *path, const char *mode) {
    int fd, flags = 0, rwflag = 0;
    FILE *stream;
    switch(*(mode++)) {
        case 'r': rwflag = O_RDONLY; break;
        case 'w': rwflag = O_WRONLY; flags |= O_CREAT | O_TRUNC; break;
        case 'a': rwflag = O_WRONLY; flags |= O_CREAT | O_APPEND; break;
        default: errno = EINVAL; return NULL;
    }
    if(mode[1] == 'b') { mode++; }
    if(mode[1] == '+') { mode++; rwflag = O_RDWR; }
    while(*mode) {
        switch(*(mode++)) {
            case 'e': flags |= O_CLOEXEC; break;
            case 'x': flags |= O_EXCL; break;
        }
    }
    if((fd = openat(dirfd, path, flags | rwflag, 0666)) < 0) { return NULL; }
    if((stream = fdopen(fd, mode)) == NULL) { close(fd); return NULL; }
    return stream;
}


int mio_readdir(DIR *dirp, struct dirent **result) {
    int err = errno;
    if((*result = readdir(dirp)) || errno == 0) {
        errno = err;
        return 0;
    } else {
        return errno;
    }
}

#endif /* MIO_C */
