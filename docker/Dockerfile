FROM debian:stretch
LABEL maintainer="Fapalien Developers <dev@fapalienchain.com>"
LABEL description="Dockerised FapalienCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
