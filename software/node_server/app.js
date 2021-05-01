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
import os from 'os';
import fs from 'fs/promises';
import path from 'path';
import url from 'url';
import debug from 'debug';
import http from 'http';
import express from 'express';
import nunjucks from 'nunjucks';
import * as socketio from 'socket.io'

import { control as rl_control, data as rl_data, files as rl_files } from './rl.js';
import { is_same_filesystem, system_poweroff, system_reboot } from './util.js';

const __filename = url.fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

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
        files: await rl_files.get_data_file_info(),
        href_download_base: request.path + 'download/',
        href_delete_base: request.path + 'delete/',
    }
    await render_page(reply, 'data.html', context);
});

app.get('/data/download/:filename', async (request, reply) => {
    const filename = request.params.filename;
    try {
        await rl_files.validate_data_file(filename);
        const datafile_path = rl_files.get_data_path(filename);
        reply.sendFile(datafile_path);
    }
    catch (err) {
        reply.status(400).send(`Error accessing data file ${filename}: ${err}`);
    }
});

app.get('/data/delete/:filename', async (request, reply) => {
    const filename = request.params.filename;
    try {
        await rl_files.validate_data_file(filename);
        await rl_files.delete_data_file(filename);
        redirect_back_or_message(request, reply, `Deleted file ${filename}`);
    } catch (err) {
        reply.status(400).send(`Error deleting data file ${filename}: ${err}`);
    }
});

app.get('/log/', async (request, reply) => {
    const logfile_path = rl_files.get_log_path();
    try {
        reply.sendFile(logfile_path);
    }
    catch (err) {
        reply.status(400).send(`Error accessing log file: ${err}`);
    }
});


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
    try {
        const rl_version = await rl_control.version()
        if (rl_version?.version != version) {
            context.warn.push(`Potentially incompatible binary and web interface ` +
                `versions (interface: ${version}, binary: ${rl_version.version})`);
        }
        context.version_string = rl_version?.version_string;
        context.version = rl_version?.version;
    }
    catch (err) {
        context.err.push(err.toString());
    }
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

        try {
            const reply = await control_action(request);
            reply.req = request;
            socket.emit('control', reply);
        }
        catch (err) {
            socket.emit('control', { err: [err.toString()] });
        }
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
        try {
            const reply = await rl_control.status()
            reply.status.sdcard_available = await is_sdcard_mounted()
                .catch(err => {
                    reply.status.sdcard_available = false;
                    reply.err = [`Error checking for mounted SD card: ${err}`];
                });
            reply.req = request;
            socket.emit('status', reply);
        }
        catch (err) {
            socket.emit('status', { err: [err.toString()] });
        }
    });

    // handle cached data request
    socket.on('data', async (request) => {
        server_debug(`rl data: ${JSON.stringify(request)}`);

        const reply = await rl_data.data_cache_read(request);
        socket.emit('data', reply);
    });
};

async function is_sdcard_mounted() {
    return !await is_same_filesystem(rl_files.path_data, __dirname);
}

async function control_action(request) {
    switch (request.cmd) {
        case 'start':
            const result = await rl_control.start(request.config);
            rl_data.data_cache_reset();
            return result;

        case 'stop':
            return rl_control.stop();

        case 'config':
            if (request.config && request.default) {
                return rl_control.config(request.config);
            } else {
                return rl_control.config();
            }

        case 'reset':
            if (request.key !== request.cmd) {
                throw Error(`invalid reset key: ${request.key}`);
            }
            return rl_control.reset();

        case 'reboot':
            if (request.key !== request.cmd) {
                throw Error(`invalid reboot key: ${request.key}`);
            }
            return { status: await system_reboot() };

        case 'poweroff':
            if (request.key !== request.cmd) {
                throw Error(`invalid poweroff key: ${request.key}`);
            }
            return { status: await system_poweroff() };

        default:
            throw Error(`invalid control command: ${request.cmd}`);
    }
}


// set up data update subscriptions
const data_proxy = new rl_data.DataSubscriber();
data_proxy.onUpdate(async (data) => io.emit('data', data));

const status_proxy = new rl_data.StatusSubscriber();
status_proxy.onUpdate(async (status) => io.emit('status', status));


// run application
server_start();
status_proxy.run();
data_proxy.run();
