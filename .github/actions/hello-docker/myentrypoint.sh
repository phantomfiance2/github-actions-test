#!/bin/sh

echo "Hello $1"
echo "inside docker container:: pwd"
pwd
ls -la
echo "inside docker container:: /"
ls -la /