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
    const result = await cli_command('status --json');

    try {
        const status = JSON.parse(result);
        return { status: status };
    } catch (err) {
        throw Error(`RocketLogger status processing error: ${err} ` +
            `(stdout: ${result}; stderr: ${stderr})`);
    }
}

/// start RocketLogger measurement
async function start(config) {
    cli_start(config);
    return { config: config };
}

/// stop RocketLogger measurement
async function stop() {
    const result = await cli_command('stop');
    return { stop: result };
}

/// reset RocketLogger by restarting the service
async function reset() {
    const result = reset_service();
    return { reset: result };
}

/// load/store RocketLogger default configuration
async function config(config = null) {
    const args = config_to_args_list('config', config);
    if (config) {
        args.push('--default');
    }
    const result = await cli_command(args.join(' '));

    try {
        const config_result = JSON.parse(result);
        if (config_result.file) {
            config_result.file.filename = path.basename(config_result.file.filename);
        }
        return { config: config_result, default: config !== null };
    } catch (err) {
        throw Error(`RocketLogger configuration processing error: ${err} ` +
            `(${result})`);
    }
}

/// get the RocketLogger CLI version
async function version() {
    const result = await cli_command('--version');

    try {
        const version = result.split('\n')[0].split(' ').reverse()[0];
        return { version_string: result, version: version };
    } catch (err) {
        throw Error(`RocketLogger version processing error: ${err} ` +
            `(${result})`);
    }
}


/// run a RocketLogger cli command
async function cli_command(args) {
    const cmd = 'rocketlogger ' + args;
    const { stdout, stderr } = await exec(cmd, { timeout: 500 })
        .catch(err => {
            throw Error(`Failed to run RocketLogger CLI command: ${err} ` +
                `(cli: ${cmd})`);
        });
    return stdout;
}

/// start a RocketLogger measurement in background
async function cli_start(config) {
    const args = config_to_args_list('start', config);
    const subprocess = spawn('rocketlogger', args, {
        detached: true,
        stdio: 'ignore',
    });
}

/// reset RocketLogger service
async function reset_service() {
    const cmd = 'sudo pkill rocketloggerd';
    const { stdout, stderr } = await exec(cmd, { timeout: 500 })
        .catch(err => {
            throw Error(`Failed to reset RocketLogger service: ${err} ` +
                `(cli: ${cmd})`);
        });
    return stdout;
}

/// get RocketLogger CLI arguments from JSON configuration
function config_to_args_list(mode, config) {
    const args = [mode, '--json'];
    if (config === null) {
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
    if (config.file === null) {
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
