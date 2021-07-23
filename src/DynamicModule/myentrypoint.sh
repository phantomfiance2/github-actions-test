#!/bin/sh

echo "Hello - testing nginx with mirror dynamic module"
echo "inside docker container:: uname -a"
uname -a
echo "inside docker container:: Starting nginx"
/usr/local/sbin/nginx -c /nginx.conf
echo "inside docker container:: ps -ef"
ps -ef
echo "inside docker container:: pwd"
pwd
ls -la
echo "inside docker container:: /"
ls -la /
echo "Listing all .so files:"
ls -la /usr/local/sbin/modules/*.so