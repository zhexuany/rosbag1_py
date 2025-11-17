#!/usr/bin/env python3
"""
Setup script for rosbag1_py package.
Builds C++ extension using CMake and pybind11.
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.build_py import build_py


class CMakeExtension(Extension):
    """CMake extension for setuptools."""
    
    def __init__(self, name, sourcedir=''):
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
        cmake_args = [
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}',
            f'-DPYTHON_EXECUTABLE={sys.executable}',
            '-DCMAKE_BUILD_TYPE=Release',
            # Skip building tests during pip install (tests are built separately)
            '-DBUILD_TESTING=OFF',
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
    author='rosbag1_py contributors',
    python_requires='>=3.6',
    packages=['rosbag1_py', 'rosbag1_py.compatibility', 'rosbag1_py.services', 'rosbag1_py.utils'],
    ext_modules=[CMakeExtension('rosbag1_py_cpp', '.')],
    cmdclass={'build_ext': CMakeBuild},
    install_requires=[
        'pybind11>=2.10.0',
    ],
    extras_require={
        'dev': [
            'pytest>=6.0',
            'pytest-cov',
        ],
    },
    zip_safe=False,
)

