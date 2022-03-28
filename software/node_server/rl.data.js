"use strict";

// imports
import debug from 'debug';
import * as zmq from 'zeromq';
import { filter_data_filename } from './rl.files.js';

export { DataSubscriber, StatusSubscriber, data_cache_reset, data_cache_read };


/// ZeroMQ socket identifier for status publishing
const status_socket = 'tcp://127.0.0.1:8276';

/// ZeroMQ socket identifier for data publishing
const data_socket = 'tcp://127.0.0.1:8277';

/// RocketLogger maximum web downstream data rate [in 1/s]
const web_data_rate = 1000;

/// Measurement data cache size [in number of timestamps]
const data_cache_size = 1000000;

/// Size of reply to data cache requests [in number of timestamps]
const data_cache_reply_size = data_cache_size / 20;

/// Measurement data cache buffer
const data_cache = {
    metadata: {},
    time: [],
    data: {},
    digital: [],
    reset: true,
};

const data_proxy_debug = debug('data_proxy');

class Subscriber {
    constructor(socketAddress) {
        this._onUpdate = null;
        this._socketAddress = socketAddress;
        this._debug = debug('rocketlogger');
    }

    onUpdate(callback) {
        if (callback !== null && typeof callback !== 'function') {
            throw Error('callback is not a function');
        }
        this._onUpdate = callback;
    }

    async run() {
        this._socket = new zmq.Subscriber();
        this._socket.connect(this._socketAddress);
        this._socket.subscribe();

        for await (const raw of this._socket) {
            // this._debug(`subscriber new data: ${raw}`);
            let data = null;
            try {
                data = this._parse(raw);
            } catch (err) {
                this._debug(`subscriber parse error: ${err}`);
            }
            if (this._onUpdate !== null) {
                this._onUpdate(data);
            }
        }
    }
}

class StatusSubscriber extends Subscriber {
    constructor(socketAddress = status_socket) {
        super(socketAddress);
    }

    _parse(raw) {
        const status = JSON.parse(raw);
        if (status.config?.file) {
            status.config.file.filename = filter_data_filename(status.config.file?.filename);
        }
        return { status: status };
    }
}

class DataSubscriber extends Subscriber {
    constructor(socketAddress = data_socket) {
        super(socketAddress);
    }

    _parse(raw) {
        const data_message = parse_data_to_message(raw);
        data_cache_write(data_message);
        return data_message;
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
    message.digital = parse_digital_data(header, data[data.length - 1]);

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

    header.downsample_factor = Math.max(1, header.data_rate / web_data_rate);
    header.sample_count = (new Uint32Array(data[data.length - 1].buffer)).length;

    return header;
}

function parse_time_data(header, data) {
    const time_in_view = new BigInt64Array(data.buffer);
    const data_out_length = header.sample_count / header.downsample_factor;

    const data_out = new Float64Array(data_out_length);
    for (let j = 0; j < data_out_length; j++) {
        data_out[j] = Number(time_in_view[0]) * 1e3 + Number(time_in_view[1]) / 1e6
            + j * 1e3 / web_data_rate;
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


// reset data cache with next write
function data_cache_reset() {
    data_cache.reset = true;
}

// write new data to cache
async function data_cache_write(reply) {
    // check for pending cache reset
    if (data_cache.reset) {
        data_proxy_debug('clear and re-initialize data cache');
        data_cache_clear();
        data_cache_init(reply.metadata);
    }

    // validate metadata of incoming data
    if (data_cache.metadata.length !== reply.metadata.length) {
        data_proxy_debug('metadata mismatch of data cache and incoming data!');
        return;
    }

    const time_view = new Float64Array(reply.time);
    for (let i = 0; i < time_view.length; i++) {
        data_cache.time.push(time_view[i]);
    }
    if (data_cache.time.length > data_cache_size) {
        data_cache.time.splice(0, data_cache.time.length - data_cache_size);
    }

    const digital_view = new Uint8Array(reply.digital);
    for (let i = 0; i < digital_view.length; i++) {
        data_cache.digital.push(digital_view[i]);
    }
    if (data_cache.digital.length > data_cache_size) {
        data_cache.digital.splice(0, data_cache.digital.length - data_cache_size);
    }

    for (const ch in data_cache.metadata) {
        if (data_cache.metadata[ch].unit === 'binary') {
            continue;
        }

        const tmp = new Float32Array(data_cache.data[ch].length + reply.data[ch].length);
        tmp.set(data_cache.data[ch], 0);
        tmp.set(reply.data[ch], data_cache.data[ch].length);
        data_cache.data[ch] = tmp;

        if (data_cache.data[ch].length > data_cache_size) {
            data_cache.data[ch].splice(0, data_cache.data[ch].length - data_cache_size);
        }
    }
    // data_proxy_debug(`data write: cache_size=${data_cache.time.length}`);
}

// clear data cache
function data_cache_clear() {
    data_proxy_debug('clear data cache');
    data_cache.metadata = {};
    data_cache.time = [];
    data_cache.digital = [];
    data_cache.data = {};
    data_cache.reset = true;
}

// initialize data cache
function data_cache_init(metadata) {
    debug('init data cache');
    data_cache.metadata = metadata;
    data_cache.time = [];
    data_cache.digital = [];
    data_cache.data = {};
    for (const ch in data_cache.metadata) {
        data_cache.data[ch] = [];
    }
    data_cache.reset = false;
}

// read from data cache for values before time_reference
async function data_cache_read(time_reference) {
    data_proxy_debug(`cache read: time_reference=${time_reference}, cache_size=${data_cache.time.length}`);

    const message = {
        metadata: {},
        time: null,
        data: {},
        digital: null,
    };

    // reverse lookup first unavailable data data from cache
    const cache_time_view = new Float64Array(data_cache.time);
    let index_end = cache_time_view.length;
    while (index_end > 0) {
        if (cache_time_view[index_end - 1] < time_reference) {
            break;
        }
        index_end--;
    }

    // check for and return on cache miss
    if (index_end <= 0) {
        data_proxy_debug('cache miss: send empty message');
        return message;
    }

    // cache hit, calculate data range for message
    const index_start = Math.max(0, index_end - data_cache_reply_size);
    data_proxy_debug(`cache hit: send data range ${index_start}:${index_end}`);

    // assemble data reply message from cache
    message.metadata = data_cache.metadata;
    message.time = cache_time_view.slice(index_start, index_end);

    for (const ch in data_cache.metadata) {
        if (data_cache.metadata[ch].unit === 'binary') {
            continue;
        }

        const cache_data_view = new Float32Array(data_cache.data[ch]);
        message.data[ch] = cache_data_view.slice(index_start, index_end);
    }

    const cache_digital_view = new Uint8Array(data_cache.digital);
    message.digital = cache_digital_view.slice(index_start, index_end);

    return message;
}
