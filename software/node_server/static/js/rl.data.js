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

/// maximum data buffer length
const RL_DATA_BUFFER_LENGTH = 10000;

/// data buffer structure
let rl_data = {};
/// data buffer metadata
let rl_metadata = {};
/// plots to update
let rl_plots = [];

/// initialize an analog data plot
function plot_get_config(digital = false) {
    let now = Date.now();

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
        },
        xaxis: {
            show: true,
            position: "bottom",
            mode: "time",
            tickFormatter: (val, _) => {
                var d = new Date(val);
                return time_to_string(d);
            },
            min: now - 10000,
            max: now,
            autoScale: "none",
            axisLabel: null,
            gridLines: false,
            showMinorTicks: true,
        },
        yaxis: {
            show: true,
            position: "left",
            axisLabel: null,
            gridLines: true,
            showMinorTicks: true,
        }
    };

    if (digital) {
        config.yaxis.autoScale = "none";
        config.yaxis.min = -0.15;
        config.yaxis.max = 5.85;
        config.yaxis.ticks = [
            [0, "LO"], [0.7, "HI"], [1, "LO"], [1.7, "HI"],
            [2, "LO"], [2.7, "HI"], [3, "LO"], [3.7, "HI"],
            [4, "LO"], [4.7, "HI"], [5, "LO"], [5.7, "HI"],
        ];
        config.yaxis.showMinorTicks = false;
        // config.yaxis.axisLabel = "Digital";
    }
    return config;
}

/// update the plots with new data
function plot_update() {
    for (p of rl_plots) {
        // collect data series to plot
        let data = [];
        for (ch in rl_data) {
            // check if enabled
            if (rl_metadata[ch].unit == p.unit) {
                data.push(rl_data[ch]);
            }
        }

        // get plot config
        const config = plot_get_config(p.digital);

        // update plot
        p.placeholder.plot(data, config);
    }
}

/**
 * Process new measurement data.
 * 
 * @param {Object} res Data result structure from server
 */
function rl_data_process(res) {
    for (const ch in res.metadata) {
        // skip hidden channel
        if (res.metadata[ch].hidden) {
            continue;
        }

        // init array buffer if necessary
        if (!rl_data[ch]) {
            rl_data[ch] = [];
        }

        // decode data
        if (res.metadata[ch].digital) {
            const buf = new Uint32Array(res.digital);
            for (let i = 0; i < buf.length; i++) {
                rl_data[ch].push([
                    res.time[i],
                    (buf[i] & (0x01 << res.metadata[ch].bit)) ? 0.7 : 0
                        + res.metadata[ch].bit]);
            }
        } else if (res.metadata[ch].scale) {
            const buf = new Int32Array(res.data[ch]);
            for (let i = 0; i < buf.length; i++) {
                rl_data[ch].push([res.time[i], buf[i] * res.metadata[ch].scale]);
            }
        } else {
            const buf = new Int32Array(res.data[ch]);
            for (let i = 0; i < buf.length; i++) {
                rl_data[ch].push([res.time[i], buf[i]]);
            }
        }

        // // decode data (legacy)
        // let data = [];
        // if (res.metadata[ch].digital) {
        //     const buf = new Uint32Array(res.digital);
        //     for (let i = 0; i < buf.length; i++) {
        //         data.push([res.time[i], (buf[i] >>> res.metadata[ch].bit) & 0x01]);
        //     }
        //     data = Array.from(new Uint32Array(res.digital),
        //         (val, idx) => [res.time[idx], (val >>> res.metadata[ch].bit) & 0x01]);
        // } else if (res.metadata[ch].scale) {
        //     const buf = new Int32Array(res.data[ch]);
        //     for (let i = 0; i < buf.length; i++) {
        //         data.push([res.time[i], buf[i] * res.metadata[ch].scale]);
        //     }
        //     data = Array.from(new Int32Array(res.data[ch]),
        //         (val, idx) => [res.time[idx], val * res.metadata[ch].scale]);
        // } else {
        //     const buf = new Int32Array(res.data[ch]);
        //     for (let i = 0; i < buf.length; i++) {
        //         data.push([res.time[i], buf[i]]);
        //     }
        //     data = Array.from(new Int32Array(res.data[ch]));
        // }

        // // add data to un-typed array buffer
        // if (rl_data[ch]) {
        //     rl_data[ch] = rl_data[ch].concat(data);
        // } else {
        //     rl_data[ch] = data;
        // }

        // drop old values
        if (rl_data[ch].length > RL_DATA_BUFFER_LENGTH) {
            rl_data[ch].splice(0,
                rl_data[ch].length - RL_DATA_BUFFER_LENGTH);
        }
    }
    rl_metadata = res.metadata;
}

/**
 * Initialization when document is fully loaded.
 */
$(() => {
    // register plots to update
    rl_plots.push({
        placeholder: $('#plot_voltage'),
        unit: 'V',
    });
    rl_plots.push({
        placeholder: $('#plot_current'),
        unit: 'A',
    });
    rl_plots.push({
        placeholder: $('#plot_digital'),
        digital: true,
        unit: null,
    });

    // plot update change handler if enabled
    $('#plot_update').change(() => {
        // trigger plot update if enabled
        if ($('#plot_update').prop('checked')) {
            plot_update();
        }
    });

    /// initialize data reception handler
    rl_socket.on('data', (res) => {
        console.log(`rl data: t=${res.t}, ${res.metadata}`);
        // process data trigger plot update if enabled
        rl_data_process(res);
        if ($('#plot_update').prop('checked')) {
            plot_update();
        }
    });
});
