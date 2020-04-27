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
if (typeof (rl) == 'undefined') {
    throw 'need to load rl.base.js before loading rl.data.js'
}

/// data buffer length to buffer locally
const RL_DATA_BUFFER_LENGTH = 10000;
/// maximum plot update rate [frames/sec]
const RL_PLOT_MAX_FPS = 50;

/// initialize RocketLogger data and plot functionality
function rocketlogger_init_data() {
    // check RocketLogger base functionality is initialized
    if (rl.status === null) {
        throw 'need RocketLogger base functionality to be initialized first.'
    }

    // initialize data buffers
    reset_buffer();

    // initialize plotting data structure
    rl.plot = {
        plots: [],
        update_rate: RL_PLOT_MAX_FPS,
        plotting: null,
        timeout: null,
    };

    // provide data() method
    rl.data = () => {
        /// @todo init buffer from data cache
        const req = {
            cmd: 'data',
            time: Date.now(),
        };
        rl._conn.socket.emit('data', req);
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
    rl._conn.socket.on('data', (res) => {
        // console.log(`rl data: t=${res.t}, ${res.metadata}`);
        // process data trigger plot update if enabled
        process_data(res);
        if ((rl.plot.timeout == null) && $('#plot_update').prop('checked')) {
            rl.plot.start();
            $('#collapseConfiguration').collapse('hide');
        }
    });
}

/// reset local data buffers
function reset_buffer() {
    rl._data.buffer = {};
    rl._data.metadata = {};
}

/// initialize an analog data plot
function plot_get_layout(plot_config, update_only = false) {
    const now = Date.now();

    /// default plotly axis configuration
    const layout = {
        xaxis: {
            range: [now - 10 * plot_config.time_scale, now],
            linewidth: 1,
            mirror: true,
            tickvals: [],
            ticktext: [],
            ticklen: 5,
            tickwidth: 1,
            tickfont: { size: 14, },
            fixedrange: true,
        },
        yaxis: {
            linewidth: 1,
            mirror: true,
            ticklen: 5,
            tickwidth: 1,
            tickfont: { size: 14, },
            autorange: true,
            fixedrange: true,
            showgrid: true,
            zeroline: true,
        },
    };

    // generate new xticks
    for (let i = 0; i < 12; i++) {
        const tick = plot_config.time_scale * (Math.ceil(now / plot_config.time_scale) - 11 + i)
        layout.xaxis.tickvals.push(tick);
        layout.xaxis.ticktext.push(time_to_string(new Date(tick)));
    }

    // plot type dependent yaxis format
    if (plot_config.digital) {
        layout.yaxis.autorange = false;
        layout.yaxis.range = [-0.15, 5.85];
        layout.yaxis.tickvals = [0, 0.7, 1, 1.7, 2, 2.7, 3, 3.7, 4, 4.7, 5, 5.7];
        layout.yaxis.ticktext = ['LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI', 'LO', 'HI'];
        layout.yaxis.zeroline = false;
    } else {
        layout.yaxis.autorange = (plot_config.range == 0);
        layout.yaxis.range = [-plot_config.range, +plot_config.range];
    }

    if (update_only) {
        return layout;
    }

    // general plot settings
    layout.font = { size: 16, };
    layout.showlegend = true;
    layout.legend = {
        x: 0.0,
        xanchor: 'left',
        y: 1.01,
        yanchor: 'bottom',
        orientation: 'h',
        font: { size: 18, },
    };
    layout.margin = {
        l: 60,
        r: 20,
        t: 5,
        b: 50,
        autoexpand: true,
    };

    return layout;
}

/// initialize plots
async function init_plots() {
    // register plots to update
    rl.plot.plots.push({
        id: 'plot_voltage',
        digital: false,
        time_scale: 1000,
        range_control: $('#plot_voltage_range'),
        range: 0,
        unit: 'V',
    });
    rl.plot.plots.push({
        id: 'plot_current',
        digital: false,
        time_scale: 1000,
        range_control: $('#plot_current_range'),
        range: 0,
        unit: 'A',
    });
    rl.plot.plots.push({
        id: 'plot_digital',
        digital: true,
        time_scale: 1000,
        range_control: null,
        range: null,
        unit: 'digital',
    });

    for (let i = 0; i < rl.plot.plots.length; i++) {
        const p = rl.plot.plots[i];

        // init new plot with default layout
        const layout = plot_get_layout(p);
        Plotly.newPlot(p.id, [], layout, { responsive: true, displayModeBar: false });

        // init range control callback handlers
        if (p.range_control) {
            p.range_control.on('change', () => {
                p.range = p.range_control.val();
            }).trigger('change');
        }
    }
    // init time control callback handlers
    $('#plot_time_scale').on('change', () => {
        for (const plot of rl.plot.plots) {
            plot.time_scale = $('#plot_time_scale').val();
        }
    }).trigger('change');
}

/// async update of all plots with new data
async function update_plots() {
    if (rl.plot.plotting != null) {
        await rl.plot.plotting;
    }
    rl.plot.timeout = setTimeout(update_plots, 1000 / rl.plot.update_rate);

    let update_plot = async (plot) => {
        // collect data series to plot
        let data = [];
        for (const ch in rl._data.buffer) {
            // check if enabled
            if ((rl._data.metadata[ch].unit == plot.unit)) {
                data.push({
                    x: rl._data.time,
                    y: rl._data.buffer[ch],
                    type: 'scattergl',
                    mode: 'lines',
                    name: ch,
                    hoverinfo: 'y+name',
                });
            }
        }

        // check for existing traces
        if (document.getElementById(plot.id).data.length) {
            const layout = plot_get_layout(plot, true);
            Plotly.update(plot.id, data, layout);
        } else {
            const layout = plot_get_layout(plot);
            Plotly.newPlot(plot.id, data, layout, { responsive: true, displayModeBar: false });
        }
    };
    if (rl.plot.plots) {
        rl.plot.plotting = Promise.all(rl.plot.plots.map((p) => update_plot(p)));
    }
}

/// process new measurement data.
function process_data(res) {
    // clear buffers and plots if necessary
    if (rl._data.buffer_clear) {
        rl._data.buffer = {};
        rl._data.time = [];
        rl.plot.plots.forEach((p) => { document.getElementById(p.id).data = []; })
        rl._data.buffer_clear = false;
    }

    for (const ch in res.metadata) {
        // skip hidden channel
        if (res.metadata[ch].hidden) {
            continue;
        }

        // init array buffer if necessary
        if (!rl._data.buffer[ch]) {
            rl._data.buffer[ch] = [];
        }

        // decode data
        if (res.metadata[ch].digital) {
            const digital_view = new Uint8Array(res.digital);
            for (let i = 0; i < digital_view.length; i++) {
                rl._data.buffer[ch].push(
                    ((digital_view[i] & (0x01 << res.metadata[ch].bit)) ? 0.7 : 0)
                    + res.metadata[ch].bit);
            }
        } else {
            const analog_view = new Float32Array(res.data[ch]);
            for (let i = 0; i < analog_view.length; i++) {
                rl._data.buffer[ch].push(analog_view[i]);
            }
        }

        // drop old values
        if (rl._data.buffer[ch].length > RL_DATA_BUFFER_LENGTH) {
            rl._data.buffer[ch].splice(0,
                rl._data.buffer[ch].length - RL_DATA_BUFFER_LENGTH);
        }
    }

    // handle timestamp buffer
    if (!rl._data.time) {
        rl._data.time = [];
    }
    const time_view = new Float64Array(res.time);
    for (let i = 0; i < time_view.length; i++) {
        rl._data.time.push(time_view[i]);
    }
    if (rl._data.time.length > RL_DATA_BUFFER_LENGTH) {
        rl._data.time.splice(0,
            rl._data.time.length - RL_DATA_BUFFER_LENGTH);
    }

    // store metadata and set digital unit
    rl._data.metadata = res.metadata;
    for (const ch in rl._data.metadata) {
        if (rl._data.metadata[ch].digital) {
            rl._data.metadata[ch].unit = 'digital';
        }
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
        if (event.target.nodeName == 'INPUT' || event.target.nodeName == 'CHECKBOX' ||
            event.target.nodeName == 'TEXTAREA') {
            return;
        }

        switch (event.which) {
            case ascii('p'):
            case ascii(' '):
                $('#plot_update').trigger('click')
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
