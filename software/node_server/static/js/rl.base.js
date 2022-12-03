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

/// RocketLogger offline timeout for connection setup
const RL_OFFLINE_TIMEOUT_MS = 2000;
/// Disk space critically low warning thresholds in permille
const RL_DISK_CRITICAL_THRESHOLD = 50;
/// Disk space low warning thresholds in permille
const RL_DISK_WARNING_THRESHOLD = 150;

/// RocketLogger structure for interaction and data management
const rl = {
    // methods to interface with the RocketLogger
    status: null,
    config: null, // provided by rl.control.js
    start: null, // provided by rl.control.js
    stop: null, // provided by rl.control.js
    reset: null, // provided by rl.control.js
    reboot: null, // provided by rl.control.js
    shutdown: null, // provided by rl.control.js
    plot: null, // provided by rl._data.js

    // RocketLogger data structures
    _data: {
        socket: null,
        status: null,
        config: null,
        metadata: null,
        buffer: null,
        time: null,
        reset: true,
    }
};

/// initialize RocketLogger interfacing base functionality
function rocketlogger_init_base() {
    // set connection timeout before showing offline banner
    const offline_timeout = setTimeout(() => {
        show('#error_offline');
    }, RL_OFFLINE_TIMEOUT_MS);

    // new socket.io socket for RocketLogger interaction
    rl._data.socket = io(window.location.origin);

    // init connection handler callbacks
    rl._data.socket.on('connect', () => {
        console.log(`socket.io connection established (${rl._data.socket.id}).`);
        clearTimeout(offline_timeout);
        hide('#error_offline');
    });
    rl._data.socket.on('disconnect', () => {
        console.log(`socket.io connection closed.`);
        show('#error_offline');
    });

    // init status with reset default
    rl._data.status = status_get_reset();

    // init default status and provide status() request method
    rl.status = () => {
        const req = { cmd: 'status' };
        rl._data.socket.emit('status', req);
    };

    // init status update callback
    rl._data.socket.on('status', (res) => {
        // console.log(`rl status: ${JSON.stringify(res)}`);
        if (res.status) {
            // reset plots on sampling start
            if (rl._data.status.sampling === false && res.status.sampling === true) {
                rl._data.reset = true;
            }
            // override default config if not set
            if (rl._data.default_config === null && res.status.config) {
                rl._data.default_config = res.status.config;
                try {
                    config_reset_default();
                }
                catch (err) {
                    /* silently ignore if `rl.control.js` not loaded */
                }
            }
            rl._data.status = res.status;
            update_status();
        }
    });
}

/// get status reset value
function status_get_reset() {
    const status = {
        /// calibration timestamp, -1 no calibration
        calibration: -1,
        /// free disk space in bytes
        disk_free_bytes: 0,
        /// free disk space in permille
        disk_free_permille: 0,
        /// disk space required per minute when sampling
        disk_use_per_min: 0,
        /// error while sampling
        error: false,
        /// status message
        message: 'waiting for status...',
        /// recorded sample count
        sample_count: 0,
        /// recorded sample time in seconds
        sample_time: 0,
        /// sampling state, true: sampling, false: idle
        sampling: false,
    };
    return status;
}

/// update the status interface with the current RocketLogger status
function update_status() {
    if (rl._data.status === null) {
        throw Error('undefined RocketLogger status.');
    }
    const status = rl._data.status;

    // status message
    let status_message = '';
    if (status.sampling) {
        status_message += 'sampling';
    } else {
        status_message += 'idle';
        if (rl.plot?.stop) {
            rl.plot.stop();
        }
    }
    if (status.error) {
        status_message += ' with error';
        show('#warn_error');
    } else {
        hide('#warn_error');
    }
    document.querySelector('#status_message').innerText = status_message;

    // sampling information
    document.querySelector('#status_samples').innerText = status.sample_count + ' samples';
    try {
        let sampling_time = status.sample_count / status.config.sample_rate;
        document.querySelector('#status_time').innerText = unix_to_timespan_string(sampling_time);
    } catch (err) {
        // skip
    }

    // calibration
    if (status.calibration_time <= 0) {
        document.querySelector('#status_calibration').innerText = '';
        show('#warn_calibration');
    } else {
        document.querySelector('#status_calibration').innerText = unix_to_datetime_string(status.calibration_time);
        hide('#warn_calibration');
    }

    // storage
    document.querySelector('#status_disk').innerText = `${bytes_to_string(status.disk_free_bytes)} (` +
        `${(status.disk_free_permille / 10).toFixed(1)}%) free`;
    hide('#warn_storage_critical');
    hide('#warn_storage_low');
    if (status.disk_free_permille <= RL_DISK_WARNING_THRESHOLD) {
        if (status.disk_free_permille <= RL_DISK_CRITICAL_THRESHOLD) {
            show('#warn_storage_critical');
        } else {
            show('#warn_storage_low');
        }
    }
    if (status.hasOwnProperty('sdcard_available') && !status.sdcard_available) {
        show('#warn_sdcard_unavailable');
    }

    // remaining sampling time
    if (status.sampling && status.disk_use_rate > 0) {
        document.querySelector('#status_remaining').innerText = unix_to_timespan_string(
            status.disk_free_bytes / status.disk_use_rate)
    }

    // control buttons and config form
    if (status.sampling) {
        replaceClass('#button_start', 'btn-success', 'btn-dark');
        replaceClass('#button_stop', 'btn-dark', 'btn-danger');
    } else {
        replaceClass('#button_start', 'btn-dark', 'btn-success');
        replaceClass('#button_stop', 'btn-danger', 'btn-dark');
    }
    document.querySelector('#button_start').disabled = status.sampling;
    document.querySelector('#button_stop').disabled = !status.sampling;
    document.querySelector('#configuration_group').disabled = status.sampling;
}

/// initialize when document is fully loaded
window.addEventListener('load', () => {
    // initialize RocketLogger interface and display default status
    rocketlogger_init_base();

    // status update button
    document.querySelector('#button_status').addEventListener('click', () => {
        rl.status();
    });

    // status update on window focus
    window.addEventListener('focus', () => {
        rl.status();
    });

    // update status interface and trigger status update
    update_status();
    rl.status();
});
