# Copyright Vera-Licona Research Group (C) 2015
# Author: Andrew Gainer-Dewar, Ph.D. <andrew.gainer.dewar@gmail.com>

# Set up variables
FROM algorun/algorun:latest
MAINTAINER Andrew Gainer-Dewar, Ph.D. <andrew.gainer.dewar@gmail.com>

# Add any algorithm dependencies here
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    python-pip \
    && rm -rf /var/lib/apt/lists/

RUN pip install \
    simplejson

# Fetch and build the code
COPY ./src /home/algorithm/src/
WORKDIR /home/algorithm/src/alg
RUN g++ -O3 -o BMR BMR.cpp
WORKDIR /home/algorithm/src

# Add the Algorun manifest and examples
COPY ./algorun_info /home/algorithm/web/algorun_info/

# Add your command to execute an algorithm as environment variable EXEC_COMMAND
ENV EXEC_COMMAND "mhs"
