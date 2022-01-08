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

import * as path from 'path';
import { promisify } from 'util';
import { exec as exec_, spawn } from 'child_process';

export { status, start, stop, reset, config, version, path_data, path_system_logfile, status_socket, data_socket, web_data_rate };

const exec = promisify(exec_);


/// RocketLogger status and data update rate [in 1/s]
const rl_update_rate = 10;

/// RocketLogger measurement data path
const path_data = '/home/rocketlogger/data';

/// RocketLogger measurement log file
const path_system_logfile = '/var/log/rocketlogger/rocketlogger.log';

/// ZeroMQ socket identifier for data publishing status
const status_socket = 'tcp://127.0.0.1:8276';

/// ZeroMQ socket identifier for status publishing
const data_socket = 'tcp://127.0.0.1:8277';

/// RocketLogger maximum web downstream data rate [in 1/s]
const web_data_rate = 1000;

/// get RocketLogger status
async function status() {
    const res = {
        err: [],
        warn: [],
        msg: [],
        status: null,
    };

    const cmd = 'rocketlogger status --json';
    const { stdout, stderr } = await exec(cmd, { timeout: 500 })
        .catch(err => {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        });

    try {
        res.status = JSON.parse(stdout);
    } catch (err) {
        res.err.push(`RocketLogger status processing error: ${err}`);
        res.err.push(`stdout: ${stdout}`);
        res.err.push(`stderr: ${stderr}`);
    }

    res.cli = cmd;
    return res;
}

/// start RocketLogger measurement
async function start(config) {
    const res = {
        err: [],
        warn: [],
        msg: [],
        config: null,
        start: null,
    };

    const args = config_to_args_list('start', config);
    const cmd = `rocketlogger ${args.join(' ')}`;
    const child_process = spawn('rocketlogger', args, {
        detached: true,
        stdio: 'ignore',
    });
    if (child_process.error) {
        res.err.push('RocketLogger binary was not found. ' +
            'Please check your system configuration!');
        return res;
    }

    res.config = config;
    res.cli = cmd;
    return res;
}

/// stop RocketLogger measurement
async function stop() {
    const res = {
        err: [],
        warn: [],
        msg: [],
        stop: null,
    };

    const cmd = 'rocketlogger stop';
    const { stdout, stderr } = await exec(cmd, { timeout: 500 })
        .catch(err => {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        });

    res.stop = stdout;
    res.cli = cmd;
    return res;
}

/// reset RocketLogger by restarting the rocketloggerd service
async function reset() {
    const res = {
        err: [],
        warn: [],
        msg: [],
        reset: null,
    };

    const cmd = 'sudo pkill rocketloggerd';
    const { stdout, stderr } = await exec(cmd, { timeout: 500 })
        .catch(err => {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        });

    res.reset = stdout;
    res.cli = cmd;
    return res;
}

/// load/store RocketLogger default configuration
async function config(config) {
    const res = {
        err: [],
        warn: [],
        msg: [],
        config: null,
        default: false,
    };

    const args = config_to_args_list('config', config);
    if (config) {
        args.push('--default');
    }
    const cmd = `rocketlogger ${args.join(' ')}`
    const { stdout, stderr } = await exec(cmd, { timeout: 500 })
        .catch(err => {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        });

    // parse and filter config
    try {
        res.config = JSON.parse(stdout);
        if (res.config.file) {
            res.config.file.filename = path.basename(res.config.file.filename);
        }
    } catch (err) {
        res.err.push(`RocketLogger configuration processing error: ${err}`);
        res.err.push(`stdout: ${stdout}`);
        res.err.push(`stderr: ${stderr}`);
    }

    res.default = (config != null);
    res.cli = cmd;
    return res;
}

/// get the RocketLogger CLI version
async function version() {
    const res = {
        err: [],
        warn: [],
        msg: [],
        version: null,
        version_string: null,
    };

    const cmd = 'rocketlogger --version';
    const { stdout, stderr } = await exec(cmd, { timeout: 500 })
        .catch(err => {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        });

    // parse RocketLogger version
    try {
        res.version_string = stdout;
        res.version = stdout.split('\n')[0].split(' ').reverse()[0];
    } catch (err) {
        res.err.push(`RocketLogger version processing error: ${err}`);
        res.err.push(`stdout: ${stdout}`);
        res.err.push(`stderr: ${stderr}`);
    }

    res.cli = cmd;
    return res;
}


/// get RocketLogger CLI arguments from JSON configuration
function config_to_args_list(mode, config) {
    const args = [mode, '--json'];
    if (config == null) {
        return args;
    }

    // force some defaults
    config.samples = 0;  // continuous sampling
    config.update_rate = rl_update_rate;
    if (config.sample_rate < config.update_rate) {
        config.update_rate = config.sample_rate;
    }

    args.push(`--samples=${config.samples}`);
    args.push(`--update=${config.update_rate}`);
    args.push(`--ambient=${config.ambient_enable}`);
    args.push(`--channel=${config.channel_enable.join(',')}`);
    args.push(`--high-range=${config.channel_force_range.join(',')}`);
    args.push(`--digital=${config.digital_enable}`);
    if (config.file == null) {
        args.push('--output=0');
    } else {
        args.push(`--output=${path.join(path_data, config.file.filename)}`);
        args.push(`--format=${config.file.format}`);
        args.push(`--size=${config.file.size}`);
        args.push(`--comment=${config.file.comment.replace(/"/g, '')}`);
    }
    args.push(`--rate=${config.sample_rate}`);
    args.push(`--web=${config.web_enable}`);

    return args;
}
