#!/usr/bin/env python3
# Copyright 2025 Zhexuan Yang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Setup script for rosbag1_py package.
Builds C++ extension using CMake and pybind11.
"""

import os
import sys
import subprocess
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    """CMake extension for setuptools."""
    
    def __init__(self, name, sourcedir='.'):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    """Custom build_ext to run CMake."""
    
    def run(self):
        """Run CMake build."""
        try:
            _ = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build rosbag1_py")
        
        for ext in self.extensions:
            self.build_extension(ext)
    
    def build_extension(self, ext):
        """Build the extension using CMake."""
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        
        # Ensure directory exists
        os.makedirs(self.build_temp, exist_ok=True)
        
        # CMake configure
        # Allow BUILD_TESTING to be controlled via environment variable
        build_testing = os.environ.get('BUILD_TESTING', 'OFF')
        cmake_args = [
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}',
            f'-DPYTHON_EXECUTABLE={sys.executable}',
            '-DCMAKE_BUILD_TYPE=Release',
            f'-DBUILD_TESTING={build_testing}',
            # Workaround for old system googletest compatibility issue
            '-DCMAKE_POLICY_VERSION_MINIMUM=3.5',
        ]
        
        # Source ROS1 environment
        ros_distro = os.environ.get('ROS_DISTRO', 'noetic')
        if ros_distro:
            # Try to source ROS setup.bash
            ros_setup = f'/opt/ros/{ros_distro}/setup.bash'
            if os.path.exists(ros_setup):
                # Export ROS environment variables
                env = os.environ.copy()
                result = subprocess.run(
                    f'source {ros_setup} && env',
                    shell=True,
                    capture_output=True,
                    text=True
                )
                for line in result.stdout.splitlines():
                    if '=' in line:
                        key, value = line.split('=', 1)
                        env[key] = value
                os.environ.update(env)
        
        build_args = ['--config', 'Release']
        
        subprocess.check_call(
            ['cmake', ext.sourcedir] + cmake_args,
            cwd=self.build_temp
        )
        subprocess.check_call(
            ['cmake', '--build', '.'] + build_args,
            cwd=self.build_temp
        )


# Read README if it exists
readme_file = Path(__file__).parent / 'README.md'
long_description = ''
if readme_file.exists():
    long_description = readme_file.read_text()

setup(
    name='rosbag1_py',
    version='1.0.0',
    description='Production-ready Python API for ROS1 bag recording with MCAP support',
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='Zhexuan Yang',
    python_requires='>=3.6',
    packages=['rosbag1_py', 'rosbag1_py.compatibility', 'rosbag1_py.services', 'rosbag1_py.utils'],
    ext_modules=[CMakeExtension('rosbag1_py.rosbag1_py_cpp')],
    cmdclass={'build_ext': CMakeBuild},
    zip_safe=False,
)

