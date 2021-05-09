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

const rl = require('./rl.js');
const util = require('./util.js');

// get version if installed client-side assets
const asset_version = {
    bootstrap: require('bootstrap/package.json').version,
    jquery: require('jquery/package.json').version,
    plotly: require('plotly.js/package.json').version,
    socketio: require('socket.io-client/package.json').version,
    timesync: require('timesync/package.json').version,
}

// configuration
const port = 5000;
const version = '1.99';
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
                `versions (interface: ${version}, binary: ${context.version})`);
        }
        context.version_string = rl_version.version_string;
        context.version = rl_version.version;
    }

    res.render(template_name, context);
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
app.get('/', (req, res) => { render_page(req, res, 'index.html') });

app.get('/debug', (req, res) => { render_page(req, res, 'debug.html') });

app.get('/control', (req, res) => { render_page(req, res, 'control.html') });

app.get('/calibration', (req, res) => { render_page(req, res, 'calibration.html') });

app.get('/data', (req, res) => {
    let files = [];
    glob.sync(path.join(rl.path_data, '*.@(rld|csv)')).forEach(file => {
        try {
            let stat = fs.statSync(file);
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
            console.log(`Error listing file ${file}: ${err}`);
        }
    });

    render_page(req, res, 'data.html', { files: files });
});


// routing of file actions
app.get('/log', (req, res) => {
    const logfile = rl.path_system_logfile;
    try {
        fs.accessSync(logfile, fs.constants.F_OK);
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
        fs.accessSync(filepath, fs.constants.R_OK);
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
        fs.accessSync(filepath);
    } catch (err) {
        res.status(404).send(`File ${req.params.filename} was not found.`);
        return;
    }

    try {
        fs.unlinkSync(filepath);
    } catch (err) {
        res.status(403).send(`Error deleting file ${req.params.filename}: ${err}`);
        return;
    }

    res.redirect('back');
});

// socket.io configure new connection
io.on('connection', (socket) => {
    console.log(`socket.io connect: ${socket.id}`);

    // logging disconnect
    socket.on('disconnect', () => {
        console.log(`socket.io disconnect: ${socket.id}`);
    });

    // default message: echo back
    socket.on('message', (data) => {
        console.log(`socket.io echo: ${data}`);
        socket.send({ echo: data });
    });

    // handle control command
    socket.on('control', (req) => {
        console.log(`rl control: ${JSON.stringify(req)}`);
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
        console.log(`rl status: ${JSON.stringify(req)}`);

        // poll status and emit on status channel
        if (req.cmd === 'status') {
            const res = rl.status();
            res.req = req;
            socket.emit('status', res);
        } else {
            socket.emit('status', { err: [`invalid status command: ${req.cmd}`] });
        }
    });

    // handle cached data request
    socket.on('data', (req) => {
        // @todo: implement local data caching
        console.log(`rl data: ${JSON.stringify(req)}`);
    });

    // handle timesync request
    socket.on('timesync', function (data) {
        console.log(`timeync: ${JSON.stringify(data)}`);
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
    console.log(`zmq data subscribe to ${rl.zmq_data_socket}`);
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
            console.log(`zmq data: metadata processing error: ${err}`);
            continue;
        }

        // get downsample factor
        const downsample_factor = Math.max(1, meta.data_rate / rl.web_data_rate);
        const buffer_length = data[data.length - 1].length / Uint32Array.BYTES_PER_ELEMENT / downsample_factor;

        // generate timestamps (second data element)
        const time_in_view = new BigInt64Array(data[1].buffer);
        const time_out = new ArrayBuffer(buffer_length * Float64Array.BYTES_PER_ELEMENT);
        const time_out_view = new Float64Array(time_out);
        for (let j = 0; j < time_out_view.length; j++) {
            time_out_view[j] = Number(time_in_view[0]) * 1e3 + Number(time_in_view[1]) / 1e6
                + j * 1e3 / rl.web_data_rate;
        }
        rep.time = time_out;

        // process digital data (last data element)
        const digital_in_view = new Uint32Array(data[data.length - 1].buffer);
        const digital_out = new ArrayBuffer(buffer_length * Uint8Array.BYTES_PER_ELEMENT);
        const digital_out_view = new Uint8Array(digital_out);
        for (let j = 0; j < Math.min(digital_out_view.length, digital_in_view.length / downsample_factor); j++) {
            digital_out_view[j] = digital_in_view[j * downsample_factor];
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
            const data_out = new ArrayBuffer(buffer_length * Float32Array.BYTES_PER_ELEMENT);
            const data_out_view = new Float32Array(data_out).fill(NaN);
            for (let j = 0; j < Math.min(data_out_view.length, data_in_view.length / downsample_factor); j++) {
                data_out_view[j] = data_in_view[j * downsample_factor] * ch.scale;
            }
            rep.data[ch.name] = data_out;
            i = i + 1;
        }

        // merge current channel data if available
        for (const ch of [1, 2]) {
            // generate new channel data and info (reuse HI channel in-place)
            rep.data[`I${ch}`] = rep.data[`I${ch}H`];
            const merge_out = rep.data[`I${ch}`];

            const channel_lo = rep.data[`I${ch}L`];
            const channel_lo_valid_mask = (0x01 << rep.metadata[`I${ch}L_valid`].bit);
            for (let j = 0; j < merge_out.length; j++) {
                if (rep.digital & channel_lo_valid_mask) {
                    merge_out[j] = channel_lo[j];
                }
            }
            // rep.data[`I${ch}`] = merge_out;
            rep.metadata[`I${ch}`] = rep.metadata[`I${ch}L`];

            // delete merged channels
            delete rep.data[`I${ch}L`];
            delete rep.metadata[`I${ch}L`];
            delete rep.data[`I${ch}H`];
            delete rep.metadata[`I${ch}H`];
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
    console.log(`zmq status subscribe to ${rl.zmq_status_socket}`);
    sock.subscribe();

    for await (const status of sock) {
        // console.log(`zmq new status: ${status}`);
        try {
            io.emit('status', { status: JSON.parse(status) });
        } catch (err) {
            console.log(`zmq status processing error: ${err}`);
        }
    }
}


// run webserver and data buffer proxies
server.listen(port, () => {
    console.log(`Example app listening at http://localhost:${port}`);
});

status_proxy();
data_proxy();
