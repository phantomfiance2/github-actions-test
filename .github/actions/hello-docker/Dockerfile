#For Debian 9
FROM debian:stretch-slim
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
COPY nginx-1.21.1 nginx-1.21.1
RUN cd nginx-1.21.1 && ./configure \
      --prefix=/etc/nginx/sbin/ \
      --sbin-path=/usr/local/sbin \
      --conf-path=/etc/nginx/nginx.conf \
      --with-http_ssl_module \
      --with-stream \
      --with-compat \
      --with-debug \
      --with-cc-opt='-O0 -g' && \
    make && make install
COPY myentrypoint.sh /myentrypoint.sh
ENTRYPOINT [ "/myentrypoint.sh" ]