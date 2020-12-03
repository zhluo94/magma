"""
Copyright (c) 2016-present, Facebook, Inc.
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree. An additional grant
of patent rights can be found in the PATENTS file in the same directory.
"""

import os

from setuptools import setup

# We can use an environment variable to pass in the package version during
# build. Since we don't distribute this on its own, we don't really care what
# version this represents. 'None' defaults to 0.0.0.
VERSION = os.environ.get('PKG_VERSION', None)

setup(
    name='lte_cloud',
    version=VERSION,
    packages=[
        'magma.common',
        'magma.common.health',
        'magma.common.redis',
        'magma.configuration',
        'magma.subscriberdb',
        'magma.subscriberdb.crypto',
        'magma.subscriberdb.protocols',
        'magma.subscriberdb.protocols.diameter',
        'magma.subscriberdb.protocols.diameter.application',
        'magma.subscriberdb.store',
        'magma.brokerd',
    ],
    install_requires=[
        'Cython>=0.29.1',
        'pystemd==0.5.0',
        'docker>=4.0.2',
  #      'fire>=0.2.0',
        'glob2>=0.7',
        'aioh2==0.2.2',
        'redis>=2.10.5',  # redis-py (Python bindings to redis)
        'redis-collections>=0.4.2',
        'aiohttp>=0.17.2',
        'grpcio==1.16.1',
        'protobuf==3.6.1',
  #     'Jinja2>=2.8',
  #      'netifaces>=0.10.4',
  #      'pylint>=1.7.1',
        'PyYAML>=3.12',
  #      'pytz>=2014.4',
        'prometheus_client==0.3.1',
        'snowflake>=0.0.3',
        'psutil==5.2.2',
        'cryptography>=1.9',
        'systemd-python>=234',
   #     'itsdangerous>=0.24',
   #     'click>=5.1',
   #     'pycares>=2.3.0',
        'python-dateutil>=1.4',
        # force same requests version as lte/gateway/python/setup.py
        'requests==2.22.0',
    #    'jsonpickle',
    #    'bravado-core==5.16.1',
        'jsonschema==3.1.0',
        "strict-rfc3339>=0.7",
        "rfc3987>=1.3.0",
        "jsonpointer>=1.13",
    #    "webcolors>=1.11.1",
        "M2Crypto",
        'pycrypto>=2.6.1',
    ],
    extras_require={
        'dev': [
        ],
    },
)
