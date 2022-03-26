"use strict";

import * as path from 'path';
import { promisify } from 'util';
import { execFile as execFile_, spawn } from 'child_process';
import { get_data_path } from './rl.files.js';

export { status, start, stop, reset, config, version };

const execFile = promisify(execFile_);


/// RocketLogger status and data update rate [in 1/s]
const rl_update_rate = 10;

/// RocketLogger CLI command timeout [in ms]
const command_timeout = 500;

/// RocketLogger command early exit delay timeout [in ms]
const command_early_exit_timeout = 1000;


/// get RocketLogger status
async function status() {
    const args = ['status', '--json'];
    const result = await rocketlogger_cli(args);

    try {
        const status = JSON.parse(result);
        return { status: status };
    } catch (err) {
        throw Error(`RocketLogger status processing error: ${err} (${result})`);
    }
}

/// start RocketLogger measurement
async function start(config) {
    const args = config_to_args_list('start', config);
    await rocketlogger_cli_detached(args);
    return { config: config };
}

/// stop RocketLogger measurement
async function stop() {
    const args = ['stop'];
    const result = await rocketlogger_cli(args);
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
    const result = await rocketlogger_cli(args);

    try {
        const config_result = JSON.parse(result);
        if (config_result.file) {
            config_result.file.filename = path.basename(config_result.file.filename);
        }
        return { config: config_result, default: config !== null };
    } catch (err) {
        throw Error(`RocketLogger configuration processing error: ${err} (${result})`);
    }
}

/// get the RocketLogger CLI version
async function version() {
    const args = ['--version'];
    const result = await rocketlogger_cli(args);

    try {
        const version = result.split('\n')[0].split(' ').reverse()[0];
        return { version_string: result, version: version };
    } catch (err) {
        throw Error(`RocketLogger version processing error: ${err} (${result})`);
    }
}


/// run a RocketLogger cli command with timeout and awaiting result
async function rocketlogger_cli(args) {
    const { stdout } = await execFile('rocketlogger', args, { timeout: command_timeout })
        .catch(err => {
            throw Error(`Failed to run RocketLogger CLI command: ${err} (args: ${args})`);
        });
    return stdout;
}

/// start a RocketLogger cli command in background process
async function rocketlogger_cli_detached(args) {
    return new Promise((resolve, reject) => {
        const subprocess = spawn('rocketlogger', args, {
            detached: true,
            stdio: 'ignore',
        });

        // check for error during process spawning
        subprocess.on('error', err => {
            reject(Error(`Failed to run RocketLogger CLI command: ${err} (args: ${args})`));
        });
        subprocess.on('exit', (code, signal) => {
            if (code === 0) {
                resolve();
            } else {
                reject(Error(`RocketLogger CLI command terminated early exit code: ${code} (args: ${args})`));
            }
        });
        setTimeout(resolve, command_early_exit_timeout);
    });
}

/// reset RocketLogger service
async function reset_service() {
    const args = ['pkill', 'rocketloggerd'];
    const { stdout } = await execFile('sudo', args, { timeout: command_timeout })
        .catch(err => {
            throw Error(`Failed to reset RocketLogger service: ${err}`);
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
        args.push(`--comment=${config.file.comment.replace(/"/g, '``')}`);
    }
    args.push(`--rate=${config.sample_rate}`);
    args.push(`--web=${config.web_enable}`);

    return args;
}
