#!/bin/bash
nginxVersion=$1;
nginxFullName=nginx-$nginxVersion
moduleFolder=$2;
moduleName=$(basename "${moduleFolder}")
# to be captured by TAP of Bats
echo ${moduleName}
echo ${nginxVersion}
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
cp -r ./$moduleFolder $nginxFullName/moduleSrc/