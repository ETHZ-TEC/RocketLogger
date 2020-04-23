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

// check RocketLogger base functionality is loaded
if (typeof (rl) == 'undefined') {
	throw 'need to load rl.base.js before loading rl.control.js'
}

/// RocketLogger channel names
const RL_CHANNEL_NAMES = ['V1', 'V2', 'V3', 'V4', 'I1L', 'I1H', 'I2L', 'I2H'];
/// RocketLogger force range channel names
const RL_CHANNEL_FORCE_NAMES = ['I1H', 'I2H'];

/// initialize RocketLogger control functionality
function rocketlogger_init_control() {
	// check RocketLogger base functionality is initialized
	if (rl.status === null) {
		throw 'need RocketLogger base functionality to be initialized first.'
	}

	// init config with reset default
	rl._data.default_config = null;

	// provide start(), stop() and config() request methods
	rl.start = () => {
		req = {
			cmd: 'start',
			config: rl._data.config,
		};
		rl._conn.socket.emit('control', req);
	};
	rl.stop = () => {
		req = {
			cmd: 'stop',
			config: null,
		};
		rl._conn.socket.emit('control', req);
	};
	rl.config = (set_default) => {
		req = {
			cmd: 'config',
			config: rl._data.config,
			default: set_default,
		};
		rl._conn.socket.emit('control', req);
	};

	// init config update callback
	rl._conn.socket.on('control', (res) => {
		console.log(`rl control: ${JSON.stringify(res)}`);
		const cmd = res.req.cmd;
		/// @todo handle control feedback
		if (cmd == 'start') {
			rl._data.config = res.config;
		} else if (cmd == 'stop') {
			// no actions
		} else if (cmd == 'config') {
			const init = (rl._data.default_config == null);
			rl._data.default_config = res.config;
			config_reset_default();

			// indicate update except on initial load
			if (!init) {
				if (res.default) {
					$("#alert_config_saved").show();
				} else {
					$("#alert_config_loaded").show();
				}
			}
		}
	});
}

/// get configuration reset value
function config_get_reset() {
	const config = {
		/// ambient sensor enable
		ambient_enable: false,
		/// channels enabled
		channel_enable: RL_CHANNEL_NAMES,
		/// channels force high range
		channel_force_range: [],
		/// digital input enable
		digital_enable: true,
		/// file storage, file config structure or null to disable
		file: {
			/// file header comment
			comment: 'Sampled using the RocketLogger web interface.',
			/// filename
			filename: 'data.rld',
			/// file format
			format: 'rld',
			/// maximum size before split in multiple files (0 disable)
			size: 1e9,
		},
		/// sample rate
		sample_rate: 1000,
		/// web interface data enable
		web_enable: true,
	};
	return config;

}

/// update RocketLogger config on interface changes
function config_change() {
	// new configuration structure
	const config = {
		ambient_enable: $('#ambient_enable').prop('checked'),
		channel_enable: [],
		channel_force_range: [],
		digital_enable: $('#digital_enable').prop('checked'),
		file: null,
		sample_rate: $('#sample_rate').val(),
		web_enable: $('#web_enable').prop('checked'),
	}

	// get channel config
	for (ch of RL_CHANNEL_NAMES) {
		if ($(`#channel_${ch.toLowerCase()}_enable`).prop('checked')) {
			config.channel_enable.push(ch);
		}
	}

	// get force channel range config
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		if ($(`#channel_${ch.toLowerCase()}_force`).prop('checked')) {
			config.channel_force_range.push(ch);
		}
	}

	// get file config
	if ($('#file_enable').prop('checked')) {
		config.file = {
			comment: $('#file_comment').val(),
			filename: $('#file_filename').val(),
			format: $('#file_format').val(),
			size: 0,
		}
		if ($('#file_split').prop('checked')) {
			config.file.size = $('#file_size').val() * $('#file_size_scale').val();
		}
	}

	// update stored config
	rl._data.config = config;

	// perform necessary interface updates
	$("#alert_config_saved").hide();
	$("#alert_config_loaded").hide();
	$('#file_group').prop('disabled', (config.file == null));
	$('#file_split_group').prop('disabled', (config.file && (config.file.size == 0)));
	$('#web_group').prop('disabled', !config.web_enable);
	$('#collapsePreview').collapse(config.web_enable ? 'show' : 'hide');

	// estimate remaining time from configuration
	let use_rate_estimated = (config.channel_enable.length +
		(config.digital_enable ? 1 : 0)) * 4 * config.sample_rate;
	if (use_rate_estimated > 0 && config.file !== null) {
		$('#status_remaining').text(
			`~ ${unix_to_timespan_string(rl._data.status.disk_free_bytes / use_rate_estimated)}`);
	} else {
		$('#status_remaining').text('indefinite');
	}
}

/// update configuration interface to RocketLogger default configuration
function config_reset_default() {
	if (rl._data.default_config === null) {
		throw 'undefined RocketLogger default configuration.'
	}
	const config = rl._data.default_config;

	// set direct value inputs
	$('#ambient_enable').prop('checked', config.ambient_enable);
	$('#digital_enable').prop('checked', config.digital_enable);
	$('#sample_rate').val(config.sample_rate);
	$('#web_enable').prop('checked', config.web_enable);

	// set channel switches
	for (ch of RL_CHANNEL_NAMES) {
		$(`#channel_${ch.toLowerCase()}_enable`).prop('checked',
			(config.channel_enable.indexOf(ch) >= 0));
	}

	// set force channel range config
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		$(`#channel_${ch.toLowerCase()}_force`).prop('checked',
			(config.channel_force_range.indexOf(ch) >= 0));
	}

	// set file config inputs
	if (config.file === null) {
		$('#file_enable').prop('checked', false);
	} else {
		$('#file_enable').prop('checked', true);
		$('#file_comment').val(config.file.comment);
		$('#file_filename').val(config.file.filename);
		$('#file_format').val(config.file.format);

		// set file split and size config
		$('#file_split').prop('checked', config.file.size > 0);
		if (config.file.size === 0) {
			config.file.size = RL_CONFIG_RESET.file.size;
		} else if (config.file.size < 1e6) {
			alert('configured file size too small, reseting to default');
			config.file.size = RL_CONFIG_RESET.file.size;
		}
		if (config.file.size >= 1e9) {
			$('#file_size').val(Math.round(config.file.size / 1e9));
			$('#file_size_scale').val('1000000000');
		} else {
			$('#file_size').val(Math.round(config.file.size / 1e6));
			$('#file_size_scale').val('1000000');
		}
	}

	// trigger configuration interface update handler
	config_change();
}

/// enable all measurement channels without forcing high range
function config_channels_enable() {
	// enable channel switches
	for (ch of RL_CHANNEL_NAMES) {
		$(`#channel_${ch.toLowerCase()}_enable`).prop('checked', true);
	}

	// unset force channel range
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		$(`#channel_${ch.toLowerCase()}_force`).prop('checked', false);
	}
	$('#digital_enable').prop('checked', true);
	$('#ambient_enable').prop('checked', true);

	// trigger configuration interface update handler
	config_change();
}

/// disable all measurement channels
function config_channels_disable() {
	// disable channels
	for (ch of RL_CHANNEL_NAMES) {
		$(`#channel_${ch.toLowerCase()}_enable`).prop('checked', false);
	}

	// unset force channel range
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		$(`#channel_${ch.toLowerCase()}_force`).prop('checked', false);
	}
	$('#digital_enable').prop('checked', false);
	$('#ambient_enable').prop('checked', false);

	// trigger configuration interface update handler
	config_change();
}

/// add date prefix to filename if not existing
function config_file_add_prefix() {
	const date_pattern = /\d{8}_\d{6}_/;

	// check filename for existing prefix
	let filename = $('#file_filename').val();
	if (date_pattern.test(filename)) {
		filename = filename.slice(16);
	}
	$('#file_filename').val(`${date_to_prefix_string(new Date())}_${filename}`);

	// trigger configuration interface update handler
	config_change();
}

/// document ready callback handler for initialization
$(() => {
	// initialize RocketLogger control functionality
	rocketlogger_init_control();

	// initialize configuration interface update handler
	$('#configuration_group').on('change', config_change);

	// initialize configuration interface helper action buttons
	$('#button_config_all').click(() => {
		config_channels_enable();
	});
	$('#button_config_none').click(() => {
		config_channels_disable();
	});
	$('#button_file_prefix').click(() => {
		config_file_add_prefix();
	});

	// initialize measurement control buttons
	$('#button_start').click(() => {
		rl.start();
	});
	$('#button_stop').click(() => {
		rl.stop();
	});

	// initialize default configuration control buttons
	$('#button_config_save').click(() => {
		$("#alert_config_saved").hide();
		$("#alert_config_loaded").hide();
		rl.config(true);
	});
	$('#button_config_load').click(() => {
		$("#alert_config_saved").hide();
		$("#alert_config_loaded").hide();
		rl.config();
	});

	// update configuration interface and trigger load config
	rl.config();
});
