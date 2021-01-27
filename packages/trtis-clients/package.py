# Copyright 2013-2020 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack import *
import os

class TrtisClients(CMakePackage):
    """C++ client code for Triton Inference Server."""

    # FIXME: Add a proper url for your package's homepage here.
    homepage = "https://github.com/triton-inference-server/server"
    url      = "https://github.com/triton-inference-server/server/archive/v2.6.0.tar.gz"

    # maintainers = ['github_user1', 'github_user2']

    version('2.6.0',                    sha256='c4fad25c212a0b5522c7d65c78b2f25ab0916ccf584ec0295643fec863cb403e')
    
    path('fix_compile_flags.patch')
    patch('use_existing.patch)'

    root_cmakelists_dir = 'build'

    # FIXME: Add dependencies if required.
    depends_on('cmake@3.18:', type='build')
    depends_on('py-grpcio', type='build')

  
    def cmake_args(self):
        args = [
            '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_C_COMPILER=cc',
            '-DCMAKE_CXX_COMPILER=c++',
            '-DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations"',
            '-DCMAKE_PREFIX_PATH=%s/share/OpenCV' % self.spec['opencv'].prefix,
            '-DTRTIS_ENABLE_GPU=OFF',
        ]
        return args
