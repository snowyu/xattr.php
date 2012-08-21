dnl $Id: config.m4 166336 2004-08-13 20:46:29Z mg $
dnl config.m4 for extension xattr

PHP_ARG_WITH(xattr, for xattr support,
Make sure that the comment is aligned:
[  --with-xattr             Include xattr support])

if test "$PHP_XATTR" != "no"; then
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="/include/attr/attributes.h"
  if test -r $PHP_XATTR/$SEARCH_FOR; then # path given as parameter
    XATTR_DIR=$PHP_XATTR
  else # search default path list
    AC_MSG_CHECKING([for xattr files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        XATTR_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi
  
  PHP_ADD_INCLUDE($XATTR_DIR/include)

  LIBNAME=attr # you may want to change this
  LIBSYMBOL=attr_get # you most likely want to change this 

  PHP_SUBST(XATTR_SHARED_LIBADD)

  PHP_NEW_EXTENSION(xattr, xattr.c isdk_xattr.c, $ext_shared)
fi
