"use strict";

export { buffer_init, buffer_add, buffer_get_view };


function buffer_init(buffer, TypedArrayT, size, levels, aggregation_factor, initial_value = null) {
    buffer.size = size;
    buffer.levels = levels;
    buffer.aggregation_factor = aggregation_factor;
    buffer.data = new TypedArrayT(buffer.size * buffer.levels);
    if (initial_value !== null) {
        buffer.data.fill(initial_value);
    }
    buffer.level = [];
    for (let i = 0; i < buffer.levels; i++) {
        buffer.level[i] = buffer.data.subarray(i * buffer.size, (i + 1) * buffer.size);
    }
}

function buffer_add(buffer, data) {
    for (let i = 1; i <= buffer.levels; i++) {
        if (i == buffer.levels) {
            // enqueue new data
            typedarray_enqueue(buffer.level[i - 1], data);
            break;
        }

        // aggregate data about to be dequeued to next lower buffer
        const aggregate_count = data.length / (buffer.aggregation_factor ** (buffer.levels - i));
        typedarray_enqueue_aggregate(buffer.level[i - 1], buffer.level[i], aggregate_count, buffer.aggregation_factor);
    }
}

function buffer_get_view(buffer) {
    return buffer.data;
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
