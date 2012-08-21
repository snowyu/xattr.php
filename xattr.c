/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Marcin Gibula <mg@iceni.pl>                                  |
  +----------------------------------------------------------------------+
*/

/* $Id: xattr.c 324524 2012-03-25 15:35:37Z felipe $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define XATTR_BUFFER_SIZE	1024	/* Initial size for internal buffers, feel free to change it */

/* These prefixes have been taken from attr(5) man page */
#define XATTR_USER_PREFIX	"user."
#define XATTR_ROOT_PREFIX	"trusted."

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_xattr.h"

#include <stdlib.h>

/*
 * One beautiful day libattr will implement listing extended attributes,
 * but until then we must do it on our own, so these headers are required.
 */
#include <sys/types.h>
#include "isdk_xattr.h"

/* {{{ xattr_functions[]
 *
 * Every user visible function must have an entry in xattr_functions[].
 */
zend_function_entry xattr_functions[] = {
	PHP_FE(xattr_set,		NULL)
	PHP_FE(xattr_get,		NULL)
	PHP_FE(xattr_remove,	NULL)
	PHP_FE(xattr_list,		NULL)
	PHP_FE(xattr_supported,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in xattr_functions[] */
};
/* }}} */

/* {{{ xattr_module_entry
 */
zend_module_entry xattr_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"xattr",
	xattr_functions,
	PHP_MINIT(xattr),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(xattr),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_XATTR_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_XATTR
ZEND_GET_MODULE(xattr)
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(xattr)
{
	REGISTER_LONG_CONSTANT("XATTR_ROOT", ATTR_ROOT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XXATTR_XATTR_NOFOLLOW", XATTR_XATTR_NOFOLLOW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XXATTR_XATTR_CREATE", XATTR_XATTR_CREATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XXATTR_XATTR_REPLACE", XATTR_XATTR_REPLACE, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(xattr)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "xattr support", "enabled");
	php_info_print_table_row(2, "PECL module version", PHP_XATTR_VERSION);
	php_info_print_table_end();
}
/* }}} */

/* {{{ proto bool xattr_set(string path, string name, string value [, int flags])
   Set an extended attribute of file */
PHP_FUNCTION(xattr_set)
{
	char *attr_name = NULL;
	char *attr_value = NULL;
	char *path = NULL;
	int error, tmp, value_len, flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|l", &path, &tmp, &attr_name, &tmp, &attr_value, &value_len, &flags) == FAILURE) {
		return;
	}

	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC) 
#if PHP_API_VERSION < 20100412
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}
	
	/* Ensure that only allowed bits are set */
	flags &= ATTR_ROOT | XATTR_XATTR_NOFOLLOW | XATTR_XATTR_CREATE | XATTR_XATTR_REPLACE; 
	
	/* Attempt to set an attribute, warn if failed. */ 
	error = xattr_setxattr(path, attr_name, attr_value, value_len, 0, flags);
	if (error == -1) {
		switch (errno) {
			case E2BIG:
				php_error(E_WARNING, "%s The value of the given attribute is too large", get_active_function_name(TSRMLS_C));
				break;
			case EPERM:
			case EACCES:
				php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
				break;
			case EOPNOTSUPP:
				php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
				break;
			case ENOENT:
			case ENOTDIR:
				php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
				break;
		}
		
		RETURN_FALSE;
	}
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string xattr_get(string path, string name [, int flags])
   Returns a value of an extended attribute */
PHP_FUNCTION(xattr_get)
{
	char *attr_name = NULL;
	char *attr_value = NULL;
	char *path = NULL;
	int error, tmp, flags = 0;
	ssize_t buffer_size = XATTR_BUFFER_SIZE;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &path, &tmp, &attr_name, &tmp, &flags) == FAILURE) {
		return;
	}

	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC) 
#if PHP_API_VERSION < 20100412
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}
	
	/* Ensure that only allowed bits are set */
	flags &= ATTR_ROOT | XATTR_XATTR_NOFOLLOW; 
	
	
	/* 
	 * If buffer is NULL then attr_get return the buffer_size
	 */
	buffer_size = xattr_getxattr(path, attr_name, NULL, 0, 0, flags);
	if (buffer_size >= 0) {
		/* Allocate a buffer with starting size XATTR_BUFFER_SIZE bytes */
		attr_value = emalloc(buffer_size);
		if (!attr_value)
			RETURN_FALSE;
		error = xattr_getxattr(path, attr_name, attr_value, buffer_size, 0, flags);
		/* Return a string if everything is ok */
		if (error>=0) {
			RETURN_STRINGL(attr_value, buffer_size, 0);
		} 
		
		/* Error handling part */
		efree(attr_value);
	}

	
	/* Give warning for some common error conditions */
	switch (errno) {
		case ENOENT:
		case ENOTDIR:
			php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
			break;
		case EPERM:
		case EACCES:
			php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
			break;
		case EOPNOTSUPP:
			php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
			break;
	}
	
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool xattr_supported(string path [, int flags])
   Checks if filesystem supports extended attributes */
PHP_FUNCTION(xattr_supported)
{
	char *buffer, *path = NULL;
	int error, tmp, flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &path, &tmp, &flags) == FAILURE) {
		return;
	}
	
	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC) 
#if PHP_API_VERSION < 20100412
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_NULL();
	}
	
	/* Is "test" a good name? */
	error = xattr_getxattr(path, "user.test", NULL, 0 ,0, flags);

	if (error >= 0)
		RETURN_TRUE;
	
	switch (errno) {
		case ENOTSUP:
			RETURN_FALSE;
		case ENOENT:
		case ENOTDIR:
			php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
			break;
		case EACCES:
			php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
			break;
	}
	
	RETURN_NULL();
}
/* }}} */

/* {{{ proto string xattr_remove(string path, string name [, int flags])
   Remove an extended attribute of file */
PHP_FUNCTION(xattr_remove)
{
	char *attr_name = NULL;
	char *path = NULL;
	int error, tmp, flags = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &path, &tmp, &attr_name, &tmp, &flags) == FAILURE) {
		return;
	}
	
	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC) 
#if PHP_API_VERSION < 20100412
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}
	
	/* Ensure that only allowed bits are set */
	flags &= ATTR_ROOT | XATTR_XATTR_NOFOLLOW; 
	
	/* Attempt to remove an attribute, warn if failed. */ 
	error = xattr_removexattr(path, attr_name, flags);
	if (error == -1) {
		switch (errno) {
			case E2BIG:
				php_error(E_WARNING, "%s The value of the given attribute is too large", get_active_function_name(TSRMLS_C));
				break;
			case EPERM:
			case EACCES:
				php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
				break;
			case EOPNOTSUPP:
				php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
				break;
			case ENOENT:
			case ENOTDIR:
				php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
				break;
		}
		
		RETURN_FALSE;
	}
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array xattr_list(string path [, int flags])
   Get list of extended attributes of file */
PHP_FUNCTION(xattr_list)
{
	char *buffer, *path = NULL;
	char *p, *prefix;
	int error, tmp, flags = 0;
	ssize_t i = 0, buffer_size = XATTR_BUFFER_SIZE;
	size_t len, prefix_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &path, &tmp, &flags) == FAILURE) {
		return;
	}
	
	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC) 
#if PHP_API_VERSION < 20100412
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}

	buffer_size = xattr_listxattr(path, NULL, 0, flags);
	if (buffer_size >= 0) {
		buffer = emalloc(buffer_size);
		if (!buffer)
			RETURN_FALSE;
		error = xattr_listxattr(path, buffer, buffer_size, flags);
		if (error >=0) {
			array_init(return_value);
			p = buffer;
			
			
			/* 
			 * We go through the whole list and add entries beginning with selected
			 * prefix to the return_value array.
			 */
			while (i != buffer_size) {
				len = strlen(p) + 1;	/* +1 for NULL */
				add_next_index_stringl(return_value, p, len, 1);
				
				p += len;
				i += len;
			}
		}
		efree(buffer);
	}
	else if (errno) { /* Print warning on common errors */
		switch (errno) {
			case ENOTSUP:
				php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
				break;
			case ENOENT:
			case ENOTDIR:
				php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
				break;
			case EACCES:
				php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
				break;
		}
		
		RETURN_FALSE;
	}
	
}
/* }}} */   

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
