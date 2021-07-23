#!/bin/bash
nginxVersion=$1;
# nginxFullName=nginx-$nginxVersion
moduleFolder=$2;
moduleName=$(basename "${moduleFolder}")

docker build -t docker-action --build-arg module_name=$moduleName --build-arg nginx_version=$nginxVersion . && docker run docker-action
