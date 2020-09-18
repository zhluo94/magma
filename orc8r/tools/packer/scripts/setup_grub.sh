#!/bin/bash -eux
################################################################################
# Copyright (c) Facebook, Inc. and its affiliates.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.
################################################################################

# set 4.9.0-9-amd64 to be the default image
sed -i 's@.*GRUB_DEFAULT=0.*@GRUB_DEFAULT="Advanced options for Debian GNU/Linux>Debian GNU/Linux, with Linux 4.9.0-9-amd64"@' /etc/default/grub
# persist it
update-grub