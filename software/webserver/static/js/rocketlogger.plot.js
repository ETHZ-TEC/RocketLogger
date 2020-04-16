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

 /// Voltage channel names
const RL_CHANNELS_VOLTAGE = ["V1", "V2", "V3", "V4"];

/// Digital channel names
const RL_CHANNELS_DIGITAL = ["DI1", "DI2", "DI3", "DI4", "DI5", "DI6"];

/// Current channel names
const RL_CHANNELS_CURRENT = ["I1", "I2"];

/// Status update polling interval in milliseconds
const DATA_UPDATE_INTERVAL = 1000;

/// Status update timeout in milliseconds
const DATA_UPDATE_TIMEOUT = 500;

/// Default plot configuration
const PLOT_CONFIG_DEFAULT = {
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
		showMinorTicks: true,
		gridLines: false,
		mode: "time",
		timeformat: "%H:%M:%S",
		timezone: "browser",
		min: 0,
		max: 10,
		position: "bottom",
	},
	yaxis: {
		axisLabel: null,
		show: true,
		showMinorTicks: true,
		gridLines: true,
		position: "left",
	}
};

const PLOT_STRUCTURE_DEFAULT = {
	placeholder: null,
	channels: [],
	unit: null,
}

/// data buffer structure with reset values
const DATA_BUFFER_RESET = {
	update_time: 0,
	channels: [],
	time: [],
	data: [],
};

/// timer structure for polling data
var data_update_timer = null;

/// track plots to update
var plots = [];

/// buffer for local data
var data_buffer = DATA_BUFFER_RESET;

/* ************************************************************************* */

/// get the timestamp of the most recent available element
function buffer_timestamp() {
	/// TODO: get timestamp of buffer head
	return 0;
}

function buffer_add(timestamp, data) {
	/// assumes data ordered by timestamp (otherwise order by timestamp)

	/// push new data

	/// get latest timestamp

	/// pop old data (maybe overrun to next buffer?)
}

/// initialize new buffer
function buffer_init(channels) {
	data_buffer = DATA_BUFFER_RESET;
}

/* ************************************************************************* */

/// request data update from server
function data_request(data, handler) {
	post_data = {
		action: 'data',
		data: JSON.stringify({
			action: "data",
			buffer_time: buffer_timestamp(),
		}),
	};

	$.ajax({
		url: "/control/data",
		type: "POST",
		timeout: DATA_UPDATE_TIMEOUT,
		data: post_data,
		dataType: "json",
		success: function (status, data, status) {
			if (status != "success") {
				console.log("Data request failed (" + status + "): " + JSON.stringify(data));
				alert("Data request failed (" + status + "), see console for details.");
				return;
			}
			if (!data.reply) {
				console.log("Failed decoding data (" + status + "): " + data);
				alert("Failed decoding data (" + status + "), see console for details.");
				return;
			}
			handler(data.reply);
		},
		error: function (xhr, status, error) {
			console.log("Data request failed (" + status.toString() + ", " + error + "): "
				+ JSON.stringify(data));
			alert("Processing data request failed, see console for details.");
		},
	});
}

/// handle new data
function data_handler(channels, timestamp, data) {
	/// check channel information

	/// assumes data ordered by timestamp (otherwise order by timestamp)

	/// push new data

	/// get latest timestamp

	/// pop old data (maybe overrun to next buffer?)

	/// initiate update of plots
}

/* ************************************************************************* */

/// start requesting data updates
function data_start() {
	// if timeout already running avoid updates in parallel
	clearTimeout(data_update_timer);
	action_status();
}

/// start requesting data updates
function data_stop() {
	// clear any pending data update interval
	clearTimeout(data_update_timer);
}

/* ************************************************************************* */

/// initialize an analog data plot
function plot_init(obj) {
	var now = new Date();
	var config = PLOT_CONFIG_DEFAULT;
	config.xaxis.min = now.getTime() - 10000;
	config.xaxis.max = now.getTime();
	return $(obj).plot([], config);
}

/// initialize a digital data plot
function plot_init_digital(obj) {
	var now = new Date();
	var config = PLOT_CONFIG_DEFAULT;
	config.xaxis.min = now.getTime() - 10000;
	config.xaxis.max = now.getTime();
	config.yaxis.min = -0.15;
	config.yaxis.max = 5.85;
	config.yaxis.ticks = [
		[0, "LO"], [0.7, "HI"], [1, "LO"], [1.7, "HI"],
		[2, "LO"], [2.7, "HI"], [3, "LO"], [3.7, "HI"],
		[4, "LO"], [4.7, "HI"], [5, "LO"], [5.7, "HI"],
	];
	config.yaxis.showMinorTicks = false;
	config.yaxis.axisLabel = "Digital";

	return $(obj).plot([], config);
}

/// update the plots with new data
function plot_update() {
	for (p in plots) {

		p.placeholder.width();
	}
}

/// update xaxis of the plot for 
function plot_shift() {
	for (p in plots) {
		p.placeholder.width();
	}
}


/* ************************************************************************* */

/// Initialize plotting after document loaded and ready
$(function () {
	var plot;

	// initialize and register plots
	plot = PLOT_STRUCTURE_DEFAULT;
	plot.channels = RL_CHANNELS_VOLTAGE;
	plot.units = 'V';
	plot.placeholder = plot_init($("#plot_voltage"));
	plots.push(plot);

	plot = PLOT_STRUCTURE_DEFAULT;
	plot.channels = RL_CHANNELS_CURRENT;
	plot.units = 'A';
	plot.placeholder = plot_init($("#plot_current"));
	plots.push(plot);

	plot = PLOT_STRUCTURE_DEFAULT;
	plot.channels = RL_CHANNELS_DIGITAL;
	plot.digital = true;
	plot.placeholder = plot_init_digital($("#plot_digital"));
	plots.push(plot);
});
