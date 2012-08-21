--TEST--
Check for xattr presence
--SKIPIF--
<?php if (!extension_loaded("xattr")) print "skip"; ?>
--FILE--
<?php 
echo "xattr extension is available";
?>
--EXPECT--
xattr extension is available
