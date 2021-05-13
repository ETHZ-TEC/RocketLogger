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

const path = require('path');
const { spawn, spawnSync } = require('child_process');

/// RocketLogger status and data update rate [in 1/s]
const rl_update_rate = 10;
/// RocketLogger maximum rate of data [in 1/s]
const rl_web_data_rate = 1000;

const rl = {
    /// RocketLogger measurement data path
    path_data: '/home/rocketlogger/data',
    /// RocketLogger measurement log file
    path_system_logfile: '/var/log/rocketlogger.log',
    /// ZeroMQ socket identifier for data publishing status
    zmq_status_socket: 'tcp://127.0.0.1:8276',
    /// ZeroMQ socket identifier for status publishing
    zmq_data_socket: 'tcp://127.0.0.1:8277',
    /// web downstream data rate [in 1/s]
    web_data_rate: rl_web_data_rate,

    /// get RocketLogger status
    status: () => {
        const res = {
            err: [],
            warn: [],
            msg: [],
            status: null,
        };

        const args = ['status', '--json'];
        const cmd = spawnSync('rocketlogger', args, { timeout: 500 });
        if (cmd.error) {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        }

        try {
            res.status = JSON.parse(cmd.stdout.toString());
        } catch (err) {
            res.err.push(`RocketLogger configuration processing error: ${err}`);
        }

        res.cli = `rocketlogger ${args.join(' ')}`;
        return res;
    },

    /// start RocketLogger measurement
    start: (config) => {
        const res = {
            err: [],
            warn: [],
            msg: [],
            config: null,
            start: null,
        };

        const args = config_to_args('start', config);
        const cmd = spawn('rocketlogger', args, {
            detached: true,
            stdio: 'ignore',
        });
        if (cmd.error) {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        }

        res.start = cmd.status;
        res.config = config;
        res.cli = `rocketlogger ${args.join(' ')}`;
        return res;
    },

    /// stop RocketLogger measurement
    stop: () => {
        const res = {
            err: [],
            warn: [],
            msg: [],
            stop: null,
        };

        const args = ['stop'];
        const cmd = spawnSync('rocketlogger', args, { timeout: 500 });
        if (cmd.error) {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        }

        res.stop = cmd.status;
        res.cli = `rocketlogger ${args.join(' ')}`;
        return res;
    },

    /// reset RocketLogger by restarting the rocketloggerd service
    reset: () => {
        const res = {
            err: [],
            warn: [],
            msg: [],
            reset: null,
        };

        const args = ['pkill', 'rocketloggerd'];
        const cmd = spawnSync('sudo', args, { timeout: 500 });
        if (cmd.error) {
            res.err.push('Restarting RocketLogger service failed. ' +
                'Please check your system configuration!');
            return res;
        }

        res.reset = cmd.status;
        res.cli = `sudo ${args.join(' ')}`;
        return res;
    },

    /// load/store RocketLogger default configuration
    config: (config) => {
        const res = {
            err: [],
            warn: [],
            msg: [],
            config: null,
            default: false,
        };

        const args = config_to_args('config', config);
        if (config) {
            args.push('--default');
        }
        const cmd = spawnSync('rocketlogger', args, { timeout: 500 });
        if (cmd.error) {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        }

        // parse and filter config
        try {
            res.config = JSON.parse(cmd.stdout.toString());
            if (res.config.file) {
                res.config.file.filename = path.basename(res.config.file.filename);
            }
        } catch (err) {
            res.err.push(`RocketLogger configuration processing error: ${err}`);
        }

        res.default = (config != null);
        res.cli = `rocketlogger ${args.join(' ')}`;
        return res;
    },

    /// get the RocketLogger CLI version
    version: () => {
        const res = {
            err: [],
            warn: [],
            msg: [],
            version: null,
            version_string: null,
        };

        const args = ['--version'];
        const cmd = spawnSync('rocketlogger', args, { timeout: 500 });
        if (cmd.error) {
            res.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
            return res;
        } else {
            res.version_string = cmd.stdout.toString();
            res.version = res.version_string.split('\n')[0].split(' ').reverse()[0];
        }

        res.cli = `rocketlogger ${args.join(' ')}`;
        return res;
    },
};

module.exports = rl;

/// get RocketLogger CLI arguments from JSON configuration
function config_to_args(mode, config) {
    const args = [mode, '--json'];
    if (config == null) {
        return args;
    }

    // force some defaults
    config.samples = 0; // continuous sampling
    config.update_rate = rl_update_rate;

    args.push(`--samples=${config.samples}`);
    args.push(`--update=${config.update_rate}`);
    args.push(`--ambient=${config.ambient_enable}`);
    args.push(`--channel=${config.channel_enable.join(',')}`);
    args.push(`--high-range=${config.channel_force_range.join(',')}`);
    args.push(`--digital=${config.digital_enable}`);
    if (config.file == null) {
        args.push('--output=0');
    } else {
        args.push(`--output=${path.join(rl.path_data, config.file.filename)}`);
        args.push(`--format=${config.file.format}`);
        args.push(`--size=${config.file.size}`);
        args.push(`--comment=${config.file.comment.replace(/"/g, '')}`);
    }
    args.push(`--rate=${config.sample_rate}`);
    args.push(`--web=${config.web_enable}`);

    return args;
}