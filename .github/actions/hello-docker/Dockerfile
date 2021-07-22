#For Debian 9
FROM debian:stretch-slim
ARG nginx_version
ARG module_name
RUN apt-get update && apt-get install -y \
      gcc \
      libpcre3 \
      libpcre3-dev \
      zlib1g-dev \
      make \
      libssl-dev \
      curl \
      gdb \
      procps \
      gdbserver
WORKDIR /root
# testing 2 lines
RUN pwd
RUN ls -la
COPY nginx-$nginx_version nginx-$nginx_version
RUN cd nginx-${nginx_version} && ./configure \
      --prefix=/etc/nginx/sbin/ \
      --sbin-path=/usr/local/sbin \
      --conf-path=/etc/nginx/nginx.conf \
      --with-http_ssl_module \
      --with-stream \
      --with-compat \
      --with-debug \
      --with-cc-opt='-O0 -g' && \
    make && make install 
RUN cd nginx-$nginx_version && ./configure --with-compat --add-dynamic-module=moduleSrc/$module_name && make modules
RUN mkdir -p /usr/local/sbin/modules/
RUN cp /root/nginx-$nginx_version/objs/*so  /usr/local/sbin/modules/
COPY myentrypoint.sh /myentrypoint.sh
COPY nginx.conf /nginx.conf
ENTRYPOINT [ "/myentrypoint.sh" ]
