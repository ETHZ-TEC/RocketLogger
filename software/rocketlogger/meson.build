# Meson build configuration.
#
# Copyright (c) 2020, Lukas Sigrist <lsigrist@mailbox.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

project('rocketlogger', 'c', 'cpp',
        version : '1.99',
        default_options : ['buildtype=debugoptimized',
            'c_std=c11', 'cpp_std=c++11', 'warning_level=3',
            'prefix=/usr', 'install_umask=0022'],
        license : 'BSD-3-Clause')

## subprojects
libprussdrv_proj = subproject('libprussdrv')
libprussdrv_dep = libprussdrv_proj.get_variable('libprussdrv_dep')


## program executables
prog_python = import('python').find_installation('python3')
compiler_cc = meson.get_compiler('c')
compiler_cxx = meson.get_compiler('cpp')


# cross compilation for Beaglebone Black
# SoC: AM335x 1GHz ARM® Cortex-A8 (ARM v7-a with NEON accelerator)

## libraries
libi2c_dep = dependency('libi2c', required : false)
if not libi2c_dep.found()
    libi2c_dep = compiler_cc.find_library('libi2c', required: false)
    if not libi2c_dep.found()
        warning('manually adding libi2c linker flag')
        add_project_link_arguments('-li2c', language: ['c', 'cpp'])
    endif
endif

# libprussdrv_dep = dependency('libprussdrv', required : false)
# if not libprussdrv_dep.found()
#     warning('manually adding libprussdrv linker flag')
#     add_project_link_arguments('-lprussdrv', language: ['c', 'cpp'])
#     # libprussdrv_proj = subproject('libprussdrv')
#     # libprussdrv_dep = libprussdrv_proj.get_variable('libprussdrv_dep')
# endif

common_deps = [
    dependency('ncurses'),
    dependency('libzmq'),
    libi2c_dep,
    libprussdrv_dep
]
binary_deps = []
daemon_deps = []


## compiler options
add_project_arguments('-D_LARGEFILE64_SOURCE',
                      language: ['c', 'cpp'])


## linker options


## generated dependencies
version_c = custom_target(
    'version.c',
    output : 'version.c',
    input : 'version_gen.py',
    command : [prog_python, '@INPUT@', '@OUTPUT@'],
    build_always_stale: true,
)


## sources
common_src = [
    'calibration.c',
    'gpio.c',
    'log.c',
    'meter.c',
    'pru.c',
    'pwm.c',
    'rl_file.c',
    'rl_hw.c',
    'rl_lib.c',
    'rl_socket.c',
    'rl.c',
    'sem.c',
    'sysfs.c',
    'util.c',
    version_c,
]
sensor_src = [
    'sensor/bme280.c',
    'sensor/sensor.c',
    'sensor/tsl4531.c',
]
binary_src = [
    'rocketlogger.c',
]
daemon_src = [
    'rocketloggerd.c',
]


## targets
binary_exe = executable('rocketlogger', binary_src + common_src + sensor_src,
                        dependencies: binary_deps + common_deps,
                        install : true, install_dir : '/usr/bin')
daemon_exe = executable('rocketloggerd', daemon_src + common_src + sensor_src,
                        dependencies: daemon_deps + common_deps,
                        install : true, install_dir : '/usr/bin')
