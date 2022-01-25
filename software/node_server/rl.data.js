"use strict";

// imports
import debug from 'debug';
import * as zmq from 'zeromq';

export { data_proxy, status_proxy, web_data_rate };


/// ZeroMQ socket identifier for data publishing status
const status_socket = 'tcp://127.0.0.1:8276';

/// ZeroMQ socket identifier for status publishing
const data_socket = 'tcp://127.0.0.1:8277';

/// RocketLogger maximum web downstream data rate [in 1/s]
const web_data_rate = 1000;

const data_proxy_debug = debug('data_proxy');
const status_proxy_debug = debug('status_proxy');


/// RocketLogger status update proxy
async function status_proxy(send) {
    const sock = new zmq.Subscriber();

    sock.connect(status_socket);
    status_proxy_debug(`zmq status subscribe to ${status_socket}`);
    sock.subscribe();

    for await (const status of sock) {
        // debug(`zmq new status: ${status_data}`);
        try {
            const status_message = parse_status_to_message(status);
            send(status_message);
        } catch (err) {
            status_proxy_debug(`status proxy parse error: ${err}`);
        }
    }
}

function parse_status_to_message(status_data) {
    return { status: JSON.parse(status_data) };
}


/// RocketLogger data update proxy
async function data_proxy(send) {
    const sock = new zmq.Subscriber();

    sock.connect(data_socket);
    data_proxy_debug(`zmq data subscribe to ${data_socket}`);
    sock.subscribe();

    for await (const data of sock) {
        // debug(`zmq new data: ${data[0]}`);
        const time_received = Date.now();
        try {
            const data_message = parse_data_to_message(data);
            data_message.t = time_received;
            send(data_message);
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
