"use strict";

// imports
import debug from 'debug';
import { AggregatingDataStore, AggregatingBinaryStore } from './buffer.js';

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
        this._time = new AggregatingDataStore(Float64Array, this._size, this._levels, this._aggregation_factor);
        this._digital = new AggregatingBinaryStore(Uint16Array, this._size, this._levels, this._aggregation_factor);
        this._data = {};
        for (const ch in this._metadata) {
            if (this._metadata[ch].unit !== 'binary') {
                this._data[ch] = new AggregatingDataStore(Float32Array, this._size, this._levels, this._aggregation_factor);
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
            let data = message.data[ch];
            // replace/interleave sub-sampled data with NaN
            if (data.length == 0) {
                data = new Float32Array(message.time.length).fill(NaN);
            } else if (data.length < message.time.length) {
                const ratio = Math.floor(message.time.length / data.length);
                data = Float32Array.from({ length: message.time.length }, (_, i) =>
                    i % ratio === 0 ? data[i / ratio] : NaN);
            }
            this._data[ch].add(data);
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
        const cache_time_view = this._time.getValidView();
        const index = cache_time_view.findIndex(value => value >= time_reference);
        this._debug(`matching cache range: ${index}:${cache_time_view.length}`);

        // check for and return on cache miss
        if (index < 0) {
            this._debug('cache miss: return empty reply');
            return reply;
        }

        // assemble data reply message from cache
        const end = index - cache_time_view.length;
        const start = limit ? end - limit : -cache_time_view.length;
        this._debug(`cache hit: return data range ${start}:${end}`);

        reply.metadata = this._metadata;
        reply.time = cache_time_view.slice(start, end);

        for (const ch in this._metadata) {
            if (this._metadata[ch].unit === 'binary') {
                continue;
            }

            const cache_data_view = this._data[ch].getView();
            reply.data[ch] = cache_data_view.slice(start, end);
        }

        const cache_digital_view = this._digital.getView();
        reply.digital = cache_digital_view.slice(start, end);

        return reply;
    }
}
