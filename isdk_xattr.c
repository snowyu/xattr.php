/*
  Copyright (c) 2012 Riceball LEE(riceball.lee@gmail.com)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

//xattr functions...

#include <errno.h>
#include <string.h>
#include "isdk_xattr.h"

#define XATTR_PREFIX  "user."

#ifdef __FreeBSD__
#include <sys/extattr.h>
#elif defined(__SUN__) || defined(__sun__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#else
#include <sys/xattr.h>
#endif

#ifdef __FreeBSD__

/* FreeBSD compatibility API */
#define XATTR_XATTR_NOFOLLOW 0x0001
#define XATTR_XATTR_CREATE 0x0002
#define XATTR_XATTR_REPLACE 0x0004
#define XATTR_XATTR_NOSECURITY 0x0008


/* Converts a freebsd format attribute list into a NULL terminated list.
 * While the man page on extattr_list_file says it is NULL terminated, 
 * it is actually the first byte that is the length of the
 * following attribute.
 */
static void convert_bsd_list(char *namebuf, size_t size)
{
    size_t offset = 0;
    while(offset < size) {
        int length = (int) namebuf[offset];
        memmove(namebuf+offset, namebuf+offset+1, length);
        namebuf[offset+length] = '\0';
        offset += length+1;
    }
}

 ssize_t xattr_getxattr(const char *path, const char *name,
                              void *value, ssize_t size, uint32_t position, 
                              int options)
{
    if (position != 0 ||
        !(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }

    if (options & XATTR_XATTR_NOFOLLOW) {
        return extattr_get_link(path, EXTATTR_NAMESPACE_USER,
                                name, value, size);
    }
    else {
        return extattr_get_file(path, EXTATTR_NAMESPACE_USER,
                                name, value, size);
    }
}

 ssize_t xattr_setxattr(const char *path, const char *name,
                              void *value, ssize_t size, uint32_t position,
                              int options)
{
    int rv = 0;
    int nofollow;

    if (position != 0) {
        return -1;
    }

    nofollow = options & XATTR_XATTR_NOFOLLOW;
    options &= ~XATTR_XATTR_NOFOLLOW;

    if (options == XATTR_XATTR_CREATE ||
        options == XATTR_XATTR_REPLACE) {

        /* meh. FreeBSD doesn't really have this in it's
         * API... Oh well.
         */
    }
    else if (options != 0) {
        return -1;
    }

    if (nofollow) {
        rv = extattr_set_link(path, EXTATTR_NAMESPACE_USER,
                                name, value, size);
    }
    else {
        rv = extattr_set_file(path, EXTATTR_NAMESPACE_USER,
                                name, value, size);
    }

    /* freebsd returns the written length on success, not zero. */
    if (rv >= 0) {
        return 0;
    }
    else {
        return rv;
    }
}

 ssize_t xattr_removexattr(const char *path, const char *name,
                                 int options)
{
    if (!(options == 0 ||
          options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }

    if (options & XATTR_XATTR_NOFOLLOW) {
        return extattr_delete_link(path, EXTATTR_NAMESPACE_USER, name);
    }
    else {
        return extattr_delete_file(path, EXTATTR_NAMESPACE_USER, name);
    }
}


 ssize_t xattr_listxattr(const char *path, char *namebuf,
                               size_t size, int options)
{
    ssize_t rv = 0;
    if (!(options == 0 ||
          options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }

    if (options & XATTR_XATTR_NOFOLLOW) {
        rv = extattr_list_link(path, EXTATTR_NAMESPACE_USER, namebuf, size);
    }
    else {
        rv = extattr_list_file(path, EXTATTR_NAMESPACE_USER, namebuf, size);
    }

    if (rv > 0 && namebuf) {
        convert_bsd_list(namebuf, rv);
    }

    return rv;
}

 ssize_t xattr_fgetxattr(int fd, const char *name, void *value,
                               ssize_t size, uint32_t position, int options)
{
    if (position != 0 ||
        !(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }

    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    }
    else {
        return extattr_get_fd(fd, EXTATTR_NAMESPACE_USER, name, value, size);
    }
}

 ssize_t xattr_fsetxattr(int fd, const char *name, void *value,
                               ssize_t size, uint32_t position, int options)
{
    int rv = 0;
    int nofollow;

    if (position != 0) {
        return -1;
    }

    nofollow = options & XATTR_XATTR_NOFOLLOW;
    options &= ~XATTR_XATTR_NOFOLLOW;

    if (options == XATTR_XATTR_CREATE ||
        options == XATTR_XATTR_REPLACE) {
        /* freebsd noop */
    }
    else if (options != 0) {
        return -1;
    }

    if (nofollow) {
        return -1;
    }
    else {
        rv = extattr_set_fd(fd, EXTATTR_NAMESPACE_USER, 
                            name, value, size);
    }

    /* freebsd returns the written length on success, not zero. */
    if (rv >= 0) {
        return 0;
    }
    else {
        return rv;
    }
}

 ssize_t xattr_fremovexattr(int fd, const char *name, int options)
{

    if (!(options == 0 ||
          options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }

    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    }
    else {
        return extattr_delete_fd(fd, EXTATTR_NAMESPACE_USER, name);
    }
}


 ssize_t xattr_flistxattr(int fd, char *namebuf, size_t size, int options)
{
    ssize_t rv = 0;

    if (!(options == 0 ||
          options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }

    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    }
    else {
        rv = extattr_list_fd(fd, EXTATTR_NAMESPACE_USER, namebuf, size);
    }

    if (rv > 0 && namebuf) {
        convert_bsd_list(namebuf, rv);
    }

    return rv;
}

#elif defined(__SUN__) || defined(__sun__)

/* Solaris 9 and later compatibility API */
#define XATTR_XATTR_NOFOLLOW 0x0001
#define XATTR_XATTR_CREATE 0x0002
#define XATTR_XATTR_REPLACE 0x0004
#define XATTR_XATTR_NOSECURITY 0x0008

#define uint32_t unsigned int

 ssize_t xattr_fgetxattr(int fd, const char *name, void *value,
                               ssize_t size, uint32_t position, int options)
{
    int xfd;
    ssize_t bytes;
    struct stat statbuf;

    /* XXX should check that name does not have / characters in it */
    xfd = openat(fd, name, O_RDONLY | O_XATTR);
    if (xfd == -1) {
	return -1;
    }
    if (lseek(xfd, position, SEEK_SET) == -1) {
	close(xfd);
	return -1;
    }
    if (value == NULL) {
        if (fstat(xfd, &statbuf) == -1) {
	    close(xfd);
	    return -1;
        }
	close(xfd);
	return statbuf.st_size;
    }
    /* XXX should keep reading until the buffer is exhausted or EOF */
    bytes = read(xfd, value, size);
    close(xfd);
    return bytes;
}

 ssize_t xattr_getxattr(const char *path, const char *name,
                              void *value, ssize_t size, uint32_t position, 
                              int options)
{
    int fd;
    ssize_t bytes;

    if (position != 0 || 
        !(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }

    fd = open(path,
	      O_RDONLY |
	      ((options & XATTR_XATTR_NOFOLLOW) ? O_NOFOLLOW : 0));
    if (fd == -1) {
	return -1;
    }
    bytes = xattr_fgetxattr(fd, name, value, size, position, options);
    close(fd);
    return bytes;
}

 ssize_t xattr_fsetxattr(int fd, const char *name, void *value,
                               ssize_t size, uint32_t position, int options)
{
    int xfd;
    ssize_t bytes = 0;

    /* XXX should check that name does not have / characters in it */
    xfd = openat(fd, name, O_XATTR | O_TRUNC |
		 ((options & XATTR_XATTR_CREATE) ? O_EXCL : 0) |
		 ((options & XATTR_XATTR_NOFOLLOW) ? O_NOFOLLOW : 0) |
		 ((options & XATTR_XATTR_REPLACE) ? O_RDWR : O_WRONLY|O_CREAT),
		 0644);
    if (xfd == -1) {
	return -1;
    }
    while (size > 0) {
	bytes = write(xfd, value, size);
	if (bytes == -1) {
	    close(xfd);
	    return -1;
	}
	size -= bytes;
	value += bytes;
    }
    close(xfd);
    return 0;
}

 ssize_t xattr_setxattr(const char *path, const char *name,
                              void *value, ssize_t size, uint32_t position,
                              int options)
{
    int fd;
    ssize_t bytes;

    if (position != 0) {
        return -1;
    }

    fd = open(path,
	      O_RDONLY | (options & XATTR_XATTR_NOFOLLOW) ? O_NOFOLLOW : 0);
    if (fd == -1) {
	return -1;
    }
    bytes = xattr_fsetxattr(fd, name, value, size, position, options);
    close(fd);
    return bytes;
}

 ssize_t xattr_fremovexattr(int fd, const char *name, int options)
{
  int xfd, status;
    /* XXX should check that name does not have / characters in it */
    if (!(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    }
    xfd = openat(fd, ".", O_XATTR, 0644);
    if (xfd == -1) {
	return -1;
    }
    status = unlinkat(xfd, name, 0);
    close(xfd);
    return status;
}

 ssize_t xattr_removexattr(const char *path, const char *name,
                                 int options)
{
    int fd;
    ssize_t status;

    fd = open(path,
	      O_RDONLY | ((options & XATTR_XATTR_NOFOLLOW) ? O_NOFOLLOW : 0));
    if (fd == -1) {
	return -1;
    }
    status =  xattr_fremovexattr(fd, name, options);
    close(fd);
    return status;
}

 ssize_t xattr_xflistxattr(int xfd, char *namebuf, size_t size, int options)
{
    int esize;
    DIR *dirp;
    struct dirent *entry;
    ssize_t nsize = 0;

    dirp = fdopendir(xfd);
    if (dirp == NULL) {
        return (-1);
    }
    while (entry = readdir(dirp)) {
        if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            continue;
	    esize = strlen(entry->d_name);
	    if (nsize + esize + 1 <= size) {
            snprintf((char *)(namebuf + nsize), esize + 1,
                    entry->d_name);
    	}
	    nsize += esize + 1; /* +1 for \0 */
    }
    closedir(dirp);
    return nsize;
}
 ssize_t xattr_flistxattr(int fd, char *namebuf, size_t size, int options)
{
    int xfd;

    xfd = openat(fd, ".", O_RDONLY);
    return xattr_xflistxattr(xfd, namebuf, size, options);
}

 ssize_t xattr_listxattr(const char *path, char *namebuf,
                               size_t size, int options)
{
    int xfd;

    xfd = attropen(path, ".", O_RDONLY);
    return xattr_xflistxattr(xfd, namebuf, size, options);
}

#elif !defined(XATTR_NOFOLLOW)
/* Linux compatibility API */
#define XATTR_XATTR_NOFOLLOW 0x0001
#define XATTR_XATTR_CREATE 0x0002
#define XATTR_XATTR_REPLACE 0x0004
#define XATTR_XATTR_NOSECURITY 0x0008
 ssize_t xattr_getxattr(const char *path, const char *name, void *value, ssize_t size, uint32_t position, int options) {
    if (position != 0 || !(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return lgetxattr(path, name, value, size);
    } else {
        return getxattr(path, name, value, size);
    }
}

 ssize_t xattr_setxattr(const char *path, const char *name, void *value, ssize_t size, uint32_t position, int options) {
    int nofollow;
    if (position != 0) {
        return -1;
    }
    nofollow = options & XATTR_XATTR_NOFOLLOW;
    options &= ~XATTR_XATTR_NOFOLLOW;
    if (options == XATTR_XATTR_CREATE) {
        options = XATTR_CREATE;
    } else if (options == XATTR_XATTR_REPLACE) {
        options = XATTR_REPLACE;
    } else if (options != 0) {
        return -1;
    }
    if (nofollow) {
        return lsetxattr(path, name, value, size, options);
    } else {
        return setxattr(path, name, value, size, options);
    }
}

 ssize_t xattr_removexattr(const char *path, const char *name, int options) {
    if (!(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return lremovexattr(path, name);
    } else {
        return removexattr(path, name);
    }
}


 ssize_t xattr_listxattr(const char *path, char *namebuf, size_t size, int options) {
    if (!(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return llistxattr(path, namebuf, size);
    } else {
        return listxattr(path, namebuf, size);
    }
}

 ssize_t xattr_fgetxattr(int fd, const char *name, void *value, ssize_t size, uint32_t position, int options) {
    if (position != 0 || !(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    } else {
        return fgetxattr(fd, name, value, size);
    }
}

 ssize_t xattr_fsetxattr(int fd, const char *name, void *value, ssize_t size, uint32_t position, int options) {
    int nofollow;
    if (position != 0) {
        return -1;
    }
    nofollow = options & XATTR_XATTR_NOFOLLOW;
    options &= ~XATTR_XATTR_NOFOLLOW;
    if (options == XATTR_XATTR_CREATE) {
        options = XATTR_CREATE;
    } else if (options == XATTR_XATTR_REPLACE) {
        options = XATTR_REPLACE;
    } else if (options != 0) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    } else {
        return fsetxattr(fd, name, value, size, options);
    }
}

 ssize_t xattr_fremovexattr(int fd, const char *name, int options) {
    if (!(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    } else {
        return fremovexattr(fd, name);
    }
}


 ssize_t xattr_flistxattr(int fd, char *namebuf, size_t size, int options) {
    if (!(options == 0 || options == XATTR_XATTR_NOFOLLOW)) {
        return -1;
    }
    if (options & XATTR_XATTR_NOFOLLOW) {
        return -1;
    } else {
        return flistxattr(fd, namebuf, size);
    }
}

#else /* Mac OS X assumed */
#define xattr_getxattr getxattr
#define xattr_fgetxattr fgetxattr
#define xattr_removexattr removexattr
#define xattr_fremovexattr fremovexattr
#define xattr_setxattr setxattr
#define xattr_fsetxattr fsetxattr
#define xattr_listxattr listxattr
#define xattr_flistxattr flistxattr
#endif

 bool IsXattrExists(const char* aFile, const char* aKey)
{
    ssize_t vLen = xattr_getxattr(aFile, aKey, NULL, 0, 0, 0);
    return vLen >= 0;
}

#ifdef ISDK_XATTR_TEST_MAIN
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "testhelp.h"

//gcc --std=c99 -I.  -o test -DHAVE_UNISTD_H -DISDK_XATTR_TEST_MAIN isdk_xattr.c sds.c zmalloc.c
int main(void) {
    {
        int fd = open ("mytestfile", O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY,
		   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd != -1) close(fd);
        test_cond("touch('mytestfile')", fd != -1);

        sds path = sdsnew("mytestfile"), key = sdsnew("mydname"), value = sdsnew("hi world!");
        test_cond("SetXattr(mytestfile, mydname, 'hi world!')", SetXattr(path, key, value)== 0);
        test_cond("IsXattrExists(mytestfile, mydname)", IsXattrExists(path, key));
        test_cond("IsXattrExists(mytestfile, notexists)", !IsXattrExists(path, "notexists"));
        test_cond("IsXattrExists(nosuchmytestfile, notexists)", !IsXattrExists("nosuchmytestfile", "notexists"));
        sds result = GetXattr(path, key);
        test_cond("GetXattr(mytestfile, mydname)",
                result && sdslen(result) == 9 && memcmp(result, "hi world!\0", 10) == 0
        );
        sdsfree(path);
        sdsfree(key);
        sdsfree(value);
        if (result) sdsfree(result);
        remove("mytestfile");
    }
    test_report();
    return 0;
}
#endif
