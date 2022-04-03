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

/// data buffer length to buffer locally
const RL_DATA_BUFFER_LENGTH = 1e6;
/// data buffer initalization interval for requesting cached data [ms]
const RL_DATA_INIT_INTERVAL = 3000;
/// data points to sample for plotting
const RL_PLOT_POINTS = 10000;
/// maximum plot update rate [frames/sec]
const RL_PLOT_MAX_FPS = 50;
/// interval for server timesync [ms]
const RL_TIMESYNC_INTERVAL_MS = 60e3;

/// Measurement data cache size [in number of timestamps]
const data_store_size = 10000;

/// Number of buffer levels
const data_store_buffer_levels = 3;

/// Aggregation factor between data cache levels
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
    rl.data = () => {
        const time_view = buffer_get_valid_view(rl._data.time);
        const req = {
            cmd: 'data',
            time: time_view.length ? time_view[0] : null,
        };
        console.log(`request cached data, time: ${req.time === null ? 'null' : (new Date(req.time)).toISOString()}`);
        rl._data.socket.emit('data', req);
    }

    // provide plot.start() and plot.stop() methods
    rl.plot.start = async () => {
        await update_plots();
    };
    rl.plot.stop = async () => {
        clearTimeout(rl.plot.timeout);
        rl.plot.timeout = null;
        await rl.plot.plotting;
    };

    // init data update callback
    rl._data.socket.on('data', (res) => {
        // console.log(`rl data: t=${res.t}, ${res.metadata}`);
        // process data trigger plot update if enabled
        process_data(res);
        if ((rl.plot.timeout === null) && $('#plot_update').prop('checked')) {
            rl.plot.start();
            $('#collapseConfiguration').collapse('hide');
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

    ts.send = (socket, req, timeout) => {
        // console.log(`timesync send: ${JSON.stringify(req)}`);
        return new Promise((resolve, reject) => {
            let sync_timeout = setTimeout(reject, timeout);
            socket.emit('timesync', req, (res) => {
                // console.log(`timesync response: ${JSON.stringify(res)}`);
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
// cached data on initial load: prepend -> validate self-restructuring by aggregation (?)
function process_data(res) {
    // reset and initialize metadata, buffers and plots
    if (rl._data.reset) {
        rl._data.metadata = res.metadata;
        rl._data.time = {};
        buffer_init(rl._data.time, Float64Array, NaN);
        rl._data.buffer = {};
        for (const ch in rl._data.metadata) {
            rl._data.buffer[ch] = {};
            buffer_init(rl._data.buffer[ch], Float32Array, NaN);
        }
        for (const plot of rl.plot.plots) {
            Plotly.purge(plot.id);
            plot.data = null;
        }
        /// TODO: only if not self-requested measurement start/(no data received yet)
        // lazy request historical data
        setTimeout(rl.data, RL_DATA_INIT_INTERVAL);
        rl._data.reset = false;
    }

    // handle timestamp buffer, drop overflow values
    const time_view = new Float64Array(res.time);
    if (time_view.length === 0) {
        console.log('cache miss, skip data processing');
        return;
    }

    // check cached or updated data, reject processing if partially overlapping
    let cached_data = false;
    const buffer_time_view = buffer_get_valid_view(rl._data.time);
    if (buffer_time_view.length === 0) {
        /* initial data update */
    } else if (time_view[0] > buffer_time_view[buffer_time_view.length - 1]) {
        /* data update */
    } else if (time_view[time_view.length - 1] < buffer_time_view[0]) {
        cached_data = true;
        // lazy request historical data
        setTimeout(rl.data, RL_DATA_INIT_INTERVAL);
        console.log(`cache hit:   [${(new Date(time_view[0])).toISOString()}, ${(new Date(time_view[time_view.length - 1])).toISOString()}], size=${time_view.length}`);
    } else {
        console.warn('overlapping data, reject data processing');
        // console.log(`data buffer: [ ${(new Date(rl._data.time[0])).toISOString()}, ${(new Date(rl._data.time[rl._data.time.length - 1])).toISOString()} ]`);
        // console.log(`data sent:   [ ${(new Date(time_view[0])).toISOString()}, ${(new Date(time_view[time_view.length - 1])).toISOString()} ]`);
        return;
    }

    // process data
    if (cached_data) {
        console.warn("processing cache data not yet implemented!");
        /// TODO: implementation idea: prepend to existing data within the buffer level
        ///       a) only on the buffer level the earliest available data value identified for the cached data request
        ///       b) dropping any overflowing values and request new cached data for next lower level
    } else {
        buffer_add(rl._data.time, time_view);
    }

    // decode channel data
    for (const ch in rl._data.metadata) {
        let data_view;
        if (rl._data.metadata[ch].unit === 'binary') {
            /// TODO: evaluate potential optimization using buffer_add() with conversion capability
            const digital_view = new Uint8Array(res.digital);
            data_view = new Float32Array(digital_view.length);
            for (let i = 0; i < digital_view.length; i++) {
                data_view[i] = 
                    ((digital_view[i] & (0x01 << rl._data.metadata[ch].bit)) ? 0.7 : 0)
                    + rl._data.metadata[ch].bit;
            }
        }
        else {
            data_view = new Float32Array(res.data[ch]);
        }

        if (cached_data) {
            console.warn("processing cache data not yet implemented!");
            /// see above
        } else {
            buffer_add(rl._data.buffer[ch], data_view);
        }
    }
}


/// initialize an analog data plot
function plot_get_xlayout(time_scale) {
    const now = Date.now() + rl._data.t_offset;

    /// default x-axis configuration
    const xaxis = {
        range: [now - 10 * time_scale, now],
        linewidth: 1,
        mirror: true,
        tickvals: [],
        ticktext: [],
        ticklen: 5,
        tickwidth: 1,
        tickfont: { size: 14, },
        fixedrange: true,
    };

    // generate x-ticks
    for (let i = -10; i <= 0; i++) {
        const tick = time_scale * (Math.floor(now / time_scale) + i)
        xaxis.tickvals.push(tick);
        xaxis.ticktext.push(time_to_string(new Date(tick)));
    }

    return xaxis;
}

/// initialize an analog data plot
function plot_get_ylayout(plot_config) {
    /// default y-axis configuration
    const yaxis = {
        linewidth: 1,
        mirror: true,
        ticklen: 5,
        tickwidth: 1,
        tickfont: { size: 14, },
        autorange: true,
        fixedrange: true,
        showgrid: true,
        zeroline: true,
    };

    // plot type dependent y-axis format
    if (plot_config.unit === 'binary') {
        yaxis.autorange = false;
        yaxis.range = [-0.15, 5.85];
        yaxis.tickvals = [0, 0.7, 1, 1.7, 2, 2.7, 3, 3.7, 4, 4.7, 5, 5.7];
        yaxis.ticktext = ['LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI'];
        yaxis.zeroline = false;
    } else {
        yaxis.autorange = (plot_config.range == 0);
        yaxis.range = [-plot_config.range, +plot_config.range];
    }

    return yaxis;
}

/// initialize an analog data plot
function plot_get_base_layout() {
    /// default plotly layout configuration
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

/// initialize plots
async function init_plots() {
    // set default timescale
    rl.plot.time_scale = 1000;

    // register plots to update
    rl.plot.plots.push({
        id: 'plot_voltage',
        range_control: $('#plot_voltage_range'),
        range: 0,
        unit: 'V',
    });
    rl.plot.plots.push({
        id: 'plot_current',
        range_control: $('#plot_current_range'),
        range: 0,
        unit: 'A',
    });
    rl.plot.plots.push({
        id: 'plot_digital',
        range_control: null,
        range: null,
        unit: 'binary',
    });
    rl.plot.plots.push({
        id: 'plot_ambient',
        range_control: null,
        range: 0,
        unit: 'ambient',
    });

    const xaxis = plot_get_xlayout(rl.plot.time_scale);
    for (const plot of rl.plot.plots) {
        // init new plot with default layout
        const layout = plot_get_base_layout(plot);
        layout.xaxis = xaxis;
        layout.yaxis = plot_get_ylayout(plot);
        Plotly.newPlot(plot.id, [], layout, { responsive: true, displayModeBar: false });

        // init range control callback handlers
        if (plot.range_control) {
            plot.range_control.on('change', () => {
                plot.range = plot.range_control.val();
            }).trigger('change');
        }
    }
    // init time control callback handlers
    $('#plot_time_scale').on('change', () => {
        rl.plot.time_scale = $('#plot_time_scale').val();
    }).trigger('change');
}

/// async update of all plots with new data
async function update_plots() {
    // rate limit plotting
    if (rl.plot.plotting !== null) {
        await rl.plot.plotting;
    }
    rl.plot.timeout = setTimeout(update_plots, 1000 / rl.plot.update_rate);

    // update plots
    const xaxis = plot_get_xlayout(rl.plot.time_scale);
    const update_plot = async (plot) => {
        // initialize plot data array
        if (!plot.data) {
            plot.data = [];
        }

        let layout = {
            xaxis: xaxis,
        };

        const time_view = buffer_get_valid_view(rl._data.time);

        // collect data series to plot
        for (const ch in rl._data.buffer) {
            const meta = rl._data.metadata[ch];
            const buffer_view = buffer_get_valid_view(rl._data.buffer[ch]);
            let trace = null;

            // find existing channel trace or initialize and add new
            const trace_index = plot.data.findIndex((value) => value.name === ch);
            if (trace_index < 0) {
                trace = {
                    type: 'scattergl',
                    mode: 'lines',
                    name: ch,
                    hoverinfo: 'y+name',
                };
                plot.data.push(trace);
            } else {
                trace = plot.data[trace_index];
            }

            // check if channel of ambient plot
            if ((plot.unit === 'ambient') && (meta.unit !== 'A') &&
                (meta.unit !== 'V') && (meta.unit !== 'binary')) {
                // time scale dependent display mode
                if (rl.plot.time_scale > 1e3) {
                    trace.mode = 'lines';
                } else {
                    trace.mode = 'markers';
                }

                const start_index = Math.max(0, buffer_view.length - 11 * rl.plot.time_scale);
                trace.x = time_view.subarray(start_index);
                trace.y = buffer_view.subarray(start_index);
            } else if (meta.unit === plot.unit) {
                const start_index = Math.max(0, buffer_view.length - 10 * rl.plot.time_scale);
                trace.x = time_view.subarray(start_index);
                trace.y = buffer_view.subarray(start_index);
            } else {
                continue;
            }
        }

        // check for existing traces
        if (document.getElementById(plot.id).data) {
            layout.yaxis = plot_get_ylayout(plot);
            Plotly.update(plot.id, plot.data, layout);
        } else {
            // init new plot with default layout
            layout = plot_get_base_layout(plot);
            layout.xaxis = xaxis;
            layout.yaxis = plot_get_ylayout(plot);
            Plotly.newPlot(plot.id, plot.data, layout, { responsive: true, displayModeBar: false });
        }
    };
    if (rl.plot.plots) {
        rl.plot.plotting = Promise.all(rl.plot.plots.map((plot) => update_plot(plot)));
    }
}

/**
 * Initialization when document is fully loaded.
 */
$(() => {
    // initialize RocketLogger data and plot functionality
    rocketlogger_init_data();

    // init plots
    init_plots();

    // plot update change handler if enabled
    $('#plot_update').on('change', () => {
        if (!$('#plot_update').prop('checked')) {
            rl.plot.stop();
        }
    });
    $('#plot_update_rate').on('change', () => {
        rl.plot.update_rate = Math.min(RL_PLOT_MAX_FPS, $('#plot_update_rate').val());
    }).trigger('change');

    // register plotting hotkeys
    $(document).on('keypress', (event) => {
        if (event.target.nodeName === 'INPUT' || event.target.nodeName === 'CHECKBOX' ||
            event.target.nodeName === 'TEXTAREA') {
            return;
        }

        switch (event.which) {
            case ascii('p'):
            case ascii(' '):
                $('#plot_update').trigger('click');
                break;
            case ascii('1'):
                $('#plot_time_scale').val(1000).trigger('change');
                break;
            case ascii('2'):
                $('#plot_time_scale').val(10000).trigger('change');
                break;
            case ascii('3'):
                $('#plot_time_scale').val(100000).trigger('change');
                break;
            default:
                return;
        }
        event.preventDefault();
    });
});


function buffer_init(buffer, TypedArrayT, initial_value = null) {
    buffer.data = new TypedArrayT(data_store_size * data_store_buffer_levels);
    if (initial_value !== null) {
        buffer.data.fill(initial_value);
    }
    buffer.level = [];
    for (let i = 0; i < data_store_buffer_levels; i++) {
        buffer.level[i] = buffer.data.subarray(i * data_store_size, (i + 1) * data_store_size);
    }
}

function buffer_add(buffer, data) {
    for (let i = 1; i <= data_store_buffer_levels; i++) {
        if (i == data_store_buffer_levels) {
            // enqueue new data
            typedarray_enqueue(buffer.level[i - 1], data);
            break;
        }

        // aggregate data about to be dequeued to next lower buffer
        const aggregate_count = data.length / (data_store_aggregation_factor ** (data_store_buffer_levels - i));
        typedarray_enqueue_aggregate(buffer.level[i - 1], buffer.level[i], aggregate_count, data_store_aggregation_factor);
    }
}

function buffer_get_view(buffer) {
    return buffer.data;
}

function buffer_get_valid_view(buffer) {
    const index_start = buffer_get_view(buffer).findIndex((value) => !isNaN(value));
    if (index_start < 0) {
        return buffer.data.subarray(0, 0);
    }
    return buffer.data.subarray(index_start);
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
