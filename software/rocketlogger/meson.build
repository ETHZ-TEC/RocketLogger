# Meson build configuration.
#
# Copyright (c) 2020-2021, Lukas Sigrist <lsigrist@mailbox.org>
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

project('rocketlogger', 'c',
    version : '1.99',
    default_options : ['buildtype=debugoptimized',
        'c_std=c11', 'cpp_std=c++11', 'warning_level=3',
        'prefix=/usr', 'install_umask=0022'],
    license : 'BSD-3-Clause')


## subprojects
libprussdrv_proj = subproject('libprussdrv')
libprussdrv_dep = libprussdrv_proj.get_variable('libprussdrv_dep')
dt_overlay_proj = subproject('bb-overlays')
dt_overlay_dep = dt_overlay_proj.get_variable('bboverlays_dep')


## program executables
prog_python = import('python').find_installation('python3')
compiler_cc = meson.get_compiler('c')
compiler_pru = find_program('clpru')
linker_pru = find_program('lnkpru')
objcopy_pru = find_program('hexpru')
compiler_dt = find_program('dtc')


## library dependencies
libi2c_dep = dependency('libi2c', required : false)
if not libi2c_dep.found()
    libi2c_dep = compiler_cc.find_library('libi2c', required : false)
    if not libi2c_dep.found()
        warning('manually adding libi2c linker flag')
        add_project_link_arguments('-li2c', language : 'c')
    endif
endif

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
    language : 'c')


## linker options
pru_linker_script = 'pru/AM335x_PRU.cmd'


## generated dependencies
version_c = custom_target('version.c',
    output : 'version.c',
    input : 'version_gen.py',
    command : [prog_python, '@INPUT@', '@OUTPUT@'],
    build_always_stale: true,
)


## sources
common_src = [
    'sensor/bme280.c',
    'sensor/sensor.c',
    'sensor/tsl4531.c',
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
binary_src = [
    'rocketlogger.c',
]
daemon_src = [
    'rocketloggerd.c',
]
pru_src = [
    'pru/rocketlogger.asm',
]
dt_overlay_src = [
    'overlay/ROCKETLOGGER.dts',
]


## targets
binary_exe = executable('rocketlogger', binary_src + common_src,
    dependencies: binary_deps + common_deps,
    install : true)
daemon_exe = executable('rocketloggerd', daemon_src + common_src,
    dependencies: daemon_deps + common_deps,
    install : true)
# @todo: configure sticky bit and install systemd service configuration

# custom PRU targets
pru_firmware_obj = custom_target('rocketlogger.asm.o',
    output : 'rocketlogger.asm.o',
    input : 'pru/rocketlogger.asm',
    command : [compiler_pru, '--silicon_version=3', '--asm_listing', '--output_file=@OUTPUT@', '@INPUT0@'])
pru_firmware_out = custom_target('rocketlogger.out',
    output : 'rocketlogger.out',
    input : [pru_linker_script, pru_firmware_obj],
    command : [linker_pru, '--output_file=@OUTPUT@', '@INPUT@'])
pru_firmware_bin = custom_target('rocketlogger.bin',
    output : 'rocketlogger.bin',
    input : pru_firmware_out,
    command : [objcopy_pru, '--binary', '--outfile=@OUTPUT@', '@INPUT@'],
    install : true,
    install_dir : '/lib/firmware')
# @todo: install shared memory size configuration file

# custom device tree overlay target
dt_overlay_tmp = custom_target('ROCKETLOGGER.dts.tmp',
    output : 'ROCKETLOGGER.dts.tmp',
    input : 'overlay/ROCKETLOGGER.dts',
    command : ['cc', '-E', '-nostdinc', '-undef', '-x', 'assembler-with-cpp', '-D__DTS__',
        '-I../subprojects/bb.org-overlays-master/include',
        '-o', '@OUTPUT@', '@INPUT@'])
# @todo: derive include path from bb_overlay_dep dependency

dt_overlay_bin = custom_target('ROCKETLOGGER.dtbo',
    output : 'ROCKETLOGGER.dtbo',
    input : dt_overlay_tmp,
    command : [compiler_dt, '--out-format=dtb', '-@', '--out=@OUTPUT@', '@INPUT@'],
    install : true,
    install_dir : '/lib/firmware')
# @todo: install uEnv.txt patches
