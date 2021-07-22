#!/bin/bash
nginxVersion=$1;
nginxFullName=nginx-$nginxVersion
moduleFolder=$2;
moduleName=$(basename "${moduleFolder}")

echo "##########"
echo ${moduleName}
echo ${nginxVersion}
echo "##########"


if [ ! -d "bin/" ]
then
  mkdir bin/
fi
wget https://nginx.org/download/$nginxFullName.tar.gz -O $nginxFullName.tar.gz
tar -xvf $nginxFullName.tar.gz
if [ ! -d "$nginxFullName/moduleSrc" ]
then
  mkdir $nginxFullName/moduleSrc/
fi
cp -r $moduleFolder $nginxFullName/moduleSrc/
echo "just before running docker-compose"
# docker-compose build --build-arg module_name=$moduleName --build-arg nginx_version=$nginxVersion 2>/dev/null
cd ./.github/actions/hello-docker/.
echo "current pwd:"
pwd
docker build -t docker-action --build-arg module_name=$moduleName --build-arg nginx_version=$nginxVersion . && docker run docker-action
