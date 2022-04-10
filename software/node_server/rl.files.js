"use strict";

// imports
import fs from 'fs/promises';
import path from 'path';
import picomatch from 'picomatch';

import { bytes_to_string, date_to_string } from './util.js';

export { delete_data_file, filter_data_filename, get_data_path, get_data_file_info, get_log_path, validate_data_file };


/// RocketLogger measurement data path
const path_data = '/home/rocketlogger/data';

/// RocketLogger measurement log file
const path_system_logfile = '/var/log/rocketlogger/rocketlogger.log';


// data and log file helper functions
async function validate_data_file(filename) {
    if (!is_valid_data_file(filename)) {
        throw Error(`invalid data filename: ${filename}`);
    }
}

function is_valid_data_file(filename) {
    return filename.indexOf(path.sep) < 0;
}

async function delete_data_file(filename) {
    return fs.unlink(get_data_path(filename));
}

function filter_data_filename(filename) {
    const basename = path.basename(filename);
    return basename;
}

function get_data_path(filename) {
    return path.join(path_data, filename);
}

function get_log_path() {
    return path_system_logfile;
}


// data file info helper functions
async function get_data_file_info() {
    const files = await get_data_files();
    const files_info = await Promise.all(files.map(get_file_info));
    return sort_file_info(files_info);
}

async function get_data_files() {
    const is_data_file = picomatch('*.@(rld|csv)');
    const dir_files = await fs.readdir(path_data);
    const data_files = dir_files.filter(f => is_data_file(f));
    return data_files.map(f => path.join(path_data, f));
}

function sort_file_info(files_info) {
    const compare_name = (a, b) => a.basename.localeCompare(b.basename);
    return files_info.sort(compare_name);
}

async function get_file_info(filename) {
    const file_stat = await fs.stat(filename);
    const file_info = {
        basename: path.basename(filename),
        dirname: path.dirname(filename),
        modified: date_to_string(file_stat.mtime),
        size: bytes_to_string(file_stat.size),
    };
    return file_info;
}
