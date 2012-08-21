--TEST--
Check write attr
--SKIPIF--
<?php
  //if (!extension_loaded("xattr")) print "skip";
  if (!xxattr_set("/Users/lixueyu/tmp", "user.key", "heee")) print "skip";
?>
--FILE--
<?php 
echo "xattr extension is available";
?>
--EXPECT--
xattr extension is available
