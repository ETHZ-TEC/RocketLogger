"use strict";

// imports
import debug from 'debug';
import { AggregatingBuffer } from './buffer.js';

export { DataCache };


class DataCache {
    constructor(size, levels, aggregation_factor, metadata) {
        this._size = size;
        this._levels = levels;
        this._aggregation_factor = aggregation_factor;
        this._metadata = metadata;
        this._debug = debug('rocketlogger:cache');
        this.reset();
    }

    // reset data cache by re-initializing buffers for existing
    reset() {
        this._time = new AggregatingBuffer(Float64Array, this._size, this._levels, this._aggregation_factor, NaN);
        this._digital = new AggregatingBuffer(Uint8Array, this._size, this._levels, this._aggregation_factor);
        this._data = {};
        for (const ch in this._metadata) {
            if (this._metadata[ch].unit !== 'binary') {
                this._data[ch] = new AggregatingBuffer(Float32Array, this._size, this._levels, this._aggregation_factor);
            }
        }
    }

    // add new data from a decoded data message
    add(message) {
        // validate metadata of incoming data
        if (this._metadata.length !== message.metadata.length) {
            throw Error('metadata mismatch of data cache and incoming data!');
        }

        // append message arrays to cache
        this._time.add(message.time);
        this._digital.add(message.digital);

        for (const ch in message.data) {
            if (message.time.length == message.data[ch].length) {
                this._data[ch].add(message.data[ch]);
            } else {
                // interleave sub-sampled data with NaN
                const ratio = Math.floor(message.time.length / message.data[ch].length);
                const data = new Float32Array(message.time.length).map((_, i) =>
                    i % ratio == 0 ? message.data[ch][i / ratio] : NaN);
                this._data[ch].add(data);
            }
        }
    }

    /// get data for values before `time_reference`, limit to most recent `limit` number of values
    get(time_reference, limit = 0) {
        this._debug(`cache read: time_reference=${time_reference}, limit=${limit}`);
        const reply = {
            metadata: {},
            time: null,
            data: {},
            digital: null,
        };

        // find cache buffer index of first already available data element
        const cache_time_view = this._time.getView();
        const index_start = cache_time_view.findIndex(value => !isNaN(value));
        const index_end = index_start + cache_time_view.subarray(index_start).findIndex(value => value >= time_reference);

        // check for and return on cache miss
        if (index_start < 0 || index_end <= index_start) {
            this._debug('cache miss: return empty reply');
            return reply;
        }

        // assemble data reply message from cache
        const index_start_reply = limit ? Math.max(index_start, index_end - limit) : index_start;
        this._debug(`cache hit: return data range ${index_start_reply}:${index_end}`);

        reply.metadata = this._metadata;
        reply.time = cache_time_view.slice(index_start_reply, index_end);

        for (const ch in this._metadata) {
            if (this._metadata[ch].unit === 'binary') {
                continue;
            }

            const cache_data_view = this._data[ch].getView();
            reply.data[ch] = cache_data_view.slice(index_start_reply, index_end);
        }

        const cache_digital_view = this._digital.getView();
        reply.digital = cache_digital_view.slice(index_start_reply, index_end);

        return reply;
    }
}
