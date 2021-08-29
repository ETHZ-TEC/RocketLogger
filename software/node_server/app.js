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
const os = require('os');
const fs = require('fs');
const path = require('path');
const glob = require('glob');
const http = require('http');
const express = require('express');
const nunjucks = require('nunjucks');
const socketio = require('socket.io');
const zmq = require('zeromq');
const debug = require('debug')('rocketlogger');

const rl = require('./rl.js');
const util = require('./util.js');

// get version of package and client-side assets
const version = require(path.join(__dirname, 'package.json')).version;
const asset_version = {
    bootstrap: require('bootstrap/package.json').version,
    jquery: require('jquery/package.json').version,
    plotly: require('plotly.js/package.json').version,
    socketio: require('socket.io-client/package.json').version,
    timesync: require('timesync/package.json').version,
}

// configuration
const hostname = 'localhost';
const port = 5000;
const path_static = path.join(__dirname, 'static');
const path_templates = path.join(__dirname, 'templates');


// initialize
const app = express();
const server = http.Server(app);
const io = socketio(server);

nunjucks.configure(path_templates, {
    autoescape: true,
    express: app,
});

/// render page templates
function render_page(req, res, template_name, context = null) {
    let date = new Date();

    // build or extend existing context
    if (context === null) {
        context = {};
    }
    if (!context.err) {
        context.err = [];
    }
    if (!context.warn) {
        context.warn = [];
    }
    context.today = {
        year: date.getUTCFullYear(),
        month: date.getUTCMonth(),
        day: date.getUTCDay(),
    };
    context.hostname = os.hostname();

    context.asset_version = asset_version;

    // validate compatibility of binary and web interface
    const rl_version = rl.version();
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

    res.render(template_name, context);  /// @todo sync IO call
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


// routing of rendered pages
app.get('/', (req, res) => { render_page(req, res, 'control.html') });

app.get('/data', (req, res) => {
    let files = [];
    glob.sync(path.join(rl.path_data, '*.@(rld|csv)')).forEach(file => {  /// @todo sync IO call (glob.sync)
        try {
            let stat = fs.statSync(file);  /// @todo sync IO call
            let file_info = {
                name: path.basename(file),
                modified: stat.mtime.toISOString().split('.')[0].replace('T', ' '),
                size: util.bytes_to_string(stat.size),
                filename: path.dirname(file),
                href_download: `/data/download/${path.basename(file)}`,
                href_delete: `/data/delete/${path.basename(file)}`,
            };
            files.push(file_info);
        } catch (err) {
            debug(`Error listing file ${file}: ${err}`);
        }
    });

    render_page(req, res, 'data.html', { files: files });
});


// routing of file actions
app.get('/log', (req, res) => {
    const logfile = rl.path_system_logfile;
    try {
        fs.accessSync(logfile, fs.constants.F_OK);  /// @todo sync IO call
    } catch (err) {
        res.status(404).send('Log file was not found. Please check your systems configuration!');
        return;
    }

    try {
        res.sendFile(logfile);
    } catch (err) {
        res.status(403).send('Error accessing log file. Please check your systems configuration!');
        return;
    }
});

app.get('/data/download/:filename', (req, res) => {
    let filepath = path.join(rl.path_data, req.params.filename);

    if (req.params.filename.indexOf(path.sep) >= 0) {
        res.status(400).send(`Invalid filename ${req.params.filename}.`);
        return;
    }

    try {
        fs.accessSync(filepath, fs.constants.R_OK);  /// @todo sync IO call
        res.download(filepath, req.params.filename);
    } catch (err) {
        res.status(404).send(`File ${req.params.filename} was not found.`);
    }
});

app.get('/data/delete/:filename', (req, res) => {
    let filepath = path.join(rl.path_data, req.params.filename);

    if (req.params.filename.indexOf(path.sep) >= 0) {
        res.status(400).send(`Invalid filename ${req.params.filename}.`);
        return;
    }

    try {
        fs.accessSync(filepath);  /// @todo sync IO call
    } catch (err) {
        res.status(404).send(`File ${req.params.filename} was not found.`);
        return;
    }

    try {
        fs.unlinkSync(filepath);  /// @todo sync IO call
    } catch (err) {
        res.status(403).send(`Error deleting file ${req.params.filename}: ${err}`);
        return;
    }

    res.redirect('back');
});

// socket.io configure new connection
io.on('connection', (socket) => {
    debug(`socket.io connect: ${socket.id}`);

    // logging disconnect
    socket.on('disconnect', () => {
        debug(`socket.io disconnect: ${socket.id}`);
    });

    // default message: echo back
    socket.on('message', (data) => {
        debug(`socket.io echo: ${data}`);
        socket.send({ echo: data });
    });

    // handle control command
    socket.on('control', (req) => {
        debug(`rl control: ${JSON.stringify(req)}`);
        // handle control command and emit on status channel
        let res = null;
        switch (req.cmd) {
            case 'start':
                res = rl.start(req.config);
                break;

            case 'stop':
                res = rl.stop();
                break;

            case 'config':
                if (req.config && req.default) {
                    res = rl.config(req.config);
                } else {
                    res = rl.config();
                }
                break;

            case 'reset':
                if (req.key !== req.cmd) {
                    res = { err: [`invalid reset key: ${req.key}`] };
                    break;
                }
                rl.stop();
                res = rl.reset();
                break;

            case 'reboot':
                if (req.key !== req.cmd) {
                    res = { err: [`invalid reboot key: ${req.key}`] };
                    break;
                }
                rl.stop();
                res = { status: util.system_reboot() };
                break;

            case 'poweroff':
                if (req.key !== req.cmd) {
                    res = { err: [`invalid poweroff key: ${req.key}`] };
                    break;
                }
                rl.stop();
                res = { status: util.system_poweroff() };
                break;

            default:
                res = { err: [`invalid control command: ${req.cmd}`] };
                break;
        }
        // process and send result
        if (res) {
            res.req = req;
            socket.emit('control', res);
        } else {
            socket.emit('control', { err: [`error processing command: ${req.cmd}`] });
        }
    });

    // handle status request
    socket.on('status', (req) => {
        debug(`rl status: ${JSON.stringify(req)}`);

        // poll status and emit on status channel
        if (req.cmd === 'status') {
            const res = rl.status();
            try {
                res.status.sdcard_available = !util.is_same_filesystem(rl.path_data, __dirname);
            } catch (err) {
                res.err.push(`Error checking for mounted SD card: ${err}`);
            }
            res.req = req;
            socket.emit('status', res);
        } else {
            socket.emit('status', { err: [`invalid status command: ${req.cmd}`] });
        }
    });

    // handle cached data request
    socket.on('data', (req) => {
        // @todo: implement local data caching
        debug(`rl data: ${JSON.stringify(req)}`);
    });

    // handle timesync request
    socket.on('timesync', function (data) {
        debug(`timeync: ${JSON.stringify(data)}`);
        socket.emit('timesync', {
            id: data && 'id' in data ? data.id : null,
            result: Date.now()
        });
    });
});


// zeromq data buffer proxy handlers
async function data_proxy() {
    const sock = new zmq.Subscriber

    sock.connect(rl.zmq_data_socket);
    debug(`zmq data subscribe to ${rl.zmq_data_socket}`);
    sock.subscribe();

    for await (const data of sock) {
        const rep = {
            t: Date.now(),
            metadata: {},
            data: {},
            digital: null,
        };

        // parse metadata (first data element)
        let meta;
        try {
            meta = JSON.parse(data[0]);
        } catch (err) {
            debug(`zmq data: metadata processing error: ${err}`);
            continue;
        }
        const downsample_factor = Math.max(1, meta.data_rate / rl.web_data_rate);

        // get timestamp and digital data views and size
        const time_in_view = new BigInt64Array(data[1].buffer);
        const digital_in_view = new Uint32Array(data[data.length - 1].buffer);
        const buffer_in_length = digital_in_view.length;
        const buffer_out_length = buffer_in_length / downsample_factor;

        // generate timestamps (second data element)
        const time_out = new Float64Array(buffer_out_length);
        for (let j = 0; j < time_out.length; j++) {
            time_out[j] = Number(time_in_view[0]) * 1e3 + Number(time_in_view[1]) / 1e6
                + j * 1e3 / rl.web_data_rate;
        }
        rep.time = time_out;

        // process digital data (last data element)
        const digital_out = new Uint8Array(buffer_out_length);
        for (let j = 0; j < Math.min(digital_out.length, digital_in_view.length / downsample_factor); j++) {
            digital_out[j] = digital_in_view[j * downsample_factor];
            /// @todo any/none down sampling
        }
        rep.digital = digital_out;

        // process channel data (starting at third data element)
        let i = 2;
        for (const ch of meta.channels) {
            rep.metadata[ch.name] = {
                scale: ch.scale,
                unit: ch.unit,
                bit: ch.bit,
            };
            if (ch.unit === 'binary') {
                continue;
            }

            const data_in_view = new Int32Array(data[i].buffer);
            const data_out = new Float32Array(buffer_out_length).fill(NaN);
            for (let j = 0; j < Math.min(data_out.length, data_in_view.length / downsample_factor); j++) {
                data_out[j] = data_in_view[j * downsample_factor] * ch.scale;
            }
            rep.data[ch.name] = data_out;
            i = i + 1;
        }

        // merge current channel data if available
        for (const ch of [1, 2]) {
            // reuse HI channel in-place if available
            if (`I${ch}H` in rep.data) {
                rep.data[`I${ch}`] = rep.data[`I${ch}H`];
                rep.metadata[`I${ch}`] = rep.metadata[`I${ch}H`];

                // delete merged channel
                delete rep.data[`I${ch}H`];
                delete rep.metadata[`I${ch}H`];
            }

            // merge valid LO current values if available
            if (`I${ch}L` in rep.data) {
                const channel_lo = rep.data[`I${ch}L`];
                const channel_lo_valid_mask = (0x01 << rep.metadata[`I${ch}L_valid`].bit);

                // generate new channel data array with NaNs if not available
                if (`I${ch}` in rep.data === false) {
                    rep.data[`I${ch}`] = new Float32Array(channel_lo.length).fill(NaN);
                    rep.metadata[`I${ch}`] = rep.metadata[`I${ch}L`];
                }
                const channel_merged = rep.data[`I${ch}`];
                for (let j = 0; j < channel_merged.length; j++) {
                    if (rep.digital[j] & channel_lo_valid_mask) {
                        channel_merged[j] = channel_lo[j];
                    }
                }

                // delete merged channel
                delete rep.data[`I${ch}L`];
                delete rep.metadata[`I${ch}L`];
            }

            // always delete channel valid links
            delete rep.metadata[`I${ch}L_valid`];
        }

        // send new data packet to clients
        io.emit('data', rep);

        /// @todo perform local data caching
    }
}

async function status_proxy() {
    const sock = new zmq.Subscriber

    sock.connect(rl.zmq_status_socket);
    debug(`zmq status subscribe to ${rl.zmq_status_socket}`);
    sock.subscribe();

    for await (const status of sock) {
        // debug(`zmq new status: ${status}`);
        try {
            io.emit('status', { status: JSON.parse(status) });
        } catch (err) {
            debug(`zmq status processing error: ${err}`);
        }
    }
}


// run webserver and data buffer proxies
server.listen(port, hostname, () => {
    debug(`RocketLogger web interface version ${version} listening at http://${hostname}:${port}`);
});

status_proxy();
data_proxy();
