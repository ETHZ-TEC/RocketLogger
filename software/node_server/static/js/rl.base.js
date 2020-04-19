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
const DISK_CRITICAL_THRESHOLD = 50;
/// Disk space low warning thresholds in permille
const DISK_WARNING_THRESHOLD = 150;

/// RocketLogger status reset value
const RL_STATUS_RESET = {
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

/// global socket.io socket object for RocketLogger interaction
const rl_socket = io(`http://${location.hostname}:5000/`);

/// request status update from server
function rl_status() {
	req = { cmd: 'status' };
	rl_socket.emit('status', req);
}

/// set the status fields with the status, null to reset
function status_set(status) {
	if (status === null) {
		status = RL_STATUS_RESET;
	}

	// status message
	let status_message = '';
	if (status.sampling) {
		status_message += 'sampling';
	} else {
		status_message += 'idle';
	}
	if (status.error) {
		status_message += ' with error';
		$('#warn_calibration').show();
	} else {
		$('#warn_calibration').hide();
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
	if (status.disk_free_permille <= DISK_WARNING_THRESHOLD) {
		if (status.disk_free_permille <= DISK_CRITICAL_THRESHOLD) {
			$('#warn_storage_critical').show();
		} else {
			$('#warn_storage_low').show();
		}
	}

	// calc remaining time @todo update calculation
	if (status.disk_use_rate > 0) {
		$('#status_remaining').text(unix_to_timespan_string(
			status.disk_free_bytes / status.disk_use_rate));
	} else {
		let config = config_get();
		let use_rate_estimated = (config.channel_enable.length +
			(config.digital_enable ? 1 : 0)) * 4 * config.sample_rate;
		if (use_rate_estimated > 0 && config.file !== null) {
			$('#status_remaining').text(
				`~ ${unix_to_timespan_string(status.disk_free_bytes / use_rate_estimated)}`);
		} else {
			$('#status_remaining').text('indefinite');
		}
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

/**
 * Initialization when document is fully loaded
 */
$(() => {
	// reset status fields to default
	status_set(null);

	// connection handler callbacks
	rl_socket.on('connect', () => {
		console.log(`socket.io connection established (${rl_socket.id}).`);
	});
	rl_socket.on('disconnect', () => {
		console.log(`socket.io connection closed.`);
	});
	// log default message callback
	rl_socket.on('message', (msg) => {
		console.log(`socket.io message: ${msg}`);
	});

	// status update callback
	rl_socket.on('status', (res) => {
		// console.log(`rl status: ${JSON.stringify(res)}`);
		status_set(res.status);
	});

	// status update button
	$('#button_status').click(() => {
		rl_status();
	});

	// status update on window focus
	window.addEventListener('focus', () => {
		rl_status();
	});

	// trigger status update on load
	rl_status();
});
