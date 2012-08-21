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

#ifndef isdk_xattr__h
 #define isdk_xattr__h

#include "config.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */


#ifdef __FreeBSD__

/* FreeBSD compatibility API */
#define XATTR_XATTR_NOFOLLOW 0x0001
#define XATTR_XATTR_CREATE 0x0002
#define XATTR_XATTR_REPLACE 0x0004
#define XATTR_XATTR_NOSECURITY 0x0008
#elif defined(__SUN__) || defined(__sun__)
/* Solaris 9 and later compatibility API */
#define XATTR_XATTR_NOFOLLOW 0x0001
#define XATTR_XATTR_CREATE 0x0002
#define XATTR_XATTR_REPLACE 0x0004
#define XATTR_XATTR_NOSECURITY 0x0008
#elif !defined(XATTR_NOFOLLOW)
/* Linux compatibility API */
#define XATTR_XATTR_NOFOLLOW 0x0001
#define XATTR_XATTR_CREATE 0x0002
#define XATTR_XATTR_REPLACE 0x0004
#define XATTR_XATTR_NOSECURITY 0x0008
#endif
#define ATTR_ROOT 0x0010

 #ifdef __cplusplus
 extern "C"
 {
 #endif


//the xattr low level functions:
 ssize_t xattr_getxattr(const char *path, const char *name,
                              void *value, ssize_t size, uint32_t position, 
                              int options);
 ssize_t xattr_setxattr(const char *path, const char *name,
                              void *value, ssize_t size, uint32_t position,
                              int options);
 ssize_t xattr_removexattr(const char *path, const char *name,
                                 int options);
 ssize_t xattr_listxattr(const char *path, char *namebuf,
                               size_t size, int options);
 ssize_t xattr_fgetxattr(int fd, const char *name, void *value,
                               ssize_t size, uint32_t position, int options);
 ssize_t xattr_fsetxattr(int fd, const char *name, void *value,
                               ssize_t size, uint32_t position, int options);
 ssize_t xattr_fremovexattr(int fd, const char *name, int options);
 ssize_t xattr_flistxattr(int fd, char *namebuf, size_t size, int options);


 bool IsXattrExists(const char* aFile, const char* aKey);


 #ifdef __cplusplus
 }
 #endif

#endif
