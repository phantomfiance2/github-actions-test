#!/bin/sh

/usr/local/sbin/nginx -c /nginx.conf
ps -ef

ls -la /usr/local/sbin/modules/*