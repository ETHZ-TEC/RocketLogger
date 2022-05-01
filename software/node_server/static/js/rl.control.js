/**
 * Copyright (c) 2020, ETH Zurich, Computer Engineering Group
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

"use strict";

// check RocketLogger base functionality is loaded
if (typeof rl === 'undefined') {
    throw Error('need to load rl.base.js before loading rl.control.js');
}

/// RocketLogger channel names
const RL_CHANNEL_NAMES = ['V1', 'V2', 'V3', 'V4', 'I1L', 'I1H', 'I2L', 'I2H'];
/// RocketLogger force range channel names
const RL_CHANNEL_FORCE_NAMES = ['I1H', 'I2H'];
/// RocketLogger data file formats
const RL_FILE_FORMATS = ['rld', 'csv'];

/// initialize RocketLogger control functionality
function rocketlogger_init_control() {
    // check RocketLogger base functionality is initialized
    if (typeof rl.status === 'undefined') {
        throw Error('need RocketLogger base functionality to be initialized first.');
    }

    // init config with reset default
    rl._data.default_config = null;

    // provide start(), stop() and config(), reset(), reboot(), and poweroff() request methods
    rl.start = () => {
        const req = {
            cmd: 'start',
            config: rl._data.config,
        };
        rl._data.socket.emit('control', req);
        rl._data.reset = true;
    };
    rl.stop = () => {
        const req = {
            cmd: 'stop',
            config: null,
        };
        rl._data.socket.emit('control', req);
    };
    rl.config = (set_default) => {
        const req = {
            cmd: 'config',
            config: rl._data.config,
            default: set_default,
        };
        rl._data.socket.emit('control', req);
    };
    rl.reset = (key) => {
        const req = {
            cmd: 'reset',
            key: key,
            config: null,
        };
        rl._data.socket.emit('control', req);
    };
    rl.reboot = (key) => {
        const req = {
            cmd: 'reboot',
            key: key,
            config: null,
        };
        rl._data.socket.emit('control', req);
    };
    rl.poweroff = (key) => {
        const req = {
            cmd: 'poweroff',
            key: key,
            config: null,
        };
        rl._data.socket.emit('control', req);
    };

    // init config update callback
    rl._data.socket.on('control', (res) => {
        console.log(`rl control: ${JSON.stringify(res)}`);
        const cmd = res.req.cmd;
        /// @todo handle control feedback
        if (cmd === 'start') {
            rl._data.config = res.config;
        } else if (cmd === 'stop') {
            // no actions
        } else if (cmd === 'config') {
            if (rl._data.status.sampling === true) {
                console.log('skip processing default config, measurement is currently active');
                return;
            }

            const init = (rl._data.default_config === null);
            rl._data.default_config = res.config;
            config_reset_default();

            // indicate update except on initial load
            if (!init) {
                if (res.default) {
                    show('#alert_config_saved');
                } else {
                    show('#alert_config_loaded');
                }
            }
        }
    });
}

/// get configuration reset value
function config_get_reset() {
    const config = {
        /// ambient sensor enable
        ambient_enable: false,
        /// channels enable
        channel_enable: RL_CHANNEL_NAMES,
        /// channels force high range
        channel_force_range: [],
        /// digital input enable
        digital_enable: true,
        /// file storage, file config structure or null to disable
        file: {
            /// file header comment
            comment: 'Sampled using the RocketLogger web interface.',
            /// filename
            filename: 'data.rld',
            /// file format
            format: 'rld',
            /// maximum size before split in multiple files (0 disable)
            size: 1e9,
        },
        /// sample rate
        sample_rate: 1000,
        /// web interface data enable
        web_enable: true,
    };
    return config;

}

/// update RocketLogger config on interface changes
function config_change() {
    // new configuration structure
    const config = {
        ambient_enable: document.querySelector('#ambient_enable').checked,
        channel_enable: [],
        channel_force_range: [],
        digital_enable: document.querySelector('#digital_enable').checked,
        file: null,
        sample_rate: document.querySelector('#sample_rate').value,
        web_enable: document.querySelector('#web_enable').checked,
    }

    // get channel config
    for (const ch of RL_CHANNEL_NAMES) {
        if (document.getElementById(`channel_${ch.toLowerCase()}_enable`).checked) {
            config.channel_enable.push(ch);
        }
    }

    // get force channel range config
    for (const ch of RL_CHANNEL_FORCE_NAMES) {
        if (document.getElementById(`channel_${ch.toLowerCase()}_force`).checked) {
            config.channel_force_range.push(ch);
        }
    }

    // get file config
    if (document.querySelector('#file_enable').checked) {
        config.file = {
            comment: document.querySelector('#file_comment').value,
            filename: document.querySelector('#file_filename').value,
            format: document.querySelector('#file_format').value,
            size: 0,
        }
        if (document.querySelector('#file_split').checked) {
            config.file.size = document.querySelector('#file_size').value * document.querySelector('#file_size_scale').value;
        }
        adjust_filename_extension(config.file);
        document.querySelector('#file_filename').value = config.file.filename;
    }

    // update stored config
    rl._data.config = config;

    // perform necessary interface updates
    hide('#alert_config_saved');
    hide('#alert_config_loaded');
    document.querySelector('#file_group').disabled = config.file === null;
    document.querySelector('#file_split_group').disabled = config.file && (config.file.size === 0);
    document.querySelector('#web_group').disabled = !config.web_enable;
    const configCollapse = new bootstrap.Collapse(document.querySelector('#collapseConfiguration'), { toggle: false });
    config.web_enable ? configCollapse.show() : configCollapse.hide();
    config.ambient_enable ? show('#plot_group_ambient') : hide('#plot_group_ambient');
    config.digital_enable ? show('#plot_group_digital') : hide('#plot_group_digital');
    config.channel_enable.some(v => v[0] === 'V') ? show('#plot_group_voltage') : hide('#plot_group_voltage');
    config.channel_enable.some(v => v[0] === 'I') ? show('#plot_group_current') : hide('#plot_group_current');

    // estimate remaining time from configuration
    let use_rate_estimated = (config.channel_enable.length +
        (config.digital_enable ? 1 : 0)) * 4 * config.sample_rate;
    if (use_rate_estimated > 0 && config.file !== null) {
        document.querySelector('#status_remaining').innerText =
            `~ ${unix_to_timespan_string(rl._data.status.disk_free_bytes / use_rate_estimated)}`;
    } else {
        document.querySelector('#status_remaining').innerText = 'indefinite';
    }
}

function adjust_filename_extension(file_config) {
    if (has_rl_extension(file_config.filename)) {
        const pos = file_config.filename.lastIndexOf('.');
        file_config.filename = file_config.filename.substr(0, pos);
    }
    file_config.filename = file_config.filename + '.' + file_config.format;
}

function has_rl_extension(filename) {
    return RL_FILE_FORMATS.some(
        ext => filename.toLowerCase().endsWith('.' + ext));
}

/// update configuration interface to RocketLogger default configuration
function config_reset_default() {
    if (rl._data.default_config === null) {
        throw Error('undefined RocketLogger default configuration.');
    }
    const config = rl._data.default_config;

    // set direct value inputs
    document.querySelector('#ambient_enable').checked = config.ambient_enable;
    document.querySelector('#digital_enable').checked = config.digital_enable;
    document.querySelector('#sample_rate').value = config.sample_rate;
    document.querySelector('#web_enable').checked = config.web_enable;

    // set channel switches
    for (const ch of RL_CHANNEL_NAMES) {
        document.getElementById(`channel_${ch.toLowerCase()}_enable`).checked =
            config.channel_enable.indexOf(ch) >= 0;
    }

    // set force channel range config
    for (const ch of RL_CHANNEL_FORCE_NAMES) {
        document.getElementById(`channel_${ch.toLowerCase()}_force`).checked =
            config.channel_force_range.indexOf(ch) >= 0;
    }

    // set file config inputs
    if (config.file === null) {
        document.querySelector('#file_enable').checked = false;
    } else {
        document.querySelector('#file_enable').checked = true;
        document.querySelector('#file_comment').value = config.file.comment;
        document.querySelector('#file_filename').value = config.file.filename;
        document.querySelector('#file_format').value = config.file.format;

        // set file split and size config
        document.querySelector('#file_split').checked = config.file.size > 0;
        if (config.file.size === 0) {
            config.file.size = RL_CONFIG_RESET.file.size;
        } else if (config.file.size < 1e6) {
            alert('configured file size too small, reseting to default');
            config.file.size = RL_CONFIG_RESET.file.size;
        }
        if (config.file.size >= 1e9) {
            document.querySelector('#file_size').value = Math.round(config.file.size / 1e9);
            document.querySelector('#file_size_scale').value = '1000000000';
        } else {
            document.querySelector('#file_size').value = Math.round(config.file.size / 1e6);
            document.querySelector('#file_size_scale').value = '1000000';
        }
    }

    // trigger configuration interface update handler
    config_change();
}

/// enable all measurement channels without forcing high range
function config_channels_enable() {
    // enable channel switches
    for (const ch of RL_CHANNEL_NAMES) {
        document.getElementById(`channel_${ch.toLowerCase()}_enable`).checked = true;
    }

    // unset force channel range
    for (const ch of RL_CHANNEL_FORCE_NAMES) {
        document.getElementById(`channel_${ch.toLowerCase()}_force`).checked = false;
    }
    document.querySelector('#digital_enable').checked = true;
    document.querySelector('#ambient_enable').checked = true;

    // trigger configuration interface update handler
    config_change();
}

/// disable all measurement channels
function config_channels_disable() {
    // disable channels
    for (const ch of RL_CHANNEL_NAMES) {
        document.getElementById(`channel_${ch.toLowerCase()}_enable`).checked = false;
    }

    // unset force channel range
    for (const ch of RL_CHANNEL_FORCE_NAMES) {
        document.getElementById(`channel_${ch.toLowerCase()}_force`).checked = false;
    }
    document.querySelector('#digital_enable').checked = false;
    document.querySelector('#ambient_enable').checked = false;

    // trigger configuration interface update handler
    config_change();
}

/// add date prefix to filename if not existing
function config_file_add_prefix() {
    const date_pattern = /\d{8}_\d{6}_/;

    // check filename for existing prefix
    let filename = document.querySelector('#file_filename').value;
    if (date_pattern.test(filename)) {
        filename = filename.slice(16);
    }
    document.querySelector('#file_filename').value = `${date_to_prefix_string(new Date())}_${filename}`;

    // trigger configuration interface update handler
    config_change();
}

/// initialize when document is fully loaded
window.addEventListener('load', () => {
    // initialize RocketLogger control functionality
    rocketlogger_init_control();

    // initialize configuration interface update handler
    document.querySelector('#configuration_group').addEventListener('change', config_change);

    // initialize configuration interface helper action buttons
    document.querySelector('#button_config_all').addEventListener('click', () => {
        config_channels_enable();
    });
    document.querySelector('#button_config_none').addEventListener('click', () => {
        config_channels_disable();
    });
    document.querySelector('#button_file_prefix').addEventListener('click', () => {
        config_file_add_prefix();
    });

    // initialize measurement control buttons
    document.querySelector('#button_start').addEventListener('click', () => {
        rl.start();
    });
    document.querySelector('#button_stop').addEventListener('click', () => {
        rl.stop();
    });

    // initialize system operation control buttons
    document.querySelector('#button_reset').addEventListener('click', () => {
        const res = prompt(
            'Are you sure to reset the RocketLogger hardware?\n' +
            'Enter "reset" to confirm and proceed:',
            '');
        if (res) {
            rl.reset(res.trim().toLowerCase());
        }
    });
    document.querySelector('#button_reboot').addEventListener('click', () => {
        const res = prompt(
            'Are you sure to reboot the RocketLogger system?\n' +
            'Enter "reboot" to confirm and proceed:',
            '');
        if (res) {
            rl.reboot(res.trim().toLowerCase());
        }
    });
    document.querySelector('#button_poweroff').addEventListener('click', () => {
        const res = prompt(
            'Are you sure to shut down the RocketLogger system?\n' +
            'Enter "shutdown" to confirm and proceed:',
            '');
        if (res) {
            rl.poweroff(res.trim().toLowerCase() === 'shutdown' ? 'poweroff' : '');
        }
    });

    // initialize default configuration control buttons
    document.querySelector('#button_config_save').addEventListener('click', () => {
        hide('#alert_config_saved');
        hide('#alert_config_loaded');
        rl.config(true);
    });
    document.querySelector('#button_config_load').addEventListener('click', () => {
        hide('#alert_config_saved');
        hide('#alert_config_loaded');
        rl.config();
    });

    // register control hotkeys
    document.addEventListener('keydown', (event) => {
        // skip event of form inputs
        if (event.target.nodeName === 'INPUT' || event.target.nodeName === 'CHECKBOX' ||
            event.target.nodeName === 'TEXTAREA') {
            return;
        }
        // skip event with pressed modifier keys
        if (event.altKey || event.ctrlKey || event.metaKey || event.shiftKey) {
            return;
        }

        switch (event.key) {
            case 's':
                if (document.querySelector('#button_start').disabled) {
                    document.querySelector('#button_stop').click();
                } else {
                    document.querySelector('#button_start').click();
                }
                break;
            case 'd':
                document.querySelector('#button_config_save').click();
                break;
            case 'l':
                document.querySelector('#button_config_load').click();
                break;
            default:
                return;
        }
        event.preventDefault();
    });

    // update configuration interface and trigger load config
    rl.config();
});
