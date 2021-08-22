project('rocketlogger', 'c',
    version : '2.0.0',
    default_options : ['buildtype=debugoptimized',
        'c_std=c11', 'cpp_std=c++11', 'warning_level=3',
        'prefix=/usr', 'install_umask=0022'],
    license : 'BSD-3-Clause')


## subprojects
libprussdrv_proj = subproject('libprussdrv')
libprussdrv_dep = libprussdrv_proj.get_variable('libprussdrv_dep')
bb_overlay_proj = subproject('bb-overlays')
bb_overlay_dep = bb_overlay_proj.get_variable('bboverlays_dep')
dt_overlay_dep = bb_overlay_dep.partial_dependency(includes : true)


## program executables
compiler_cc = meson.get_compiler('c')
compiler_dt = find_program('dtc')
compiler_pru = find_program('clpru')
linker_pru = find_program('lnkpru')
objcopy_pru = find_program('hexpru')


## library dependencies
common_deps = [
    dependency('ncurses'),
    dependency('libzmq'),
    libprussdrv_dep
]
binary_deps = []
daemon_deps = []


## compiler options
add_project_arguments('-D_LARGEFILE64_SOURCE',
    language : 'c')


## linker options
pru_linker_script = 'pru/AM335x_PRU.cmd'


## generated files
version_data = configuration_data({
    'version' : meson.project_version(),
    'git_description' : run_command(['git', 'describe', '--tags', '--dirty']).stdout().strip(),
    'git_date' : run_command(['git', 'show', '--no-patch', '--format=%ci', 'HEAD']).stdout().strip(),
    'compile_date' : run_command(['date', '--rfc-3339=seconds']).stdout().strip(),
})
version_c = configure_file(input : 'version.c.in',
    output : 'version.c',
    configuration : version_data)


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

## configuration files
default_calibration_file = [
    'config/calibration.dat',
]
systemd_service_src = [
    'config/rocketlogger.service',
]
pru_module_config_src = [
    'config/rocketlogger.conf',
]


## targets
# system executables and services
binary_exe = executable('rocketlogger', binary_src + common_src,
    dependencies: binary_deps + common_deps,
    install : true,
    install_mode : ['rwsr-xr-x', 0, 0])
daemon_exe = executable('rocketloggerd', daemon_src + common_src,
    dependencies: daemon_deps + common_deps,
    install : true,
    install_mode : ['rwsr-xr-x', 0, 0])
install_data(systemd_service_src,
    install_dir : get_option('sysconfdir') / 'systemd' / 'system',
    install_mode : ['rw-r--r--', 0, 0])
install_data(default_calibration_file,
    install_dir : get_option('sysconfdir') / 'rocketlogger',
    install_mode : ['rw-r--r--', 0, 0])

# custom PRU targets
pru_firmware_obj = custom_target('rocketlogger.asm.o',
    output : 'rocketlogger.asm.o',
    input : pru_src,
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
install_data(pru_module_config_src,
    install_dir : get_option('sysconfdir') / 'modprobe.d',
    install_mode : ['rw-r--r--', 0, 0])

# custom device tree overlay target
dt_overlay_tmp = custom_target('ROCKETLOGGER.dts.tmp',
    output : 'ROCKETLOGGER.dts.tmp',
    input : dt_overlay_src,
    command : compiler_cc.cmd_array() + ['-E', '-nostdinc', '-undef', '-x', 'assembler-with-cpp', '-D__DTS__',
        '-I../subprojects/bb.org-overlays-28cb22447cefc9824e2c8288e59b36d46e4f5422/include',  # @todo: derive include path from subproject
        '-o', '@OUTPUT@', '@INPUT@'])
dt_overlay_bin = custom_target('ROCKETLOGGER.dtbo',
    output : 'ROCKETLOGGER.dtbo',
    input : dt_overlay_tmp,
    command : [compiler_dt, '--out-format=dtb', '-@', '--out=@OUTPUT@', '@INPUT@'],
    install : true,
    install_dir : '/lib/firmware')

# post install configuration of systemd service and device tree overlay
meson.add_install_script('install.sh')
