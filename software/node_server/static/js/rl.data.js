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

// check RocketLogger base functionality is loaded
if (typeof rl === 'undefined') {
    throw Error('need to load rl.base.js before loading rl.data.js');
}

/// data buffer initialization interval for requesting cached data [ms]
const RL_DATA_INIT_INTERVAL = 3000;
/// maximum plot update rate [frames/sec]
const RL_PLOT_MAX_FPS = 50;
/// interval for server timesync [ms]
const RL_TIMESYNC_INTERVAL_MS = 60e3;
/// plot color scheme (Plotly.js default)
const RL_PLOT_COLORS = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf'];


/// Measurement data buffer size [in number of elements]
const data_store_size = 10000;

/// Number of buffer levels
const data_store_buffer_levels = 3;

/// Aggregation factor between data buffer levels
const data_store_aggregation_factor = 10;


/// initialize RocketLogger data and plot functionality
function rocketlogger_init_data() {
    // check RocketLogger base functionality is initialized
    if (typeof rl.status === 'undefined') {
        throw Error('need RocketLogger base functionality to be initialized first.');
    }

    // force initial reset of data buffers
    rl._data.reset = true;

    // initialize plotting data structure
    rl.plot = {
        plots: [],
        update_rate: RL_PLOT_MAX_FPS,
        plotting: null,
        timeout: null,
    };

    // provide data() method to poll cached data
    rl.data = async () => {
        const time_view = rl._data.time.getValidView();
        const request = {
            cmd: 'data',
            time: time_view.length ? time_view[0] : null,
        };
        console.log(`request cached data, time: ${request.time === null ? 'null' : new Date(request.time).toISOString()}`);
        rl._data.socket.emit('data', request);
    }

    // provide plot.start() and plot.stop() methods
    rl.plot.start = async () => {
        await plots_update();
    };
    rl.plot.stop = async () => {
        clearTimeout(rl.plot.timeout);
        rl.plot.timeout = null;
        await rl.plot.plotting;
    };

    // init data update callback
    rl._data.socket.on('data', (reply) => {
        // console.log(`rl data: t=${reply.t}, ${reply.metadata}`);
        // process data trigger plot update if enabled
        process_data(reply);
        if (rl.plot.timeout === null && document.querySelector('#plot_update').checked) {
            rl.plot.start();
            new bootstrap.Collapse(document.querySelector('#collapseConfiguration'), { toggle: false }).hide();
        }
    });

    // setup timesync
    const ts = timesync.create({
        server: rl._data.socket,
        interval: RL_TIMESYNC_INTERVAL_MS
    });

    ts.on('change', (offset) => {
        console.log(`server timesync update: ${offset} ms`);
        rl._data.t_offset = offset;
    });

    ts.send = async (socket, request, timeout) => {
        // console.log(`timesync send: ${JSON.stringify(request)}`);
        return new Promise((resolve, reject) => {
            const sync_timeout = setTimeout(reject, timeout);
            socket.emit('timesync', request, (_) => {
                // console.log(`timesync response: ${JSON.stringify(reply)}`);
                clearTimeout(sync_timeout);
                resolve();
            });
        });
    };

    // server timesync callback
    rl._data.socket.on('timesync', (data) => {
        ts.receive(null, data);
    });
}

/// process new measurement data
function process_data(reply) {
    // check received timestamp buffer, drop overflow values
    const time_view = new Float64Array(reply.time);
    if (time_view.length === 0) {
        console.log('cache miss, skip data processing');
        return;
    }

    // check cached or updated data, skip processing if partially overlapping
    let reset = rl._data.reset || (reply.reset) ? rl._data.reset : false;
    let cached_data = false;
    if (rl._data.time) {
        const buffer_time_view = rl._data.time.getValidView();
        if (time_view[0] > buffer_time_view[buffer_time_view.length - 1]) {
            // data update
        } else if (time_view[time_view.length - 1] < buffer_time_view[0]) {
            // data update from cache -> cache processing and lazy request additional historical data 
            cached_data = true;
            setTimeout(rl.data, RL_DATA_INIT_INTERVAL);
            console.log(`cache hit:   [${(new Date(time_view[0])).toISOString()}, ${(new Date(time_view[time_view.length - 1])).toISOString()}], size=${time_view.length}`);
        } else {
            console.warn('data overlapping with buffer received');
        }
    } else {
        // initial data update -> reset and lazy request historical data
        reset = true;
        setTimeout(rl.data, RL_DATA_INIT_INTERVAL);
    }

    // check for reset and data store and plots
    if (reset) {
        data_reset(reply.metadata);
        plots_reset();
    }

    // process data
    data_add(reply, cached_data);
}


/// initialize plots
function plots_init() {
    // set default timescale
    rl.plot.time_scale = 1000;

    // register plots to update
    rl.plot.plots = [
        {
            element: document.querySelector('#plot_voltage'),
            range_control: document.querySelector('#plot_voltage_range'),
            range: 0,
            unit: 'V',
        },
        {
            element: document.querySelector('#plot_current'),
            range_control: document.querySelector('#plot_current_range'),
            range: 0,
            unit: 'A',
        },
        {
            element: document.querySelector('#plot_digital'),
            range_control: null,
            range: null,
            unit: 'binary',
        },
        {
            element: document.querySelector('#plot_ambient'),
            range_control: null,
            range: 0,
            unit: 'ambient',
        },
    ];

    plots_reset();

    // register range control handlers for plots
    for (const plot of rl.plot.plots) {
        if (plot.range_control) {
            plot.range_control.addEventListener('change', () => {
                plot.range = plot.range_control.value;
            });
            triggerEvent(plot.range_control, 'change');
        }
    }

    // register time scale control handler
    document.querySelector('#plot_time_scale').addEventListener('change', () => {
        rl.plot.time_scale = document.querySelector('#plot_time_scale').value;
    });
    triggerEvent('#plot_time_scale', 'change');
}

async function plots_reset() {
    const xaxis = plot_get_xlayout(rl.plot.time_scale);
    return Promise.all(rl.plot.plots.map(plot => plot_reset(plot, xaxis)));
}

/// async update of all plots with new data
async function plots_update() {
    // rate limit plotting
    if (rl.plot.plotting !== null) {
        await rl.plot.plotting;
    }
    rl.plot.timeout = setTimeout(plots_update, 1000 / rl.plot.update_rate);

    // update plots
    const xaxis = plot_get_xlayout(rl.plot.time_scale);
    if (rl.plot.plots) {
        rl.plot.plotting = Promise.all(rl.plot.plots.map(plot => plot_update(plot, xaxis)));
    }
}

/// default plotly layout configuration
function plot_get_base_layout() {
    const layout = {
        font: { size: 16, },
        hovermode: 'x',
        showlegend: true,
        legend: {
            x: 0.0,
            xanchor: 'left',
            y: 1.01,
            yanchor: 'bottom',
            orientation: 'h',
            font: { size: 18, },
        },
        margin: {
            l: 60,
            r: 20,
            t: 5,
            b: 50,
            autoexpand: true,
        }
    };

    return layout;
}

/// initialize an analog data plot
function plot_get_xlayout(time_scale) {
    const now = Date.now() + rl._data.t_offset;
    const ticks = Array.from({ length: 11 }, (_, i) => time_scale * (Math.floor(now / time_scale) - 10 + i));

    const xaxis = {
        range: [now - 10 * time_scale, now],
        linewidth: 1,
        mirror: true,
        tickvals: ticks,
        ticktext: ticks.map(tick => time_to_string(new Date(tick))),
        ticklen: 5,
        tickwidth: 1,
        tickfont: { size: 14, },
        fixedrange: true,
    };

    return xaxis;
}

/// initialize an analog data plot
function plot_get_ylayout(plot_config) {
    const yaxis = {
        linewidth: 1,
        mirror: true,
        ticklen: 5,
        tickwidth: 1,
        tickfont: { size: 14, },
        autorange: plot_config.range == 0,
        fixedrange: true,
        range: [-plot_config.range, +plot_config.range],
        showgrid: true,
        zeroline: true,
    };

    // binary plot specific y-axis format
    if (plot_config.unit === 'binary') {
        yaxis.autorange = false;
        yaxis.range = [-0.15, 5.85];
        yaxis.tickvals = [0, 0.66, 1, 1.66, 2, 2.66, 3, 3.66, 4, 4.66, 5, 5.66];
        yaxis.ticktext = ['LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI'];
        yaxis.zeroline = false;
    }

    return yaxis;
}

async function plot_reset(plot, xaxis) {
    // purge plot and reset traces
    Plotly.purge(plot.element);
    plot.data = [];
    let trace_index = 0;

    // collect data series to plot
    for (const ch in rl._data.buffer) {
        const meta = rl._data.metadata[ch];

        // skip channels of other plots
        if (plot.unit === 'ambient') {
            const non_ambient = rl.plot.plots.some(value => value.unit === meta.unit);
            if (non_ambient) {
                continue;
            }
        } else {
            if (plot.unit !== meta.unit) {
                continue;
            }
        }

        // assemble and add trace config
        const trace = {
            type: 'scattergl',
            mode: 'lines',
            line: {
                color: RL_PLOT_COLORS[trace_index % RL_PLOT_COLORS.length],
            },
            connectgaps: true,
            name: ch,
            hoverinfo: 'y+name',
            x: rl._data.time.getView(),
            y: rl._data.buffer[ch].getView(),
        };
        if (plot.unit == 'binary') {
            const trace_name = ch.replace('min', '').replace('max', '');
            trace.legendgroup = trace_name;
            trace.showlegend = false;
            if (ch.endsWith('min')) {
                trace_index -= 1;
                const legend_entry_trace = {
                    type: 'scattergl',
                    mode: 'lines',
                    line: trace.line,
                    name: trace_name,
                    legendgroup: trace_name,
                    hoverinfo: 'skip',
                    x: [NaN],
                    y: [NaN],
                };
                plot.data.push(legend_entry_trace);
            }
            if (ch.endsWith('max')) {
                trace.fill = 'tonexty';
            }
        }
        plot.data.push(trace);
        trace_index += 1;
    }

    // init new plot with default layout
    const layout = plot_get_base_layout();
    layout.xaxis = xaxis;
    layout.yaxis = plot_get_ylayout(plot);
    Plotly.newPlot(plot.element, plot.data, layout, { responsive: true, displayModeBar: false });
}

async function plot_update(plot, xaxis) {
    if (!plot.element?.data) {
        console.warn('skip update of un-initialized plot');
        return;
    }

    const time_view = rl._data.time.getView();
    const index_start = time_view.findIndex(value => value >= xaxis.range[0]);
    for (const trace of plot.data) {
        if (!(trace.name in rl._data.buffer)) {
            continue;
        }
        // get range of values to plot
        const buffer_view = rl._data.buffer[trace.name].getView();
        trace.x = time_view.subarray(index_start);
        trace.y = buffer_view.subarray(index_start);

        // time scale dependent display mode
        if (plot.unit === 'ambient') {
            if (rl.plot.time_scale > 1e3) {
                trace.mode = 'lines';
            } else {
                trace.mode = 'markers';
            }
        }
    }

    const layout_update = {
        xaxis: xaxis,
        yaxis: plot_get_ylayout(plot),
    };
    Plotly.update(plot.element, plot.data, layout_update);
}

/// initialize when document is fully loaded
window.addEventListener('load', () => {
    // initialize RocketLogger data and plot functionality
    rocketlogger_init_data();

    // init plots
    plots_init();

    // plot update change handler if enabled
    document.querySelector('#plot_update').addEventListener('change', () => {
        if (!document.querySelector('#plot_update').checked) {
            rl.plot.stop();
        }
    });

    document.querySelector('#plot_update_rate').addEventListener('change', () => {
        rl.plot.update_rate = Math.min(RL_PLOT_MAX_FPS, document.querySelector('#plot_update_rate').value);
    });
    triggerEvent('#plot_update_rate', 'change');

    // register plotting hotkeys
    document.addEventListener('keydown', (event) => {
        // skip event of form inputs
        if (event.target.nodeName === 'INPUT' || event.target.nodeName === 'CHECKBOX' ||
            event.target.nodeName === 'TEXTAREA') {
            return;
        }
        // skip event with pressed modifier keys
        if (event.altKey || event.ctrlKey || event.metaKey || event.shiftKey) {
            return;
        }

        switch (event.key) {
            case ' ':
            case 'p':
                document.querySelector('#plot_update').click();
                break;
            case '1':
            case '2':
            case '3':
                document.querySelector('#plot_time_scale').selectedIndex = event.key - '1';
                triggerEvent('#plot_time_scale', 'change');
                break;
            default:
                return;
        }
        event.preventDefault();
    });
});


function data_reset(metadata) {
    rl._data.metadata = metadata;
    rl._data.time = new AggregatingDataStore(Float64Array, data_store_size, data_store_buffer_levels, data_store_aggregation_factor);
    rl._data.buffer = {};
    for (const ch in rl._data.metadata) {
        if (rl._data.metadata[ch].unit === 'binary') {
            const chMin = ch + 'min';
            rl._data.metadata[chMin] = { ...rl._data.metadata[ch] };
            rl._data.buffer[chMin] = new AggregatingDataStore(Float32Array, data_store_size, data_store_buffer_levels, data_store_aggregation_factor);

            const chMax = ch + 'max';
            rl._data.metadata[chMax] = { ...rl._data.metadata[ch] };
            rl._data.metadata[chMax].bit += 8;
            rl._data.buffer[chMax] = new AggregatingDataStore(Float32Array, data_store_size, data_store_buffer_levels, data_store_aggregation_factor);

            delete rl._data.metadata[ch];
        } else {
            rl._data.buffer[ch] = new AggregatingDataStore(Float32Array, data_store_size, data_store_buffer_levels, data_store_aggregation_factor);
        }
    }
    rl._data.reset = false;
}

function data_add(message, cache_data = false) {
    const insert = (buffer, data) => cache_data ? buffer.prepend(data) : buffer.add(data);

    const time_view = new Float64Array(message.time);
    insert(rl._data.time, time_view);

    // decode channel data
    for (const ch in rl._data.metadata) {
        if (rl._data.metadata[ch].unit === 'binary') {
            const bit_offset = rl._data.metadata[ch].bit;
            const data_digital = Float32Array.from(new Uint16Array(message.digital),
                value => (bit_offset % 8) + ((value & (0x0001 << bit_offset)) ? 0.66 : 0));
            insert(rl._data.buffer[ch], data_digital);
        } else {
            const data_view = new Float32Array(message.data[ch]);
            if (time_view.length == data_view.length) {
                insert(rl._data.buffer[ch], data_view);
            } else {
                // interleave sub-sampled data with NaN
                const ratio = Math.floor(time_view.length / data_view.length);
                const data = new Float32Array(time_view.length).map((_, i) =>
                    i % ratio == 0 ? data_view[i / ratio] : NaN);
                insert(rl._data.buffer[ch], data);
            }
        }
    }
}


class AggregatingBuffer {
    constructor(TypedArrayT, size, levels, aggregation_factor, initial_value = 0) {
        if (!TypedArrayT.hasOwnProperty('BYTES_PER_ELEMENT')) {
            throw TypeError('supporting TypedArray types only');
        }
        this._size = size;
        this._levels = levels;
        this._aggregation_factor = aggregation_factor;
        this._data = new TypedArrayT(this._levels * this._size).fill(initial_value);
        this._dataLevel = Array.from({ length: this._levels },
            (_, i) => this._data.subarray(i * this._size, (i + 1) * this._size));
    }

    add(data) {
        // aggregate data about to be dequeued to next lower buffer
        for (let i = 1; i < this._levels; i++) {
            const aggregate_count = data.length / (this._aggregation_factor ** (this._levels - i));
            this.constructor._enqueue_aggregate(this._dataLevel[i], this._dataLevel[i - 1], aggregate_count, this._aggregation_factor);
        }

        // enqueue new data
        this.constructor._enqueue(data, this._dataLevel[this._levels - 1]);
    }

    getView() {
        return this._data;
    }

    // enqueue typed array data at end of typed array buffer
    static _enqueue(buffer_in, buffer_out) {
        buffer_out.set(buffer_out.subarray(buffer_in.length));
        buffer_out.set(buffer_in, buffer_out.length - buffer_in.length);
    }

    // enqueue aggregates of a typed array at end of typed array buffer
    static _enqueue_aggregate(buffer_in, buffer_out, count, aggregation_factor) {
        buffer_out.set(buffer_out.subarray(count));
        for (let i = 0; i < count; i++) {
            buffer_out[buffer_out.length - count + i] = buffer_in[i * aggregation_factor];
        }
    }
}


class AggregatingDataStore extends AggregatingBuffer {
    constructor(TypedArrayT, size, levels, aggregation_factor) {
        if (!TypedArrayT.name.startsWith('Float')) {
            throw TypeError('supports only floating point types');
        }
        super(TypedArrayT, size, levels, aggregation_factor, NaN);
        this._start_index = this._data.length;
    }

    prepend(data) {
        let index_start = this._get_start_index();
        for (let i = 0; i < this._levels; i++) {
            // skip to next non-full buffer level
            if (index_start > this._size) {
                index_start -= this._size;
                continue;
            }

            // insert data limited to non-full buffer level
            const insert_size = Math.min(index_start, data.length);
            this._dataLevel[i].set(data.subarray(data.length - insert_size), index_start - insert_size);
            return;
        }
        console.warn('failed inserting cached data into buffer');
    }

    getValidView() {
        const index_start = this._get_start_index();
        return this._data.subarray(index_start);
    }

    _get_start_index() {
        return this._start_index = this._data.findLastIndex(isNaN, this._start_index) + 1;
    }
}
