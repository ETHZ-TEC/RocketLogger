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

/// RocketLogger channel names
const RL_CHANNEL_NAMES = ['V1', 'V2', 'V3', 'V4', 'I1L', 'I1H', 'I2L', 'I2H'];
/// RocketLogger force range channel names
const RL_CHANNEL_FORCE_NAMES = ['I1H', 'I2H'];

/// RocketLogger config reset value
const RL_CONFIG_RESET = {
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

/// start RocketLogger measurement
function rl_start(config) {
	req = {
		cmd: 'start',
		config: config,
	};
	rl_socket.emit('control', req);
}

/// stop RocketLogger measurement
function rl_stop() {
	req = {
		cmd: 'stop',
		config: null,
	};
	rl_socket.emit('control', req);
}

/// load/store RocketLogger default configuration
function rl_config(config) {
	req = {
		cmd: 'config',
		config: config,
		default: (config != null),
	};
	rl_socket.emit('control', req);
}

/// get configuration from inputs
function config_get() {
	// get channel config
	let channel_enable_config = [];
	for (ch of RL_CHANNEL_NAMES) {
		if ($(`#channel_${ch.toLowerCase()}_enable`).prop('checked')) {
			channel_enable_config.push(ch);
		}
	}

	// get force channel range config
	let channel_force_range_config = [];
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		if ($(`#channel_${ch.toLowerCase()}_force`).prop('checked')) {
			channel_force_range_config.push(ch);
		}
	}

	// get file config
	let file_config = null;
	if ($('#file_enable').prop('checked')) {
		let file_size = 0;
		if ($('#file_split').prop('checked')) {
			file_size = $('#file_size').val() * $('#file_size_scale').val();
		}
		file_config = {
			comment: $('#file_comment').val(),
			filename: $('#file_filename').val(),
			format: $('#file_format').val(),
			size: file_size,
		}
	}

	// assemble config structure
	let config = {
		ambient_enable: $('#ambient_enable').prop('checked'),
		channel_enable: channel_enable_config,
		channel_force_range: channel_force_range_config,
		digital_enable: $('#digital_enable').prop('checked'),
		file: file_config,
		sample_rate: $('#sample_rate').val(),
		web_enable: $('#web_enable').prop('checked'),
	};
	return config;
}

/// set inputs to specified or reset configuration
function config_set(config) {
	if (config === null) {
		config = RL_CONFIG_RESET;
	}

	$('#ambient_enable').prop('checked', config.ambient_enable).change();
	$('#digital_enable').prop('checked', config.digital_enable).change();
	$('#sample_rate').val(config.sample_rate).change();
	$('#web_enable').prop('checked', config.web_enable).change();

	// set channel switches
	for (ch of RL_CHANNEL_NAMES) {
		$(`#channel_${ch.toLowerCase()}_enable`).prop('checked',
			(config.channel_enable.indexOf(ch) >= 0)).change();
	}

	// set force channel range config
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		$(`#channel_${ch.toLowerCase()}_force`).prop('checked',
			(config.channel_force_range.indexOf(ch) >= 0)).change();
	}

	// set file config
	if (config.file === null) {
		$('#file_enable').prop('checked', false).change();
	} else {
		$('#file_enable').prop('checked', true).change();
		$('#file_comment').val(config.file.comment).change();
		$('#file_filename').val(config.file.filename).change();
		$('#file_format').val(config.file.format).change();

		// set file split and size config
		$('#file_split').prop('checked', config.file.size > 0).change();
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
}

/// enable all measurement channels without forcing high range
function config_enable_channels() {
	// enable channel switches
	for (ch of RL_CHANNEL_NAMES) {
		$(`#channel_${ch.toLowerCase()}_enable`).prop('checked', true).change();
	}

	// unset force channel range
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		$(`#channel_${ch.toLowerCase()}_force`).prop('checked', false).change();
	}
	$('#digital_enable').prop('checked', true).change();
	$('#ambient_enable').prop('checked', true).change();
}

/// disable all measurement channels
function config_disable_channels() {
	// disable channels
	for (ch of RL_CHANNEL_NAMES) {
		$(`#channel_${ch.toLowerCase()}_enable`).prop('checked', false).change();
	}

	// unset force channel range
	for (ch of RL_CHANNEL_FORCE_NAMES) {
		$(`#channel_${ch.toLowerCase()}_force`).prop('checked', false).change();
	}
	$('#digital_enable').prop('checked', false).change();
	$('#ambient_enable').prop('checked', false).change();
}

/// add date prefix to filename if not existing
function config_add_file_prefix() {
	const date_pattern = /\d{8}_\d{6}_/;

	// check filename for existing prefix
	let filename = $('#file_filename').val();
	if (date_pattern.test(filename)) {
		filename = filename.slice(16);
	}
	$('#file_filename').val(`${date_to_prefix_string(new Date())}_${filename}`);
}

/**
 * Initialization when document is fully loaded
 */
$(() => {
	// reset configuration controls
	config_set(null);

	// initialize measurement control buttons
	$('#button_start').click(() => {
		rl_start(config_get());
	});
	$('#button_stop').click(() => {
		rl_stop();
	});

	// initialize default configuration control buttons
	$('#button_config_save').click(() => {
		$("#alert_config_saved").hide();
		$("#alert_config_loaded").hide();
		rl_config(config_get());
	});
	$('#button_config_load').click(() => {
		$("#alert_config_saved").hide();
		$("#alert_config_loaded").hide();
		rl_config();
	});

	// initialize configuration helper action buttons
	$('#button_config_all').click(() => {
		config_enable_channels();
	});
	$('#button_config_none').click(() => {
		config_disable_channels();
	});
	$('#button_file_prefix').click(() => {
		config_add_file_prefix();
	});

	// initialize configuration check boxes
	$('#file_enable').change(() => {
		$('#file_group').prop('disabled', !$('#file_enable').prop('checked'));
	});
	$('#file_split').change(() => {
		$('#file_split_group').prop('disabled', !$('#file_split').prop('checked'));
	});
	$('#web_enable').change(() => {
		$('#web_group').prop('disabled', !$('#web_enable').prop('checked'));
		$('#collapsePreview').collapse($('#web_enable').prop('checked') ? 'show' : 'hide');
	});

	// status update callback
	rl_socket.on('control', (res) => {
		console.log(`rl control: ${JSON.stringify(res)}`);
		const cmd = res.req.cmd;
		// @todo handle control feedback
		if (cmd == 'start') {
			// rl_status();
		} else if (cmd == 'stop') {
			// setTimeout(rl_status, 300);
		} else if (cmd == 'config') {
			config_set(res.config);
			if (res.default) {
				$("#alert_config_saved").show();
			} else {
				$("#alert_config_loaded").show();
			}
		}
	});
});
