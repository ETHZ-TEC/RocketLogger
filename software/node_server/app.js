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
const { spawnSync } = require('child_process');


// configuration
const port = 5000;
const version = '1.99';
const zmq_data_socket = 'tcp://127.0.0.1:5555'
const path_system_logfile = '/var/log/rocketlogger.log';
const path_static = path.join(__dirname, 'static');
const path_templates = path.join(__dirname, 'templates');
const path_files = path.join(__dirname, 'files');


// initialize
const app = express();
const server = http.Server(app);
const io = socketio(server);

// app.use(express.json()) // for parsing application/json
app.use(express.urlencoded({ extended: true })) // for parsing application/x-www-form-urlencoded

nunjucks.configure(path_templates, {
    autoescape: true,
    express: app,
});


/// helper function to display byte values
function bytes_to_string(bytes) {
    if (bytes === 0) {
        return "0 B";
    }
    let log1k = Math.floor(Math.log10(bytes) / 3);
    let value = (bytes / Math.pow(1000, log1k));

    switch (log1k) {
        case 0:
            return value.toFixed(0) + " B";
        case 1:
            return value.toFixed(2) + " kB";
        case 2:
            return value.toFixed(2) + " MB";
        case 3:
            return value.toFixed(2) + " GB";
        case 4:
            return value.toFixed(2) + " TB";
        default:
            return bytes.toPrecision(5);
    }
}

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

    // validate compatibility of binart and web interface
    try {
        let cmd = spawnSync('rocketlogger', ['--version']);
        if (cmd.error) {
            context.err.push('RocketLogger binary was not found. ' +
                'Please check your system configuration!');
        } else {
            context.version_string = cmd.stdout.toString();
            context.version = context.version_string.split('\n')[0].split(' ').reverse()[0];
            if (version != context.version) {
                context.warn.push(`Potentially incompatible binary and web interface ` +
                    `versions (interface: ${version}, binary: ${context.version})`);
            }
        }
    } catch (err) {
        console.log(err);
        context.err.push('Failed getting RocketLogger binary version. ' +
            'Please check your system configuration!');
        context.version = null;
        context.version_string = null;
    }

    res.render(template_name, context);
}


// routing of static files
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
    glob.sync(path.join(path_files, '*.@(rld|csv)')).forEach(file => {
        try {
            let stat = fs.statSync(file);
            let file_info = {
                name: path.basename(file),
                modified: stat.mtime.toISOString().split('.')[0].replace('T', ' '),
                size: bytes_to_string(stat.size),
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
    try {
        fs.accessSync(path_system_logfile, fs.constants.R_OK);
        res.sendFile(path_system_logfile);
    } catch (err) {
        res.status(404).send('Log file was not found. Please check your systems configuration!');
    }
});

app.get('/data/download/:filename', (req, res) => {
    let filepath = path.join(path_files, req.params.filename);

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

app.delete('/data/delete/:filename', (req, res) => {
    let filepath = path.join(path_files, req.params.filename);

    if (req.params.filename.indexOf(path.sep) >= 0) {
        res.status(400).send(`Invalid filename ${req.params.filename}.`);
        return;
    }

    try {
        fs.accessSync(filepath);
    } catch (err) {
        res.status(404).send(`File ${req.params.filename} was not found.`);
    }

    try {
        fs.unlinkSync(filepath);
    } catch (err) {
        res.status(403).send(`Error deleting file ${req.params.filename}: ${err}`);
    }

    res.send(`deleted file: ${req.params.filename}`);
});

// routing of control actions
app.post('/control/:action', (req, res) => {
    console.log(req.body);

    let action = req.params.action;
    let data = null;
    if (!action) {
        res.status(400).send('Invalid request: no action.');
    }
    try {
        data = JSON.parse(req.body.data);
    } catch (err) {
        res.status(400).send('Invalid request: no data.');
    }

    if (action != data.action) {
        res.status(400).send('Invalid request: ambiguous action in the request data.');
    }

    let response = {
        req: {
            action: action,
            data: data,
        },
        rep: {
            err: null,
            warn: null,
            msg: null,
        },
    };

    switch (action) {
        case 'start':
            break;
        case 'stop':
            break;
        case 'status':
            break;
        case 'config':
            break;
        case 'calibrate':
            break;
        default:
            res.status(400).send(`Invalid request: unknown action ${action}.`);
            break;
    }
    res.json(response);
});


// socket.io connection handlers
io.on('connection', (socket) => {
    console.log(`socket.io connect: ${socket.id}`);
    socket.on('disconnect', (socket) => {
        console.log(`socket.io disconnect: ${socket.id}`);
    });
    socket.on('control', (data) => {
        console.log(`socket.io control: ${data}`);
    });
    socket.on('message', (data) => {
        console.log(`socket.io echo: ${data}`);
        socket.send({ echo: data });
    });
});


// zeromq data buffer proxy handlers
async function data_proxy() {
    const sock = new zmq.Subscriber

    sock.connect(zmq_data_socket);
    console.log(`zmq sub: connected to ${zmq_data_socket}`);

    sock.subscribe();
    for await (const [timestamp, data] of sock) {
        io.emit('data', { time: Date.now(), ts: timestamp, data: data });
    }
}


// run webserver and data buffer proxies
server.listen(port, () => {
    console.log(`Example app listening at http://localhost:${port}`);
});

data_proxy();
