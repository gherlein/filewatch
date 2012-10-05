<?php

unlink("./1/*.md5");
$file=$argv[1];
$cmd="./1/$file ./2/$file";
echo $cmd;
rename($cmd);
?>
