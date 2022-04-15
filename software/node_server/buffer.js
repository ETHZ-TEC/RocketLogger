"use strict";

export { AggregatingBuffer };


class AggregatingBuffer {
    constructor(TypedArrayT, size, levels, aggregation_factor, initial_value = null) {
        this._size = size;
        this._levels = levels;
        this._aggregation_factor = aggregation_factor;
        this._data = new TypedArrayT(this._levels * this._size);
        this._dataLevel = Array.from({ length: this._levels },
            (_, i) => this._data.subarray(i * this._size, (i + 1) * this._size));
        if (initial_value !== null) {
            this._data.fill(initial_value);
        }
    }

    add(data) {
        for (let i = 1; i <= this._levels; i++) {
            if (i == this._levels) {
                // enqueue new data
                typedarray_enqueue(this._dataLevel[i - 1], data);
                break;
            }

            // aggregate data about to be dequeued to next lower buffer
            const aggregate_count = data.length / (this._aggregation_factor ** (this._levels - i));
            typedarray_enqueue_aggregate(this._dataLevel[i - 1], this._dataLevel[i], aggregate_count, this._aggregation_factor);
        }
    }

    getView() {
        return this._data;
    }
}

// enqueue typed array data at end of typed array buffer
function typedarray_enqueue(buffer_out, buffer_in) {
    buffer_out.set(buffer_out.subarray(buffer_in.length));
    buffer_out.set(buffer_in, buffer_out.length - buffer_in.length);
}

// enqueue aggregates of a typed array at end of typed array buffer
function typedarray_enqueue_aggregate(buffer_out, buffer_in, count, aggregation_factor) {
    buffer_out.set(buffer_out.subarray(count));
    aggregate(buffer_out.subarray(buffer_out.length - count), buffer_in, aggregation_factor);
}

// typed array to typed array sample aggregation
function aggregate(buffer_out, buffer_in, factor) {
    for (let i = 0; i < buffer_out.length; i++) {
        buffer_out[i] = buffer_in[i * factor];
    }
}
