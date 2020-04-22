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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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

/// Disk space critically low warning thresholds in permille
const RL_DISK_CRITICAL_THRESHOLD = 50;
/// Disk space low warning thresholds in permille
const RL_DISK_WARNING_THRESHOLD = 150;

/// RocketLogger structure for interaction and data management
const rl = {
	// RocketLogger connection and control handling
	_conn: {
		socket: null,
	},
	// RocketLogger data structures
	_data: {
		status: null,
		config: null,
		buffer: null,
		metadata: null,
	},

	// methods to interface with the RocketLogger
	status: null,
	config: null, // provided by rl.control.js
	start: null, // provided by rl.control.js
	stop: null, // provided by rl.control.js
	plot: null, // provided by rl._data.js
};

/// initialize RocketLogger interfacing base functionality
function rocketlogger_init_base() {
	// new socket.io socket for RocketLogger interaction
	rl._conn.socket = io(window.location.origin);

	// init connection handler callbacks
	rl._conn.socket.on('connect', () => {
		console.log(`socket.io connection established (${rl._conn.socket.id}).`);
	});
	rl._conn.socket.on('disconnect', () => {
		console.log(`socket.io connection closed.`);
	});
	// init default message callback
	rl._conn.socket.on('message', (msg) => {
		console.log(`socket.io message: ${msg}`);
	});

	// init status with reset default
	rl._data.status = status_get_reset();

	// init default status and provide status() request method
	rl.status = () => {
		req = { cmd: 'status' };
		rl._conn.socket.emit('status', req);
	};

	// init status update callback
	rl._conn.socket.on('status', (res) => {
		// console.log(`rl status: ${JSON.stringify(res)}`);
		rl._data.status = res.status;
		update_status();
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
		throw 'undefined RocketLogger status.'
	}
	const status = rl._data.status;

	// status message
	let status_message = '';
	if (status.sampling) {
		status_message += 'sampling';
	} else {
		status_message += 'idle';
		if (rl.plot) {
			rl.plot.stop();
		}
	}
	if (status.error) {
		status_message += ' with error';
		$('#warn_error').show();
	} else {
		$('#warn_error').hide();
	}
	$('#status_message').text(status_message).change();

	// sampling information
	$('#status_samples').text(status.sample_count + ' samples');
	try {
		let sampling_time = status.sample_count / status.config.sample_rate;
		$('#status_time').text(unix_to_timespan_string(sampling_time));
	} catch (err) {
		// skip
	}

	// calibration
	if (status.calibration_time <= 0) {
		$('#status_calibration').text('');
		$('#warn_calibration').show();
	} else {
		$('#status_calibration').text(unix_to_datetime_string(status.calibration_time));
		$('#warn_calibration').hide();
	}

	// storage
	$('#status_disk').text(`${bytes_to_string(status.disk_free_bytes)} (` +
		`${(status.disk_free_permille / 10).toFixed(1)}%) free`);
	$('#warn_storage_critical').hide();
	$('#warn_storage_low').hide();
	if (status.disk_free_permille <= RL_DISK_WARNING_THRESHOLD) {
		if (status.disk_free_permille <= RL_DISK_CRITICAL_THRESHOLD) {
			$('#warn_storage_critical').show();
		} else {
			$('#warn_storage_low').show();
		}
	}

	// remaining sampling time
	if (status.sampling && status.disk_use_rate > 0) {
		$('#status_remaining').text(unix_to_timespan_string(
			status.disk_free_bytes / status.disk_use_rate));
	}

	// control buttons and config form
	if (status.sampling) {
		$('#button_start').removeClass('btn-success').addClass('btn-dark');
		$('#button_stop').addClass('btn-danger').removeClass('btn-dark');
	} else {
		$('#button_start').addClass('btn-success').removeClass('btn-dark');
		$('#button_stop').removeClass('btn-danger').addClass('btn-dark');
	}
	$('#button_start').prop('disabled', status.sampling);
	$('#button_stop').prop('disabled', !status.sampling);
	$('#configuration_group').prop('disabled', status.sampling);
}

/// document ready callback handler for initialization
$(() => {
	// initialize RocketLogger interface and display default status
	rocketlogger_init_base();

	// status update button
	$('#button_status').click(() => {
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
