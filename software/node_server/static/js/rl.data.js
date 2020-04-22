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

// check RocketLogger base functionality is loaded
if (typeof (rl) == 'undefined') {
    throw 'need to load rl.base.js before loading rl._data.js'
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
        auto_scroll: true,
        plotting: null,
        timeout: null,
    };

    // provide data() method
    rl.data = () => {
        /// @todo init buffer from data cache
        req = {
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
function plot_get_config(digital = false) {
    let now = Date.now();
    const time_base = 1000;

    /// default plot configuration
    const config = {
        series: {
            shadowSize: 0,
            lines: {
                show: true,
            },
            points: {
                show: false,
            },
            downsample: {
                threshold: 1000,
            },
        },
        xaxis: {
            show: true,
            position: 'bottom',
            mode: 'time',
            min: now - 10 * time_base,
            max: now,
            autoScale: 'none',
            ticks: Array.from({ length: 12 }, (_, i) => time_base * (Math.ceil(Date.now() / time_base) - 11 + i)),
            tickFormatter: (val, _) => {
                var d = new Date(val);
                return time_to_string(d);
            },
            axisLabel: null,
            gridLines: false,
            showMinorTicks: true,
        },
        yaxis: {
            show: true,
            position: 'left',
            axisLabel: null,
            gridLines: true,
            showMinorTicks: true,
        }
    };

    if (digital) {
        config.yaxis.min = -0.15;
        config.yaxis.max = 5.85;
        config.yaxis.autoScale = 'none';
        config.yaxis.ticks = [
            [0, 'LO'], [0.7, 'HI'], [1, 'LO'], [1.7, 'HI'],
            [2, 'LO'], [2.7, 'HI'], [3, 'LO'], [3.7, 'HI'],
            [4, 'LO'], [4.7, 'HI'], [5, 'LO'], [5.7, 'HI'],
        ];
        config.yaxis.showMinorTicks = false;
        // config.yaxis.axisLabel = "Digital";
    }
    return config;
}

/// async update of all plots with new data
async function update_plots() {
    if (rl.plot.plotting != null) {
        await rl.plot.plotting;
    }
    rl.plot.timeout = setTimeout(update_plots, 1000 / RL_PLOT_MAX_FPS);

    let update_plot = async (plot) => {
        // collect data series to plot
        let data = [];
        for (ch in rl._data.buffer) {
            // check if enabled
            if ((rl._data.metadata[ch].unit == plot.unit)) {
                data.push(rl._data.buffer[ch]);
            }
        }

        // get plot config
        const config = plot_get_config(plot.unit == 'digital');

        // update plot
        plot.placeholder.plot(data, config);
    };
    rl.plot.plotting = Promise.all(rl.plot.plots.map((p) => update_plot(p)));
}

/**
 * Process new measurement data.
 * 
 * @param {Object} res Data result structure from server
 */
function process_data(res) {
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
            const buf = new Uint32Array(res.digital);
            for (let i = 0; i < buf.length; i++) {
                rl._data.buffer[ch].push([
                    res.time[i],
                    (buf[i] & (0x01 << res.metadata[ch].bit)) ? 0.7 : 0
                        + res.metadata[ch].bit]);
            }
        } else if (res.metadata[ch].scale) {
            const buf = new Int32Array(res.data[ch]);
            for (let i = 0; i < buf.length; i++) {
                rl._data.buffer[ch].push([res.time[i], buf[i] * res.metadata[ch].scale]);
            }
        } else {
            const buf = new Int32Array(res.data[ch]);
            for (let i = 0; i < buf.length; i++) {
                rl._data.buffer[ch].push([res.time[i], buf[i]]);
            }
        }

        // drop old values
        if (rl._data.buffer[ch].length > RL_DATA_BUFFER_LENGTH) {
            rl._data.buffer[ch].splice(0,
                rl._data.buffer[ch].length - RL_DATA_BUFFER_LENGTH);
        }
    }

    // store metadata and set digital unit
    rl._data.metadata = res.metadata;
    for (ch in rl._data.metadata) {
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

    // register plots to update
    rl.plot.plots.push({
        placeholder: $('#plot_voltage'),
        unit: 'V',
    });
    rl.plot.plots.push({
        placeholder: $('#plot_current'),
        unit: 'A',
    });
    rl.plot.plots.push({
        placeholder: $('#plot_digital'),
        unit: 'digital',
    });

    // plot update change handler if enabled
    $('#plot_update').change(() => {
        if (!$('#plot_update').prop('checked')) {
            rl.plot.stop();
        }
    });
});
