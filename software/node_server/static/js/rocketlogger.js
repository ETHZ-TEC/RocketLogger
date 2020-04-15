/**
 * Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

/// Disk space critically low warning thresholds in permille
const DISK_CRITICAL_THRESHOLD = 50;
/// Disk space low warning thresholds in permille
const DISK_WARNING_THRESHOLD = 150;
/// Status update polling interval in milliseconds
const STATUS_UPDATE_IDLE_INTERVAL = 5000;
/// Status update polling interval in milliseconds
const STATUS_UPDATE_SAMPLING_INTERVAL = 1000;

/// RocketLogger channel names
const CHANNEL_NAMES = ["V1", "V2", "V3", "V4", "I1L", "I1H", "I2L", "I2H"];
/// RocketLogger force range channel names
const CHANNEL_FORCE_NAMES = ["I1H", "I2H"];

/// RocketLogger status reset value
const STATUS_RESET = {
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
	message: "waiting for status...",
	/// recorded sample count
	sample_count: 0,
	/// recorded sample time in seconds
	sample_time: 0,
	/// sampling state, true: sampling, false: idle
	sampling: false,
};

/// RocketLogger config reset value
const CONFIG_RESET = {
	/// ambient sensor enable
	ambient_enable: false,
	/// channels enabled
	channel_enable: CHANNEL_NAMES,
	/// channels force high range
	channel_force_range: [],
	/// digital input enable
	digital_enable: true,
	/// file storage, file config structure or null to disable
	file: {
		/// file header comment
		comment: "Sampled using the RocketLogger web interface.",
		/// filename
		filename: "data.rld",
		/// file format
		format: "rld",
		/// maximum size before split in multiple files (0 disable)
		size: 1e9,
	},
	/// sample rate
	sample_rate: 1000,
	/// web interface data enable
	web_enable: true,
};

/// timer structure for updating the status
var status_update_timer = null;

/* ************************************************************************* */

/// helper function to display byte values
function bytes_to_string(bytes) {
	if (bytes === 0) {
		return "0 B";
	}
	var log1k = Math.floor(Math.log10(bytes) / 3);
	var value = (bytes / Math.pow(1000, log1k)).toFixed(2);

	switch (log1k) {
		case 0:
			return value + " B";
		case 1:
			return value + " kB";
		case 2:
			return value + " MB";
		case 3:
			return value + " GB";
		case 4:
			return value + " TB";
		default:
			return bytes.toPrecision(5);
	}
}

/// helper function to extend single digit date values
function date_zero_extend(value) {
	if (value < 10) {
		return "0" + value.toFixed();
	}
	return value.toFixed();
}

/// get formatted date string from date object
function date_to_string(date, join = '-') {
	var year = date.getUTCFullYear().toString();
	var month = date_zero_extend(date.getUTCMonth() + 1);
	var day = date_zero_extend(date.getUTCDate());
	return year + join + month + join + day;
}

/// get formatted time string from date object
function time_to_string(time, join = ':') {
	var hour = date_zero_extend(time.getUTCHours());
	var minute = date_zero_extend(time.getUTCMinutes());
	var second = date_zero_extend(time.getUTCSeconds());
	return hour + join + minute + join + second;
}

/// get file prefix string from date object
function date_to_prefix_string(date) {
	return date_to_string(date, '') + "_" + time_to_string(date, '');
}

/// get time string from unit timestamp
function unix_to_datetime_string(seconds) {
	var date = new Date(seconds * 1000);
	return date_to_string(date) + " " + time_to_string(date);
}

/// get time duration string from unit timestamp
function unix_to_timespan_string(seconds) {
	if (seconds == null || isNaN(seconds) || seconds === 0) {
		return "0 s";
	}

	var date = new Date(seconds * 1000);
	var year = date.getUTCFullYear() - 1970;
	var month = date.getUTCMonth() + 1;
	var day = date.getUTCDate();
	var hour = date.getUTCHours();
	var minute = date.getUTCMinutes();
	var second = date.getUTCSeconds();

	var str = "";
	if (year > 0 || str.length > 0) {
		str = str + year.toFixed() + " year" + ((year != 1) ? "s " : " ");
	}
	if (month > 0 || str.length > 0) {
		str = str + date_zero_extend(month) + " month" + ((month != 1) ? "s " : " ");
	}
	if (day > 0 || str.length > 0) {
		str = str + date_zero_extend(day) + " day" + ((day != 1) ? "s " : " ");
	}
	if (hour > 0 || str.length > 0) {
		str = str + date_zero_extend(hour) + " h ";
	}
	if (minute > 0 || str.length > 0) {
		str = str + date_zero_extend(minute) + " min ";
	}
	if (second > 0 || str.length > 0) {
		str = str + date_zero_extend(second) + " s";
	}

	return str.trim();
}

/* ************************************************************************* */

/// Perform a remote action
function action(action, data, handler) {
	post_data = {
		action: action,
		data: data,
	};

	$.ajax({
		url: "/control/" + action,
		type: "POST",
		timeout: STATUS_UPDATE_SAMPLING_INTERVAL,
		data: post_data,
		dataType: "json",
		success: function (data, status) {
			if (status != "success") {
				console.log("Request failed (" + status + "): " + JSON.stringify(data));
				alert("Request failed (" + status + "), see console for details.");
				return;
			}
			if (!data.reply) {
				console.log("Failed decoding reply (" + status + "): " + data);
				alert("Failed decoding reply (" + status + "), see console for details.");
				return;
			}
			handler(data.reply);
		},
		error: function (xhr, status, error) {
			alert("Processing action " + action + " failed, see console for details.");
			console.log(action + " request failed (" + status.toString() + ", " + error + "): "
				+ JSON.stringify(data));
		},
	});
}

/// perform get status remote action
function action_status() {
	// first clear pending timeout to avoid race-condition and parallel requests
	clearTimeout(status_update_timer);
	window.onfocus = null;

	data = {
		action: "status",
		config: null,
	};
	data_json = JSON.stringify(data);
	action("status", data_json, function (data) {
		// process status response
		if (data.status == null) {
			alert("Updating status failed. Disabled auto status updates!");
			return;
		}

		// changes interface based on status
		status_set(data.status);

		// auto refresh after timeout
		if (data.status.sampling) {
			status_update_timer = setTimeout(action_status, STATUS_UPDATE_SAMPLING_INTERVAL);
		} else {
			status_update_timer = setTimeout(action_status, STATUS_UPDATE_IDLE_INTERVAL);
		}
	});
}

/// perform a config get and or set remote action
function action_config(set_default) {
	data = {
		action: "config",
		config: config_get(),
		set_default: set_default,
	};
	data_json = JSON.stringify(data);

	// hide any past save/load notifications
	$("#alert_config_saved").hide();
	$("#alert_config_loaded").hide();

	// perform remote action
	action("config", data_json, function (data) {
		// process config response
		if (data.config === null) {
			if (set_default) {
				alert("Failed saving new default configuration.");
				return;
			} else {
				alert("Failed getting configuration.");
				return;
			}
		}
		// get and display data
		config_set(data.config);

		// display config restore
		if (set_default) {
			$("#alert_config_saved").show();
		} else {
			$("#alert_config_loaded").show();
		}
	});
}

/// perform config get remote action
function action_config_load() {
	return action_config(set_default = false);
}

/// perform config set remote action
function action_config_save() {
	return action_config(set_default = true);
}

/// start measurement remote action
function action_start() {
	data = {
		action: "start",
		config: config_get(),
	};
	data_json = JSON.stringify(data);
	action("start", data_json, function (data) {
		// process start response
		if (data.start == null) {
			alert("Starting measurement failed.");
			return;
		}

		// (temporary) change interface status
		$("#button_start").prop("disabled", true);
		$("#button_start").removeClass("btn-success").addClass("btn-dark");
		$("#button_stop").prop("disabled", false);
		$("#button_stop").addClass("btn-danger").removeClass("btn-dark");
		$("#configuration_group").prop("disabled", true);

		// trigger fast sampling update
		clearTimeout(status_update_timer);
		status_update_timer = setTimeout(action_status, STATUS_UPDATE_IDLE_INTERVAL);
	});
}

/// stop measurement remote action
function action_stop() {
	data = {
		action: "stop",
		config: null,
	};
	data_json = JSON.stringify(data);
	action("stop", data_json, function (data) {
		// process start response
		if (data.stop == null) {
			alert("Stopping measurement failed.");
			return;
		}

		// (temporary) change interface status
		$("#button_start").prop("disabled", false);
		$("#button_start").addClass("btn-success").removeClass("btn-dark");
		$("#button_stop").prop("disabled", true);
		$("#button_stop").removeClass("btn-danger").addClass("btn-dark");
		$("#configuration_group").prop("disabled", false);
	});
}

/* ************************************************************************* */

/// get configuration from inputs
function config_get() {
	// get channel config
	var channel_enable_config = [];
	for (var i = 0; i < CHANNEL_NAMES.length; i++) {
		var ch = CHANNEL_NAMES[i];
		if ($("#channel_" + ch.toLowerCase() + "_enable").prop("checked")) {
			channel_enable_config.push(ch);
		}
	}

	// get force channel range config
	var channel_force_range_config = [];
	for (i = 0; i < CHANNEL_FORCE_NAMES.length; i++) {
		var ch = CHANNEL_FORCE_NAMES[i];
		if ($("#channel_" + ch.toLowerCase() + "_force").prop("checked")) {
			channel_force_range_config.push(ch);
		}
	}

	// get file config
	var file_config = null;
	if ($("#file_enable").prop("checked")) {
		var file_size = 0;
		if ($("#file_split").prop("checked")) {
			file_size = $("#file_size").val() * $("#file_size_scale").val();
		}
		file_config = {
			comment: $("#file_comment").val(),
			filename: $("#file_filename").val(),
			format: $("#file_format").val(),
			size: file_size,
		}
	}

	// assemble config structure
	var config = {
		ambient_enable: $("#ambient_enable").prop("checked"),
		channel_enable: channel_enable_config,
		channel_force_range: channel_force_range_config,
		digital_enable: $("#digital_enable").prop("checked"),
		file: file_config,
		sample_rate: $("#sample_rate").val(),
		web_enable: $("#web_enable").prop("checked"),
	};
	return config;
}

/// set inputs to specified or reset configuration
function config_set(config) {
	if (config === null) {
		config = CONFIG_RESET;
	}

	$("#ambient_enable").prop("checked", config.ambient_enable).change();
	$("#digital_enable").prop("checked", config.digital_enable).change();
	$("#sample_rate").val(config.sample_rate).change();
	$("#web_enable").prop("checked", config.web_enable).change();

	// set channel switches
	for (var i = 0; i < CHANNEL_NAMES.length; i++) {
		var ch = CHANNEL_NAMES[i];
		$("#channel_" + ch.toLowerCase() + "_enable").prop("checked",
			(config.channel_enable.indexOf(ch) >= 0)).change();
	}

	// set force channel range config
	for (var i = 0; i < CHANNEL_FORCE_NAMES.length; i++) {
		var ch = CHANNEL_FORCE_NAMES[i];
		$("#channel_" + ch.toLowerCase() + "_force").prop("checked",
			(config.channel_force_range.indexOf(ch) >= 0)).change();
	}

	// set file config
	if (config.file === null) {
		$("#file_enable").prop("checked", false).change();
	} else {
		$("#file_enable").prop("checked", true).change();
		$("#file_comment").val(config.file.comment).change();
		$("#file_filename").val(config.file.filename).change();
		$("#file_format").val(config.file.format).change();

		// set file split and size config
		$("#file_split").prop("checked", config.file.size > 0).change();
		if (config.file.size === 0) {
			config.file.size = CONFIG_RESET.file.size;
		} else if (config.file.size < 1e6) {
			alert("configured file size too small, reseting to default");
			config.file.size = CONFIG_RESET.file.size;
		}
		if (config.file.size >= 1e9) {
			$("#file_size").val(Math.round(config.file.size / 1e9));
			$("#file_size_scale").val("1000000000");
		} else {
			$("#file_size").val(Math.round(config.file.size / 1e6));
			$("#file_size_scale").val("1000000");
		}
	}
}

/// set the status fields with the status, null to reset
function status_set(status) {
	if (status === null) {
		status = STATUS_RESET;
	}

	// status message
	var status_message = "";
	if (status.sampling) {
		status_message += "sampling";
	} else {
		status_message += "idle";
	}
	if (status.error) {
		status_message += " with error";
		$("#warn_calibration").show();
	} else {
		$("#warn_calibration").hide();
	}
	$("#status_message").text(status_message).change();

	// sampling information
	$("#status_samples").text(status.sample_count + " samples");
	$("#status_time").text(unix_to_timespan_string(status.sample_time));

	// calibration
	if (status.calibration_time <= 0) {
		$("#status_calibration").text("");
		$("#warn_calibration").show();
	} else {
		$("#status_calibration").text(unix_to_datetime_string(status.calibration_time));
		$("#warn_calibration").hide();
	}

	// storage
	var disk_status = bytes_to_string(status.disk_free_bytes) + " (" +
		(status.disk_free_permille / 10).toFixed(1) + "%) free";
	$("#warn_storage_critical").hide();
	$("#warn_storage_low").hide();
	if (status.disk_free_permille <= DISK_WARNING_THRESHOLD) {
		if (status.disk_free_permille <= DISK_CRITICAL_THRESHOLD) {
			$("#warn_storage_critical").show();
		} else {
			$("#warn_storage_low").show();
		}
	}
	$("#status_disk").text(disk_status);

	// calc remaining time @todo update calculation
	if (status.disk_use_rate > 0) {
		var time_remaining = status.disk_free_bytes / status.disk_use_rate;
		$("#status_remaining").text(unix_to_timespan_string(time_remaining));
	} else {
		var config = config_get();
		var use_rate_estimated = (config.channel_enable.length + 
			(config.digital_enable ? 1 : 0)) * 4 * config.sample_rate;
		if (use_rate_estimated > 0 && config.file !== null) {
			var time_remaining = status.disk_free_bytes / use_rate_estimated;
			$("#status_remaining").text("~ " + unix_to_timespan_string(time_remaining));
		} else {
			$("#status_remaining").text("indefinite");
		}
	}

	// control buttons and config form
	if (status.sampling) {
		$("#button_start").removeClass("btn-success").addClass("btn-dark");
		$("#button_stop").addClass("btn-danger").removeClass("btn-dark");
	} else {
		$("#button_start").addClass("btn-success").removeClass("btn-dark");
		$("#button_stop").removeClass("btn-danger").addClass("btn-dark");
	}
	$("#button_start").prop("disabled", status.sampling);
	$("#button_stop").prop("disabled", !status.sampling);
	$("#configuration_group").prop("disabled", status.sampling);
}

/* ************************************************************************* */

/// force update of the status
function status_force_update() {
	// if timeout already running avoid updates in parallel
	clearTimeout(status_update_timer);
	action_status();
}

/// adapt to background configuration setting changes
function status_refresh_change() {
	if ($("#status_refresh").prop("checked")) {
		// if enabled, continue updates (no action)
		window.onblur = null;
		window.onfocus = null;
	} else {
		// if disabled, stop updates (no action)
		window.onblur = function () {
			clearTimeout(status_update_timer);

			// on focus force update status to restart
			window.onfocus = function () {
				window.onfocus = null;
				status_force_update();
			};
		};
		// // on focus force update status to restart
		// window.onfocus = function() {
		//   status_force_update();
		//   window.onfocus = null;
		// };
	}
}

/// enable all measurement channels without forcing high range
function config_enable_channels() {
	$("#channel_v1_enable").prop("checked", true).change();
	$("#channel_v2_enable").prop("checked", true).change();
	$("#channel_v3_enable").prop("checked", true).change();
	$("#channel_v4_enable").prop("checked", true).change();
	$("#channel_i1h_enable").prop("checked", true).change();
	$("#channel_i1l_enable").prop("checked", true).change();
	$("#channel_i1h_force").prop("checked", false).change();
	$("#channel_i2h_enable").prop("checked", true).change();
	$("#channel_i2l_enable").prop("checked", true).change();
	$("#channel_i2h_force").prop("checked", false).change();
	$("#channel_digital").prop("checked", true).change();
	$("#channel_ambient").prop("checked", true).change();
}

/// disable all measurement channels
function config_disable_channels() {
	$("#channel_v1_enable").prop("checked", false).change();
	$("#channel_v2_enable").prop("checked", false).change();
	$("#channel_v3_enable").prop("checked", false).change();
	$("#channel_v4_enable").prop("checked", false).change();
	$("#channel_i1h_enable").prop("checked", false).change();
	$("#channel_i1l_enable").prop("checked", false).change();
	$("#channel_i1h_force").prop("checked", false).change();
	$("#channel_i2h_enable").prop("checked", false).change();
	$("#channel_i2l_enable").prop("checked", false).change();
	$("#channel_i2h_force").prop("checked", false).change();
	$("#channel_digital").prop("checked", false).change();
	$("#channel_ambient").prop("checked", false).change();
}

/// add date prefix to filename if not existing
function config_add_file_prefix() {
	var filename = $('#file_filename').val();

	// check filename for date
	var date_pattern = /\d{8}_\d{6}_/;
	if (date_pattern.test(filename)) {
		filename = filename.slice(16);
	}
	filename = date_to_prefix_string(new Date()) + "_" + filename;
	$('#file_filename').val(filename);
}

/* ************************************************************************* */

/// Initialization when document is fully loaded
$(() => {
	// convert bootstrap hidden class to jquery show/hide
	$(".d-none").hide().removeClass("d-none");

	// reset status fields to default and reset config
	status_set(null);
	config_set(null);

	// initialize control buttons
	$("#button_start").click(action_start);
	$("#button_stop").click(action_stop);
	$("#button_status").click(status_force_update);
	$("#status_refresh").change(status_refresh_change);

	// initialize configuration buttons
	$('#button_config_save').click(action_config_save);
	$('#button_config_load').click(action_config_load);
	$('#button_config_all').click(config_enable_channels);
	$('#button_config_none').click(config_disable_channels);
	$('#button_file_prefix').click(config_add_file_prefix)

	// initialize configuration check boxes
	$("#file_enable").change(function () {
		$("#file_group").prop("disabled", !$("#file_enable").prop("checked"));
	});
	$("#file_split").change(function () {
		$("#file_split_group").prop("disabled", !$("#file_split").prop("checked"));
	});
	$("#web_enable").change(function () {
		$("#web_group").prop("disabled", !$("#web_enable").prop("checked"));
		$("#collapsePreview").collapse($("#web_enable").prop("checked") ? "show" : "hide");
	});

	// initialize plots



	// start status updates
	status_force_update();
});
