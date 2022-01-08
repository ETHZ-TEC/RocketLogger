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
import * as zmq from 'zeromq';

import * as rl from './rl.js';
import { bytes_to_string, date_to_string, is_same_filesystem, system_poweroff, system_reboot } from './util.js';

const __filename = url.fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const server_debug = debug('server');
const data_proxy_debug = debug('data_proxy');
const status_proxy_debug = debug('status_proxy');
const glob = promisify(G);

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


// initialize
const app = express();
const server = http.Server(app);
const io = new socketio.Server(server);

nunjucks.configure(path_templates, {
    autoescape: true,
    express: app,
});


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

app.get('/log/', async (request, reply) => {
    await serve_file(reply, path.basename(rl.path_system_logfile), path.dirname(rl.path_system_logfile));
});

app.get('/data/', async (request, reply) => {
    const context = {
        files: await get_data_file_info(),
        href_download_base: request.path + 'download/',
        href_delete_base: request.path + 'delete/',
    }
    await render_page(reply, 'data.html', context);
});

app.get('/data/download/:filename', async (request, reply) => {
    await serve_file_if_valid(reply, request.params.filename, rl.path_data);
});

app.get('/data/delete/:filename', async (request, reply) => {
    const deleted = await delete_file_if_valid(reply, request.params.filename, rl.path_data);
    if (deleted) {
        redirect_back_or_message(request, reply, `Deleted file ${request.params.filename}`);
    }
});


// request handling helper functions
async function is_valid_filename(filename) {
    return filename.indexOf(path.sep) >= 0;
}

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
    return await glob(path.join(rl.path_data, filename_glob));
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

// file operation helper functions
async function serve_file_if_valid(reply, basename, dirname) {
    if (is_valid_filename(basename)) {
        return await serve_file(reply, basename, dirname);
    }
    reply.status(400).send(`Invalid filename ${basename}.`);
}

async function serve_file(reply, basename, dirname) {
    try {
        await fs.access(dirname, constants.R_OK);
        reply.sendFile(path.join(dirname, basename));
    } catch (err) {
        reply.status(404).send(`File ${basename} was not found.`);
    }
}

async function delete_file_if_valid(reply, basename, dirname) {
    if (is_valid_filename(basename)) {
        return await delete_file(reply, basename, dirname);
    }
    reply.status(400).send(`Invalid filename ${basename}.`);
    return false;
}

async function delete_file(reply, basename, dirname) {
    const filepath = path.join(dirname, basename);
    try {
        await fs.access(filepath);
    } catch (err) {
        reply.status(404).send(`File ${basename} was not found.`);
        return false;
    }
    try {
        await fs.unlink(filepath);
    } catch (err) {
        reply.status(403).send(`Error deleting file ${filename}: ${err}`);
        return false;
    }
    return true;
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
            return await rl.start(request.config);

        case 'stop':
            return await rl.stop();

        case 'config':
            if (request.config && request.default) {
                return await rl.config(request.config);
            } else {
                return await rl.config();
            }

        case 'reset':
            if (request.key !== request.cmd) {
                throw Error(`invalid reset key: ${request.key}`);
            }
            await rl.stop();
            return await rl.reset();

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


// data buffer update subscriber
async function data_proxy() {
    const sock = new zmq.Subscriber();

    sock.connect(rl.data_socket);
    data_proxy_debug(`zmq data subscribe to ${rl.data_socket}`);
    sock.subscribe();

    for await (const data of sock) {
        // debug(`zmq new data: ${data[0]}`);
        const time_received = Date.now();
        try {
            const data_message = parse_data_to_message(data);
            data_message.t = time_received;
            io.emit('data', data_message);
            /// @todo perform local data caching
        } catch (err) {
            data_proxy_debug(`data proxy parse error: ${err}`);
            continue;
        }
    }
}

function parse_data_to_message(data) {
    const message = {
        metadata: {},
        data: {},
        digital: null,
    };

    const header = parse_data_header(data);

    for (const metadata of header.channels) {
        message.metadata[metadata.name] = metadata;
    }

    message.time = parse_time_data(header, data[1]);
    message.digital = parse_digital_data(header, data[data.length -1]);

    // process channel metadata and non-binary data channels
    let channel_data_index = 2;
    for (const channel in message.metadata) {
        const metadata = message.metadata[channel];
        if (metadata.unit === 'binary') {
            continue;
        }

        message.data[channel] = parse_channel_data(header, metadata, data[channel_data_index]);
        channel_data_index = channel_data_index + 1;
    }

    merge_channels(message.metadata, message.data, message.digital);

    return message;
}

function parse_data_header(data) {
    const header = JSON.parse(data[0]);

    header.downsample_factor = Math.max(1, header.data_rate / rl.web_data_rate);
    header.sample_count = (new Uint32Array(data[data.length - 1].buffer)).length;

    return header;
}

function parse_time_data(header, data) {
    const time_in_view = new BigInt64Array(data.buffer);
    const data_out_length = header.sample_count / header.downsample_factor;

    const data_out = new Float64Array(data_out_length);
    for (let j = 0; j < data_out_length; j++) {
        data_out[j] = Number(time_in_view[0]) * 1e3 + Number(time_in_view[1]) / 1e6
            + j * 1e3 / rl.web_data_rate;
    }

    return data_out;
}
function parse_digital_data(header, data) {
    const data_in_view = new Uint32Array(data.buffer);
    const data_out_length = data_in_view.length / header.downsample_factor;

    const data_out = new Uint8Array(data_out_length);
    for (let j = 0; j < Math.min(data_out_length, data_in_view.length / header.downsample_factor); j++) {
        data_out[j] = data_in_view[j * header.downsample_factor];
        /// @todo any/none down sampling
    }

    return data_out;
}

function parse_channel_data(header, metadata, data) {
    if (metadata.unit === 'binary') {
        throw Error('cannot parse binary as non-binary data');
    }

    const data_in_view = new Int32Array(data.buffer);
    const data_out_length = data_in_view.length / header.downsample_factor;

    const data_out = new Float32Array(data_out_length).fill(NaN);
    for (let j = 0; j < Math.min(data_out_length, data_in_view.length / header.downsample_factor); j++) {
        data_out[j] = data_in_view[j * header.downsample_factor] * metadata.scale;
    }

    return data_out;
}

function merge_channels(metadata, channel_data, digital_data) {
    // merge current channel data if available
    for (const ch of [1, 2]) {
        // reuse HI channel in-place if available
        if (`I${ch}H` in channel_data) {
            channel_data[`I${ch}`] = channel_data[`I${ch}H`];
            metadata[`I${ch}`] = metadata[`I${ch}H`];

            // delete merged channel
            delete channel_data[`I${ch}H`];
            delete metadata[`I${ch}H`];
        }

        // merge valid LO current values if available
        if (`I${ch}L` in channel_data) {
            const channel_lo = channel_data[`I${ch}L`];
            const channel_lo_valid_mask = (0x01 << metadata[`I${ch}L_valid`].bit);

            // generate new channel data array with NaNs if not available
            if (`I${ch}` in channel_data === false) {
                channel_data[`I${ch}`] = new Float32Array(channel_lo.length).fill(NaN);
                metadata[`I${ch}`] = metadata[`I${ch}L`];
            }
            const channel_merged = channel_data[`I${ch}`];
            for (let j = 0; j < channel_merged.length; j++) {
                if (digital_data[j] & channel_lo_valid_mask) {
                    channel_merged[j] = channel_lo[j];
                }
            }

            // delete merged channel
            delete channel_data[`I${ch}L`];
            delete metadata[`I${ch}L`];
        }

        // always delete channel valid links
        delete metadata[`I${ch}L_valid`];
    }
}


// status update subscriber
async function status_proxy() {
    const sock = new zmq.Subscriber();

    sock.connect(rl.status_socket);
    status_proxy_debug(`zmq status subscribe to ${rl.status_socket}`);
    sock.subscribe();

    for await (const status of sock) {
        // debug(`zmq new status: ${status_data}`);
        try {
            const status_message = parse_status_to_message(status);
            io.emit('status', status_message);
        } catch (err) {
            status_proxy_debug(`status proxy parse error: ${err}`);
        }
    }
}

function parse_status_to_message(status_data) {
    return { status: JSON.parse(status_data) };
}


// install socket.io connect handler
io.on('connect', client_connected);

// run webserver and data buffer proxies
server.listen(port, hostname, () => {
    server_debug(`RocketLogger web interface version ${version} listening at http://${hostname}:${port}`);
});

status_proxy();
data_proxy();
