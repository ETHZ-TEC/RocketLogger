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

// imports
import * as os from 'os';
import * as fs from 'fs/promises';
import { constants } from 'fs';
import * as path from 'path';
import url from 'url';
import debug from 'debug';
import { promisify } from 'util';
import G from 'glob';
import * as http from 'http';
import express from 'express';
import nunjucks from 'nunjucks';
import * as socketio from 'socket.io'

import * as rl from './rl.js';
import { data_proxy, status_proxy } from './rl.data.js';
import { bytes_to_string, date_to_string, is_same_filesystem, system_poweroff, system_reboot } from './util.js';

const __filename = url.fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const glob = promisify(G);

const server_debug = debug('server');

// get version of a package
const get_version = async (package_path = __dirname) => {
    return JSON.parse(await fs.readFile(path.join(package_path, 'package.json'))).version;
}

// configuration
const version = await get_version();
const hostname = 'localhost';
const port = 5000;
const path_static = path.join(__dirname, 'static');
const path_modules = path.join(__dirname, 'node_modules');
const path_templates = path.join(__dirname, 'templates');
const asset_version = {
    bootstrap: await get_version(path.join(path_modules, 'bootstrap')),
    jquery: await get_version(path.join(path_modules, 'jquery')),
    plotly: await get_version(path.join(path_modules, 'plotly.js')),
    socketio: await get_version(path.join(path_modules, 'socket.io-client')),
    timesync: await get_version(path.join(path_modules, 'timesync')),
};


// server application and plugin initialization
const app = express();
const server = http.Server(app);
const io = new socketio.Server(server);

nunjucks.configure(path_templates, {
    autoescape: true,
    express: app,
});


// run webserver
const server_start = async () => {
    try {
        io.on('connect', client_connected);
        server.listen(port, hostname, () => {
            console.log(`RocketLogger web interface version ${version} listening at http://${hostname}:${port}`);
        });
    } catch (err) {
        console.error(err);
        process.exit(1);
    }
}


// routing of static files
app.use('/assets', [
    express.static(__dirname + '/node_modules/bootstrap/dist/css'),
    express.static(__dirname + '/node_modules/bootstrap/dist/js'),
    express.static(__dirname + '/node_modules/jquery/dist'),
    express.static(__dirname + '/node_modules/plotly.js/dist'),
    express.static(__dirname + '/node_modules/socket.io-client/dist'),
    express.static(__dirname + '/node_modules/timesync/dist')
]);
app.use('/static', express.static(path_static));
app.get('/robots.txt', express.static(path_static));
app.get('/sitemap.xml', express.static(path_static));


// route dynamically rendered pages
app.get('/', async (request, reply) => { await render_page(reply, 'control.html'); });

app.get('/data/', async (request, reply) => {
    const context = {
        files: await get_data_file_info(),
        href_download_base: request.path + 'download/',
        href_delete_base: request.path + 'delete/',
    }
    await render_page(reply, 'data.html', context);
});

app.get('/data/download/:filename', async (request, reply) => {
    const filename = request.params.filename;
    try {
        await validate_data_file(filename);
        reply.sendFile(get_data_path(filename));
    }
    catch (err) {
        reply.status(400).send(`Error accessing data file ${filename}: ${err}`);
    }
});

app.get('/data/delete/:filename', async (request, reply) => {
    const filename = request.params.filename;
    try {
        await validate_data_file(filename); 
        await delete_data_file(filename);
        redirect_back_or_message(request, reply, `Deleted file ${filename}`);
    } catch (err) {
        reply.status(400).send(`Error deleting data file ${filename}: ${err}`);
    }
});

app.get('/log/', async (request, reply) => {
    try {
        reply.sendFile(get_log_path());
    }
    catch (err) {
        reply.status(400).send(`Error accessing log file: ${err}`);
    }
});


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

function get_data_path(filename) {
    return path.join(rl.path_data, filename);
}

function get_log_path() {
    return rl.path_system_logfile;
}


// request handling helper functions
function redirect_back_or_message(request, reply, message) {
    const referer = request.headers?.referer;
    if (referer) {
        reply.redirect(referer);
    } else {
        reply.send(message);
    }
}


// render page template
async function render_page(reply, template_name, context = null) {
    // init context if none provided
    if (context === null) {
        context = {};
    }
    await amend_system_context(context);
    return reply.render(template_name, context);  /// @todo sync IO call
}

async function amend_system_context(context) {
    if (!context.err) {
        context.err = [];
    }
    if (!context.warn) {
        context.warn = [];
    }

    const now = new Date();
    context.today = {
        year: now.getUTCFullYear(),
        month: now.getUTCMonth(),
        day: now.getUTCDay(),
    };
    context.hostname = os.hostname();

    context.asset_version = asset_version;

    // validate compatibility of binary and web interface
    const rl_version = await rl.version();
    for (const err of rl_version.err) {
        context.err.push(err);
    }
    if (rl_version.version) {
        if (rl_version.version != version) {
            context.warn.push(`Potentially incompatible binary and web interface ` +
                `versions (interface: ${version}, binary: ${rl_version.version})`);
        }
        context.version_string = rl_version.version_string;
        context.version = rl_version.version;
    }
}


// data file info helper functions
async function get_data_file_info() {
    const files = await get_data_files();
    const files_info = await Promise.all(files.map(get_file_info));
    return sort_file_info(files_info);
}

async function get_data_files(filename_glob = '*.@(rld|csv)') {
    return glob(path.join(rl.path_data, filename_glob));
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


// configure new client connection
const client_connected = (socket) => {
    server_debug(`client connected: ${socket.id}`);

    // logging disconnect
    socket.on('disconnect', () => {
        server_debug(`client disconnected: ${socket.id}`);
    });

    // handle time synchronization
    socket.on('timesync', (request) => {
        server_debug(`timesync: ${JSON.stringify(request)}`);
        socket.emit('timesync', {
            id: request?.id,
            result: Date.now(),
        });
    });

    // handle control command
    socket.on('control', async (request) => {
        server_debug(`rl control: ${JSON.stringify(request)}`);

        const reply = await control_action(request)
            .catch(err => {
                return { err: [err.toString()] };
            });
        reply.req = request;
        socket.emit('control', reply);
    });

    // handle status request
    socket.on('status', async (request) => {
        server_debug(`rl status: ${JSON.stringify(request)}`);

        // validate status update request
        if (request.cmd !== 'status') {
            socket.emit('status', { err: [`invalid status command: ${request.cmd}`] });
            return;
        }

        // poll status and emit on status channel
        const reply = await rl.status()
            .catch(err => {
                return { err: [err.toString()] };
            });
        reply.status.sdcard_available = await is_sdcard_mounted()
            .catch(err => {
                reply.err.push(`Error checking for mounted SD card: ${err}`);
            });
        reply.req = request;
        socket.emit('status', reply);
    });

    // handle cached data request
    socket.on('data', (req) => {
        // @todo: implement local data caching
        server_debug(`rl data: ${JSON.stringify(req)}`);
    });
};

async function is_sdcard_mounted() {
    return !await is_same_filesystem(rl.path_data, __dirname);
}

async function control_action(request) {
    switch (request.cmd) {
        case 'start':
            return rl.start(request.config);

        case 'stop':
            return rl.stop();

        case 'config':
            if (request.config && request.default) {
                return rl.config(request.config);
            } else {
                return rl.config();
            }

        case 'reset':
            if (request.key !== request.cmd) {
                throw Error(`invalid reset key: ${request.key}`);
            }
            await rl.stop();
            return rl.reset();

        case 'reboot':
            if (request.key !== request.cmd) {
                throw Error(`invalid reboot key: ${request.key}`);
            }
            await rl.stop();
            return { status: await system_reboot() };

        case 'poweroff':
            if (request.key !== request.cmd) {
                throw Error(`invalid poweroff key: ${request.key}`);
            }
            await rl.stop();
            return { status: await system_poweroff() };

        default:
            throw Error(`invalid control command: ${request.cmd}`);
    }
}


// run application
server_start();

status_proxy(async (data) => io.emit('status', data));
data_proxy(async (data) => io.emit('data', data));
