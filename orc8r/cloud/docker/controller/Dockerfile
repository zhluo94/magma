# ------------------------------------------------------------------------------
# Base image (for tests, precommit, codegen, etc.)
# ------------------------------------------------------------------------------
ARG baseImage="ubuntu:xenial"
FROM ${baseImage} as base

# Install the runtime deps from apt
RUN apt-get update && \
    apt-get install -y curl make git gcc bzr unzip vim sudo virtualenv pkg-config \
    python3-dev python3-pip libsystemd-dev libssl-dev swig

# Install Golang 1.12
WORKDIR /usr/local
RUN curl https://dl.google.com/go/go1.12.linux-amd64.tar.gz -O && \
    tar xf go1.12.linux-amd64.tar.gz && \
    cp -r go/bin/* /usr/local/bin/
ENV GO111MODULE on

# Env variables for specifying the output directories
ENV GOBIN /build/bin
ENV PLUGIN_DIR /build/plugins
ENV PYTHON_BUILD /build/python
ENV PIP_CACHE_HOME ~/.pipcache
ENV PATH ${PATH}:${GOBIN}:${PYTHON_BUILD}/bin
# Use public go modules proxy
ENV GOPROXY https://proxy.golang.org

# Install protobuf compiler
RUN curl -Lfs https://github.com/google/protobuf/releases/download/v3.1.0/protoc-3.1.0-linux-x86_64.zip -o protoc3.zip && \
    unzip protoc3.zip -d protoc3 && \
    mv protoc3/bin/protoc /bin/protoc && \
    chmod a+rx /bin/protoc && \
    mv protoc3/include/google /usr/include/ && \
    chmod -R a+Xr /usr/include/google && \
    rm -rf protoc3.zip protoc3

# Copy just the go.mod files to download the golang deps from internet.
# This step would allow us to cache the downloads in a layer, and prevent
# reaching out to the internet unless any of the go.mod file is changed.
COPY gomod /gomod
RUN mkdir -p /gomod/src/modules
RUN for module in `ls -d /gomod/src/magma/**`; do cd ${module}/cloud/go && echo "${module}/cloud/go" && go mod download; done
RUN find /gomod/src/modules -maxdepth 2 -mindepth 2 -type d -exec bash -c "cd {}/cloud/go && go mod download" \;

# Copy the configs
COPY configs /etc/magma/configs

# Copy the code
ENV MAGMA_ROOT /src/magma
COPY src /src
RUN mkdir -p /src/modules
WORKDIR /src/magma/orc8r/cloud
RUN echo export INTERNAL_MAGMA_MODULES="\"`ls -d /src/magma/** | tr '\n' ' '`\"" >> /etc/profile.d/env.sh
RUN echo export EXTERNAL_MAGMA_MODULES="\"`find /src/modules -maxdepth 2 -mindepth 2 -type d | tr '\n' ' '`\"" >> /etc/profile.d/env.sh
#RUN . /etc/profile.d/env.sh && echo export MAGMA_MODULES="\"$INTERNAL_MAGMA_MODULES $EXTERNAL_MAGMA_MODULES\"" >> /etc/profile.d/env.sh
RUN . /etc/profile.d/env.sh && echo export MAGMA_MODULES="\"${MAGMA_ROOT}/orc8r ${MAGMA_ROOT}/lte\"" >> /etc/profile.d/env.sh
RUN . /etc/profile.d/env.sh && make tools

# ------------------------------------------------------------------------------
# Builder image with binaries and plugins
# ------------------------------------------------------------------------------
FROM base as builder

RUN . /etc/profile.d/env.sh && make build

# added for brokerd utelco
# RUN . /etc/profile.d/env.sh && make build_python

# -----------------------------------------------------------------------------
# Go-cache base image
# -----------------------------------------------------------------------------
FROM ubuntu:xenial as gocache
COPY --from=builder /root/.cache /root/.cache

# ------------------------------------------------------------------------------
# Production image
# ------------------------------------------------------------------------------
# FROM ubuntu:xenial
FROM base 
# added for brokerd utelco
RUN . /etc/profile.d/env.sh && make build_python

# Install the runtime deps from apt
RUN apt-get update && \
    apt-get install -y daemontools netcat openssl supervisor wget unzip

ARG CNTLR_FILES=src/magma/orc8r/cloud/docker/controller

# Setup Swagger UI
COPY ${CNTLR_FILES}/setup_swagger_ui /usr/local/bin/setup_swagger_ui
RUN /usr/local/bin/setup_swagger_ui

# Create an empty envdir for overriding in production
RUN mkdir -p /var/opt/magma/envdir
# TODO: automatically read the services from the plugins
#env controller_services streamer,subscriberdb,policydb,metricsd,certifier,bootstrapper,accessd,obsidian,dispatcher,directoryd,feg_relay,health,vpnservice,state,device,analytics,tenants
env controller_services streamer,subscriberdb,brokerd,policydb,metricsd,certifier,bootstrapper,accessd,obsidian,dispatcher,directoryd,state,device,tenants
# TODO: fix dispatcher to work on containers
ENV SERVICE_HOST_NAME localhost
# TODO: fix metricsd for fetching host level metrics
ENV HOST_NAME dev

COPY src/magma/orc8r/cloud/docker/wait-for-it.sh /usr/local/bin

# Copy the configs
COPY configs /etc/magma/configs

# Copy the build artifacts
COPY --from=builder /build/bin /var/opt/magma/bin
COPY --from=builder /build/plugins /var/opt/magma/plugins
COPY --from=builder /${CNTLR_FILES}/apidocs /var/opt/magma/static/apidocs

# Copy the key files
COPY src/magma/key_files /var/opt/magma/key_files

# Copy the scripts from the context
COPY ${CNTLR_FILES}/create_test_controller_certs /usr/local/bin/create_test_controller_certs

# Copy the supervisor configs
COPY ${CNTLR_FILES}/supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY ${CNTLR_FILES}/supervisor_logger.py /usr/local/lib/python2.7/dist-packages/supervisor_logger.py
CMD ["/usr/bin/supervisord"]
