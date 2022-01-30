"use strict";

import * as path from 'path';
import { promisify } from 'util';
import { exec as exec_, spawn } from 'child_process';
import { get_data_path } from './rl.files.js';

export { status, start, stop, reset, config, version };

const exec = promisify(exec_);


/// RocketLogger status and data update rate [in 1/s]
const rl_update_rate = 10;


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
        args.push(`--output=${get_data_path(config.file.filename)}`);
        args.push(`--format=${config.file.format}`);
        args.push(`--size=${config.file.size}`);
        args.push(`--comment=${config.file.comment.replace(/"/g, '')}`);
    }
    args.push(`--rate=${config.sample_rate}`);
    args.push(`--web=${config.web_enable}`);

    return args;
}
