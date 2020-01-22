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

// --- CONSTANTS --- //

// state
RL_RUNNING = "1";
RL_OFF = "0";
RL_ERROR = "-1";


// file
NO_FILE = "0";
CSV = "1";
BIN = "2";


// configuration
NUM_PLOT_CHANNELS = 12;
NUM_DIG_CHANNELS = 6;
NUM_CHANNELS = 8;
NUM_I_CHANNELS = 2;


// plot
BUFFER_SIZE = 100;
TIME_DIV = 10;

VOLTAGE_SCALE = 100000;
CURRENT_SCALE = 100000;

DIG_DIST_FACTOR = 1.5;

T_SCALES = [1, 10, 100];

STD_WIDTH = 878;
MAX_DIG_PLOT_HEIGHT = 470;
MIN_DIG_PLOT_HEIGHT = 214;

CHANNEL_NAMES = ["DI1", "DI2", "DI3", "DI4", "DI5", "DI6", "I1", "V1", "V2", "I2", "V3", "V4"];
CHANNEL_COLORS = ["#0072BD", "#D95319", "#EDB120", "#7E2F8E", "#77AC30", "#4DBEEE", "#0072BD", "#0072BD", "#D95319", "#D95319", "#EDB120", "#77AC30"];


// time-out
UPDATE_INTERVAL = 500;
STATUS_TIMEOUT_TIME = 3000;
STOP_TIMEOUT_TIME = 3000;


// scales
GB_SCALE = 1000000000;
MB_SCALE = 1000000;
KB_SCALE = 1000;
MS_SCALE = 1000;
KSPS = 1000;


// keys
KEY_S = 83;
KEY_L = 76;
KEY_D = 68;
KEY_P = 80;
KEY_1 = 49;
KEY_2 = 50;
KEY_3 = 51;



// --- GLOBAL VARIABLES --- //

// state
var state = RL_OFF;
var error = false;
var stopping = false;
var starting = false;
var reqId = 0;
var currentTime = 0;

var isActive = true;


// configuration
var filename = "data.rld";
var loadDefault = true;
var freeSpace;


// channel information
var channels = [true, true, true, true, true, true, true, true];
var forceHighChannels = [false, false];
var plotChannels = [false, false, false, false, false, false, false, false, false, false, false, false];
var displayChannels = [true, true, true, true, true, true, false, false, false, false, false, false];
var numDigDisplayed;
isCurrent = [false, false, false, false, false, false, true, false, false, true, false, false];
isDigital = [true, true, true, true, true, true, false, false, false, false, false, false];


// plotting
var plotEnabled = '1';
var tScale = 0;
var maxBufferCount = TIME_DIV * T_SCALES[tScale];
var plotBufferCount = 0;
var plotData;

var vScale = 1;
var iScale = 1;

var vAxisLabel = "Voltage [V]";
var iAxisLabel = "Current [µA]";

var plotCollapseEnabled = true;


// plots
var vPlot;
var iPlot;
var digPlot;


// time out
var timeOut;
var startTimeOut;


// ajax post object
var statusPost;
var config = { command: 'start', sampleRate: '1', updateRate: '1', channels: 'all', forceHigh: '0', ignoreCalibration: "0", fileName: 'data.rld', fileFormat: 'bin', fileSize: '0', digitalInputs: '1', webServer: '1', setDefault: '0', fileComment: '' };



// --- FUNCTIONS --- //


// UPDATE
function update() {

	// get status
	if (isActive || $("#active:checked").length > 0) {
		reqId++;
		getStatus();
	}

	timeOut = setTimeout(update, STATUS_TIMEOUT_TIME);

}

// STATUS CHECK
function getStatus() {

	var e = document.getElementById("time_scale");
	var tempTScale = e.options[e.selectedIndex].value;
	if (tempTScale != tScale) {
		currentTime = 0;
	}

	statusPost = { command: 'status', id: reqId.toString(), fetchData: plotEnabled, timeScale: tempTScale.toString(), time: currentTime.toString() };

	$.ajax({
		type: "post",
		url: 'rl.php',
		dataType: 'json',
		data: statusPost,

		complete: function (response) {
			$('#output').html(response.responseText);
			tempState = JSON.parse(response.responseText);

			// clear time-out
			clearTimeout(timeOut);

			// extract state
			respId = tempState[0];
			if (respId == reqId) {

				// parse status
				statusReceived(tempState.slice(1));

			} else {
				// error occured
				error = true;

				// set timer
				timeOut = setTimeout(update, UPDATE_INTERVAL);
			}
		}
	});

	if (error == true) {
		document.getElementById("status").innerHTML = 'Status: ERROR';
	}
}

function statusReceived(tempState) {
	state = tempState[0];

	// free disk space
	freeSpace = tempState[1];
	document.getElementById("free_space").innerHTML = (parseInt(freeSpace) / GB_SCALE).toFixed(3) + 'GB';

	// calibration time
	var calibrationTime = parseInt(tempState[2]);
	if (calibrationTime != 0) {
		var d = new Date(calibrationTime * MS_SCALE);
		var datestring = d.getFullYear() + "-" + ("0" + (d.getMonth() + 1)).slice(-2) + "-" + ("0" + d.getDate()).slice(-2) + " " + ("0" + d.getHours()).slice(-2) + ":" + ("0" + d.getMinutes()).slice(-2) + ":" + ("0" + d.getSeconds()).slice(-2);
		document.getElementById("calibration").innerHTML = datestring;
		document.getElementById("calibration").style.color = "";
	} else {
		document.getElementById("calibration").innerHTML = "No Calibration File!";
		document.getElementById("calibration").style.color = "red";
	}

	// left sampling time 
	if ($("#enable_storing:checked").length > 0) {
		showSamplingTime();
	} else {
		document.getElementById("time_left").innerHTML = "∞";
	}

	// display status on page
	if (state == RL_RUNNING) {
		// parse status and data
		clearTimeout(startTimeOut);
		starting = false;
		error = false;

		// color buttons
		document.getElementById("stop").className = "btn btn-danger";
		document.getElementById("start").className = "btn btn-default";

		if (stopping == true) {
			document.getElementById("status").innerHTML = 'Status: STOPPING';
		} else {
			document.getElementById("status").innerHTML = 'Status: RUNNING';
		}
		parseStatus(tempState.slice(3));

	} else {

		// color buttons
		document.getElementById("stop").className = "btn btn-default";
		document.getElementById("start").className = "btn btn-success";

		if (state == RL_ERROR || error == true) {
			document.getElementById("status").innerHTML = 'Status: ERROR';
		} else if (starting == true) {
			document.getElementById("status").innerHTML = 'Status: STARTING';
		} else if (state == RL_OFF) {
			document.getElementById("status").innerHTML = 'Status: IDLE';
			stopping = false;
		} else {
			document.getElementById("status").innerHTML = 'Status: UNKNOWN';
		}

		// reset displays
		document.getElementById("dataAvailable").innerHTML = "";
		document.getElementById("samples_taken").innerHTML = "";
		document.getElementById("samples_taken_val").innerHTML = "";
		document.getElementById("time_sampled").innerHTML = "";
		document.getElementById("time_sampled_val").innerHTML = "";

		// load default
		if (loadDefault) {
			parseStatus(tempState.slice(3));
			loadDefault = false;
		} else {
			// set timer
			timeOut = setTimeout(update, UPDATE_INTERVAL);
		}
	}

	enableDisableConf();
}

function showSamplingTime() {

	var e = document.getElementById("file_format");
	if (e.options[e.selectedIndex].value == "csv") {
		document.getElementById("time_left").innerHTML = "unknown";

	} else {

		parseChannels();

		var numChannelsActivated = 0;
		for (var i = 0; i < NUM_CHANNELS; i++) {
			if (channels[i]) {
				numChannelsActivated++;
			}
		}
		var e = document.getElementById("sample_rate");
		var tempSampleRate = e.options[e.selectedIndex].value;
		var rate = (numChannelsActivated + 1) * 4 * tempSampleRate;
		var timeLeft = freeSpace / rate;

		var date = new Date(timeLeft * MS_SCALE);
		var month = date.getUTCMonth();
		if (month > 0) {
			document.getElementById("time_left").innerHTML = "> 1Month";
		} else {

			var d = date.getUTCDate() - 1;
			var h = date.getUTCHours();
			var m = date.getUTCMinutes();

			var t = m + "min";
			if (h > 0) {
				t = h + "h " + t;
			}
			if (d > 0) {
				t = d + "d " + t;
			}
			document.getElementById("time_left").innerHTML = t;

		}
	}
}

function parseStatus(tempState) {

	// EXTRACT STATUS INFO
	var sampleRate = parseInt(tempState[0]);
	var digitalInputs = tempState[2];
	var calibrationIgnore = tempState[3];
	var fileFormat = tempState[4];
	var sysFilename = tempState[5];
	var maxFileSize = parseInt(tempState[6]) / MB_SCALE;
	var tempChannels = JSON.parse(tempState[7]);
	var tempForceHighChannels = JSON.parse(tempState[8]);
	var samplesTaken = tempState[9];
	var dataAvailable = tempState[10];
	var newData = tempState[11];

	// PARSE STATUS INFO

	// sample rate
	var e = document.getElementById("sample_rate");
	// switch statement didn't work ...
	var i = 0;
	if (sampleRate == 1) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 10) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 100) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 1000) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 2000) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 4000) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 8000) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 16000) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 32000) {
		e.selectedIndex = i;
	}
	i++;
	if (sampleRate == 64000) {
		e.selectedIndex = i;
	}

	// digital inputs
	document.getElementById("digital_inputs").checked = (digitalInputs == "1");
	var digInps = (digitalInputs == "1");

	// calibration
	if (calibrationIgnore == "1") {
		document.getElementById("ignore_calibration").checked = true;
	} else {
		document.getElementById("ignore_calibration").checked = false;
	}

	// file format
	var e = document.getElementById("file_format");

	if (fileFormat == NO_FILE) {
		document.getElementById("enable_storing").checked = false;
	} else {
		if (fileFormat == CSV) {
			e.selectedIndex = 1;
		} else {
			e.selectedIndex = 0;
		}
		document.getElementById("enable_storing").checked = true;
	}

	// file name
	var temp = sysFilename.split('/');
	filename = temp[temp.length - 1];
	$("#filename").val(filename);

	// max file size
	if (maxFileSize > 0) {
		document.getElementById("file_size_limited").checked = true;

		var e = document.getElementById("file_size_unit");
		if (maxFileSize >= KB_SCALE) {
			maxFileSize = maxFileSize / KB_SCALE;
			e.selectedIndex = 1;
		} else {
			e.selectedIndex = 0;
		}
		$("#file_size").val(maxFileSize.toString());
	} else {
		document.getElementById("file_size_limited").checked = false;
	}

	// channels
	for (var i = 0; i < NUM_CHANNELS; i++) {
		if (tempChannels[i] == 1) {
			channels[i] = true;
		} else {
			channels[i] = false;
		}
	}
	updateChannels();

	plotChannels = [digInps, digInps, digInps, digInps, digInps, digInps, channels[0] || channels[1], channels[2], channels[3], channels[4] || channels[5], channels[6], channels[7]];

	// fhrs
	for (var i = 0; i < NUM_I_CHANNELS; i++) {
		if (tempForceHighChannels[i] == 1) {
			forceHighChannels[i] = true;
		} else {
			forceHighChannels[i] = false;
		}
	}
	updateFhrs();

	// samples taken
	if (state == RL_RUNNING) {

		// determine sampled time
		var date = new Date(samplesTaken / sampleRate * MS_SCALE);
		var t = "";
		var month = date.getUTCMonth();
		var d = date.getUTCDate() - 1;
		var h = date.getUTCHours();
		var m = date.getUTCMinutes();
		var s = date.getUTCSeconds();

		if (d == 0) {
			t = s + "s";
		}
		if (m > 0) {
			t = m + "min " + t;
		}
		if (h > 0) {
			t = h + "h " + t;
		}
		if (d > 0) {
			t = d + "d " + t;
		}
		if (month > 0) {
			t = month + "Months " + t;
		}

		document.getElementById("samples_taken").innerHTML = 'Samples Taken:';
		document.getElementById("samples_taken_val").innerHTML = samplesTaken;
		document.getElementById("time_sampled").innerHTML = 'Time Sampled:';
		document.getElementById("time_sampled_val").innerHTML = t;
	}

	// data
	if (dataAvailable == "1") {
		document.getElementById("dataAvailable").innerHTML = "";
		if (plotEnabled == '1' && newData == "1") {
			// handle data
			dataReceived(tempState.slice(12));

			// re-update
			update();
		} else {
			timeOut = setTimeout(update, UPDATE_INTERVAL);
		}
	} else {
		document.getElementById("dataAvailable").innerHTML = "No data available!";
		timeOut = setTimeout(update, UPDATE_INTERVAL);
	}
}


// DATA HANDLING

function resetData() {

	plotBufferCount = 0;
	plotData = [];
	plotData = [[], [], [], [], [], [], [], [], [], [], [], []];
}


function dataReceived(tempState) {

	// extract information
	var tempTScale = tempState[0];
	currentTime = parseInt(tempState[1]);
	var bufferCount = parseInt(tempState[2]);
	var bufferSize = parseInt(tempState[3]);

	if (tempTScale != tScale) {
		resetData();
		tScale = tempTScale;
		maxBufferCount = TIME_DIV * T_SCALES[tScale];
	}

	// process data
	if (bufferCount > 0) {

		plotBufferCount += bufferCount;

		if (plotBufferCount > maxBufferCount) {
			// client buffer already full
			for (var i = 0; i < NUM_PLOT_CHANNELS; i++) {
				plotData[i] = plotData[i].slice((plotBufferCount - maxBufferCount) * bufferSize);
			}
			plotBufferCount = maxBufferCount;
		}

		for (var i = 0; i < bufferCount * bufferSize; i++) {
			var tempData = JSON.parse(tempState[4 + i]);
			var k = 0;
			for (var j = 0; j < NUM_PLOT_CHANNELS; j++) {
				if (plotChannels[j]) {
					if (!isDigital[j]) {
						if (isCurrent[j]) {
							plotData[j].push([currentTime - MS_SCALE * (bufferCount - 1) + MS_SCALE / bufferSize * i, tempData[k] / CURRENT_SCALE]);
						} else {
							plotData[j].push([currentTime - MS_SCALE * (bufferCount - 1) + MS_SCALE / bufferSize * i, tempData[k] / VOLTAGE_SCALE]);
						}
					} else {
						plotData[j].push([currentTime - MS_SCALE * (bufferCount - 1) + MS_SCALE / bufferSize * i, parseInt(tempData[k])]);
					}
					k++;
				}
			}
		}
		updatePlot();
	}
}

// update plot
function updatePlot() {

	resizePlot();

	// update channels to display
	updateDisplayChannels();

	// vPlot
	vPlot.setData(getVData());
	var e = document.getElementById("voltage_range");
	vRange = e.options[e.selectedIndex].value;
	if (vRange > 0) {
		vPlot.getOptions().yaxes[0].min = -vRange * vScale;
		vPlot.getOptions().yaxes[0].max = vRange * vScale;
	} else {
		resetVPlot();
	}
	vPlot.getOptions().xaxes[0].min = currentTime - MS_SCALE * (maxBufferCount - 1);
	vPlot.getOptions().xaxes[0].max = currentTime + MS_SCALE - 10 * T_SCALES[tScale];
	vPlot.setupGrid();
	vPlot.draw();

	// iPlot
	iPlot.setData(getIData());
	var e = document.getElementById("current_range");
	iRange = e.options[e.selectedIndex].value;
	if (iRange > 0) {
		iPlot.getOptions().yaxes[0].min = -iRange * iScale;
		iPlot.getOptions().yaxes[0].max = iRange * iScale;
	} else {
		resetIPlot();
	}
	iPlot.getOptions().xaxes[0].min = currentTime - MS_SCALE * (maxBufferCount - 1);
	iPlot.getOptions().xaxes[0].max = currentTime + MS_SCALE - 10 * T_SCALES[tScale];
	iPlot.setupGrid();
	iPlot.draw();

	// digPlot
	resetDigPlot();
	digPlot.getOptions().yaxes[0].max = numDigDisplayed * DIG_DIST_FACTOR - 0.4,
		digPlot.setData(getDigData());
	digPlot.getOptions().xaxes[0].min = currentTime - MS_SCALE * (maxBufferCount - 1);
	digPlot.getOptions().xaxes[0].max = currentTime + MS_SCALE - 10 * T_SCALES[tScale];
	digPlot.setupGrid();
	digPlot.draw();

}

function resizePlot(plotPlaceholder) {
	var plotWidth = $('#v_plot').width();
	$('#v_plot').height(plotWidth / 2);
	$('#i_plot').height(plotWidth / 2);

	numDigDisplayed = 0;
	for (var i = 0; i < NUM_DIG_CHANNELS; i++) {
		if (displayChannels[i] == true) {
			numDigDisplayed++;
		}
	}
	var size = 70 * numDigDisplayed + 50; // TODO constants
	var relSize = size / MAX_DIG_PLOT_HEIGHT;
	size = Math.max(MIN_DIG_PLOT_HEIGHT, plotWidth / 2 * relSize);
	$('#dig_plot').height(size);
}

// convert data for plotting
function getVData() {

	// generate for plot
	var vData = [];

	var max = maxVValue(plotData);

	for (var i = 0; i < NUM_PLOT_CHANNELS; i++) {
		if (plotChannels[i] && displayChannels[i] && !isCurrent[i] && !isDigital[i]) {

			// adapt values
			var tempData = [];

			var e = document.getElementById("voltage_range");
			vRange = e.options[e.selectedIndex].value;
			if (vRange == 0) {
				// auto ranging
				if (max < 1) {
					vScale = 1000;
					vAxisLabel = "Voltage [uV]";
				} else if (max > 1000) {
					vScale = 0.001;
					vAxisLabel = "Voltage [V]";
				} else {
					vScale = 1;
					vAxisLabel = "Voltage [mV]";
				}
			} else {
				if (vRange == 0.1 || vRange == 1) {
					vScale = 1000;
					vAxisLabel = "Voltage [uV]";
				} else if (vRange == 10 || vRange == 100 || vRange == 1000) {
					vScale = 1;
					vAxisLabel = "Voltage [mV]";
				} else {
					vScale = 0.001;
					vAxisLabel = "Voltage [V]";
				}
			}
			for (var j = 0; j < plotData[i].length; j++) {
				tempData.push([plotData[i][j][0], plotData[i][j][1] * vScale]);
			}

			var plotChannel = { color: CHANNEL_COLORS[i], label: CHANNEL_NAMES[i], data: tempData };//plotData[i]};//
			vData.push(plotChannel);
		}
	}

	return vData;
}

// convert data for plotting
function getIData() {

	// generate for plot
	var iData = [];

	var max = maxIValue(plotData);
	//document.getElementById("test").innerHTML = max;

	for (var i = 0; i < NUM_PLOT_CHANNELS; i++) {
		if (plotChannels[i] && displayChannels[i] && isCurrent[i] && !isDigital[i]) {

			// adapt values
			var tempData = [];

			var e = document.getElementById("current_range");
			iRange = e.options[e.selectedIndex].value;
			if (iRange == 0) {
				// auto ranging
				if (max < 1) {
					iScale = 1000;
					iAxisLabel = "Current [nA]";
				} else if (max > 1000) {
					iScale = 0.001;
					iAxisLabel = "Current [mA]";
				} else {
					iScale = 1;
					iAxisLabel = "Current [µA]";
				}
			} else {
				if (iRange == 0.01 || iRange == 0.1 || iRange == 1) {
					iScale = 1000;
					iAxisLabel = "Current [nA]";
				} else if (iRange == 10 || iRange == 100 || iRange == 1000) {
					iScale = 1;
					iAxisLabel = "Current [µA]";
				} else {
					iScale = 0.001;
					iAxisLabel = "Current [mA]";
				}
			}

			for (var j = 0; j < plotData[i].length; j++) {
				tempData.push([plotData[i][j][0], plotData[i][j][1] * iScale]);
			}

			var plotChannel = { color: CHANNEL_COLORS[i], label: CHANNEL_NAMES[i], data: tempData };
			iData.push(plotChannel);
		}
	}

	return iData;
}

function getDigData() {
	// generate for plot
	var digData = [];
	var digDataRev = [];

	var k = 0;
	for (var i = NUM_PLOT_CHANNELS - 1; i >= 0; i--) {
		if (plotChannels[i] && displayChannels[i] && isDigital[i]) {

			var tempData = [];
			for (var j = 0; j < plotData[i].length; j++) {
				tempData.push([plotData[i][j][0], plotData[i][j][1] + DIG_DIST_FACTOR * k]);
			}
			var plotChannel = { color: CHANNEL_COLORS[i], label: CHANNEL_NAMES[i], data: tempData };
			digData.push(plotChannel);

			k++;
		}
	}

	for (var j = 0; j < digData.length; j++) {
		digDataRev.push(digData[digData.length - 1 - j]);
	}

	return digDataRev;
}

// max v value for auto scale
function maxVValue(data) {

	var max = 0;

	for (var i = 0; i < data.length; i++) {
		if (plotChannels[i] && displayChannels[i] && !isCurrent[i] && !isDigital[i]) {
			var tempData = data[i];
			var tempMax = 0;
			for (var j = 1; j < tempData.length; j++) {
				if (Math.abs(tempData[j][1]) > tempMax) {
					tempMax = Math.abs(tempData[j][1]);
				}
			}
			if (tempMax > max) {
				max = tempMax;
			}
		}
	}

	return max;
}

// max i value for auto scale
function maxIValue(data) {

	var max = 0;

	for (var i = 0; i < data.length; i++) {
		if (plotChannels[i] && displayChannels[i] && isCurrent[i] && !isDigital[i]) {
			var tempData = data[i];
			var tempMax = 0;
			for (var j = 1; j < tempData.length; j++) {
				if (Math.abs(tempData[j][1]) > tempMax) {
					tempMax = Math.abs(tempData[j][1]);
				}
			}
			if (tempMax > max) {
				max = tempMax;
			}
		}
	}

	return max;
}


// PLOTS

// v plot
function resetVPlot() {

	resizePlot();

	vPlot = $.plot("#vPlaceholder", getVData(), {
		series: {
			shadowSize: 0
		},
		xaxis: {
			mode: "time",
			show: true
		},
		yaxis: {
			tickFormatter: function (val, axis) { return val < axis.max ? val.toFixed(2) : vAxisLabel; }
		}
	});
}

// i plot
function resetIPlot() {

	resizePlot();

	iPlot = $.plot("#iPlaceholder", getIData(), {
		series: {
			shadowSize: 0
		},
		xaxis: {
			mode: "time",
			show: true
		},
		yaxis: {
			tickFormatter: function (val, axis) { return val < axis.max ? val.toFixed(2) : iAxisLabel; }
		}
	});
}

// di plot
function resetDigPlot() {

	resizePlot();

	digPlot = $.plot("#digPlaceholder", getDigData(), {
		series: {
			shadowSize: 0
		},
		xaxis: {
			mode: "time",
			show: true
		},
		yaxis: {
			min: -0.1,
			max: NUM_DIG_CHANNELS * DIG_DIST_FACTOR + 0.1,
			ticks: [[0, 'Low'], [1, 'High'], [1.5, 'Low'], [2.5, 'High'], [3, 'Low'], [4, 'High'], [4.5, 'Low'], [5.5, 'High'], [6, 'Low'], [7, 'High'], [7.5, 'Low'], [8.5, 'High']]
		}
	});
}

// start
function start() {

	if (state == RL_RUNNING) {
		alert("Rocketlogger already running.\nPress Stop!");
		return false;
	}
	if (starting == true) {
		alert("Rocketlogger already starting!");
		return false;
	}

	var startPost = parseConf();
	if (!startPost) {
		return false;
	}

	startPost.command = 'start';

	// reset data
	resetData();
	starting = true;

	startTimeOut = setTimeout(startFailed, STATUS_TIMEOUT_TIME);

	$.ajax({
		type: "post",
		url: 'rl.php',
		dataType: 'json',
		data: startPost,

		complete: function (response) {
			$('#output').html(response.responseText);
			var startResp = JSON.parse(response.responseText);
			if (startResp[0] == "ERROR") {
				// alert error
				alert("Server error: " + startResp[1]);

				// reset state to error
				starting = false;
				error = true;
				clearTimeout(startTimeOut);
			}
		}
	});
}

function startFailed() {
	starting = false;
	error = true;
}

// stop
function stop() {
	if (state == RL_OFF) {
		alert("RocketLogger not running!");
		return;
	}
	if (stopping == true) {
		alert("You already pressed stop!");
		return;
	} else {
		stopping = true;
		$.ajax({
			type: "post",
			url: 'rl.php',
			dataType: 'json',
			data: { command: 'stop' },

			complete: function (response) {
				// do nothing
			}
		});
		// set time-out, if stopping fails
		setTimeout(stopFailed, STOP_TIMEOUT_TIME);
	}
}

function stopFailed() {
	stopping = false;
}

// set default conf
function setDefault() {

	var setPost = parseConf();
	if (!setPost) {
		return false;
	}

	setPost.command = 'set_conf';

	$.ajax({
		type: "post",
		url: 'rl.php',
		dataType: 'json',
		data: setPost,

		complete: function (response) {
			$('#output').html(response.responseText);
			var startResp = JSON.parse(response.responseText);
			if (startResp[0] == "ERROR") {
				// alert error
				alert("Server error: " + startResp[1]);

				// reset state to error
				error = true;
			}
		}
	});
}

// channel update
function updateChannels() {
	document.getElementById("i1h").checked = channels[0];
	document.getElementById("i1l").checked = channels[1];
	document.getElementById("v1").checked = channels[2];
	document.getElementById("v2").checked = channels[3];
	document.getElementById("i2h").checked = channels[4];
	document.getElementById("i2l").checked = channels[5];
	document.getElementById("v3").checked = channels[6];
	document.getElementById("v4").checked = channels[7];
}

function updateFhrs() {
	document.getElementById("fhr1").checked = forceHighChannels[0];
	document.getElementById("fhr2").checked = forceHighChannels[1];
}

function updateDisplayChannels() {
	displayChannels = [$("#plot_d1:checked").length > 0, $("#plot_d2:checked").length > 0, $("#plot_d3:checked").length > 0, $("#plot_d4:checked").length > 0, $("#plot_d5:checked").length > 0, $("#plot_d6:checked").length > 0, $("#plot_i1:checked").length > 0, $("#plot_v1:checked").length > 0, $("#plot_v2:checked").length > 0, $("#plot_i2:checked").length > 0, $("#plot_v3:checked").length > 0, $("#plot_v4:checked").length > 0];
}

// CONFIGURATION PARSING
function parseConf() {

	// rate
	var e = document.getElementById("sample_rate");
	var tempSampleRate = e.options[e.selectedIndex].value;
	if (tempSampleRate >= KSPS) {
		tempSampleRate = tempSampleRate / KSPS + "k";
	}
	config.sampleRate = tempSampleRate;


	// file
	if ($("#enable_storing:checked").length > 0) {
		config.fileName = filename;
	} else {
		config.fileName = "0"; // no storing
	}

	// file comment
	if (config.fileName != "0") {
		config.fileComment = $("#comment").val();
	} else {
		config.fileComment = '';
	}

	// file format
	var e = document.getElementById("file_format");
	var r = document.getElementById("sample_rate");
	config.fileFormat = e.options[e.selectedIndex].value;
	if (e.options[e.selectedIndex].value == "csv" && (r.options[r.selectedIndex].value == 64000 || r.options[r.selectedIndex].value == 32000)) {
		if (!confirm("Warning: Using CSV-files with high data rates may cause overruns!")) {
			return false;
		}
	}

	// file size
	if ($("#file_size_limited:checked").length > 0) {
		var e = document.getElementById("file_size_unit");

		// check size
		if (parseInt($("#file_size").val()) < 5 && e.options[e.selectedIndex].value == "m") {
			alert("Too small file size! Minimum is 5MB");
			return false;
		}
		var size = $("#file_size").val();
		if (e.options[e.selectedIndex].value == 'm') {
			size *= MB_SCALE;
		} else {
			size *= GB_SCALE;
		}
		config.fileSize = size;

	} else {
		config.fileSize = "0";
	}

	// channels
	if (parseChannels() == 0) {
		alert("No channel selected!");
		return false;
	}

	// force-channels
	var first = 1;
	if ($("#fhr1:checked").length > 0) {
		if (first == 1) {
			config.forceHigh = "1";
			first = 0;
		} else {
			config.forceHigh += ",1";
		}
	}
	if ($("#fhr2:checked").length > 0) {
		if (first == 1) {
			config.forceHigh = "2";
			first = 0;
		} else {
			config.forceHigh += ",2";
		}
	}
	if (first == 1) { // no forcing
		config.forceHigh = "0";
	}


	// digital inputs
	if ($("#digital_inputs:checked").length > 0) {
		config.digitalInputs = '1';
	} else {
		config.digitalInputs = '0';
	}

	// ignore calibration
	if ($("#ignore_calibration:checked").length > 0) {
		config.ignoreCalibration = "1";
	} else {
		config.ignoreCalibration = "0";
	}

	return config;
}

function parseChannels() {
	var numChannels = 0;
	channels.fill(false);

	if ($("#i1h:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "0";
		} else {
			config.channels += ",0";
		}
		channels[0] = true;
		numChannels++;
	}
	if ($("#i1l:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "1";
		} else {
			config.channels += ",1";
		}
		channels[1] = true;
		numChannels++;
	}
	if ($("#v1:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "2";
		} else {
			config.channels += ",2";
		}
		channels[2] = true;
		numChannels++;
	}
	if ($("#v2:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "3";
		} else {
			config.channels += ",3";
		}
		channels[3] = true;
		numChannels++;
	}

	if ($("#i2h:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "4";
		} else {
			config.channels += ",4";
		}
		channels[4] = true;
		numChannels++;
	}
	if ($("#i2l:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "5";
		} else {
			config.channels += ",5";
		}
		channels[5] = true;
		numChannels++;
	}
	if ($("#v3:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "6";
		} else {
			config.channels += ",6";
		}
		channels[6] = true;
		numChannels++;
	}
	if ($("#v4:checked").length > 0) {
		if (numChannels == 0) {
			config.channels = "7";
		} else {
			config.channels += ",7";
		}
		channels[7] = true;
		numChannels++;
	}

	return numChannels;
}

// configuration disable
function enableDisableConf() {
	if (state == RL_RUNNING) {

		// disable start, enable stop button
		document.getElementById("start").disabled = true;
		document.getElementById("stop").disabled = false;


		// disable conf
		$('#collapse2').find('*').attr('disabled', true);

		// re-enable buttons
		if ($("#enable_storing:checked").length > 0) {
			document.getElementById("download").disabled = false;
		}
		document.getElementById("list").disabled = false;
		document.getElementById("download_log").disabled = false;
	} else {

		// disable start, enable stop button
		document.getElementById("start").disabled = false;
		document.getElementById("stop").disabled = true;

		// enable conf
		$('#collapse2').find('*').attr('disabled', false);

		// disable file conf
		enableDisableFile();
		if ($("#file_size_limited:checked").length <= 0) {
			document.getElementById("file_size").disabled = true;
			document.getElementById("file_size_unit").disabled = true;
		}
	}
}
function enableDisableFile() {

	if ($("#enable_storing:checked").length > 0) {
		document.getElementById("file_format").disabled = false;
		document.getElementById("filename").disabled = false;
		document.getElementById("date_to_filename").disabled = false;
		document.getElementById("file_size_limited").disabled = false;
		document.getElementById("file_size").disabled = false;
		document.getElementById("file_size_unit").disabled = false;
		document.getElementById("download").disabled = false;
		document.getElementById("comment").disabled = false;
	} else {
		document.getElementById("file_format").disabled = true;
		document.getElementById("filename").disabled = true;
		document.getElementById("date_to_filename").disabled = true;
		document.getElementById("file_size_limited").disabled = true;
		document.getElementById("file_size").disabled = true;
		document.getElementById("file_size_unit").disabled = true;
		document.getElementById("download").disabled = true;
		document.getElementById("comment").disabled = true;
	}
}


// files
function download() {
	file = 'data/' + filename;
	window.open(file);
}

function showLog() {
	file = 'log/rocketlogger.log';
	window.open(file);
}

function browseFiles() {
	window.open('data/');
}

function plotsCollapsed() {
	var collapsing = $('#collapse3').hasClass('in');
	if (collapsing == true) {
		if (plotEnabled == '1') {
			plotCollapseEnabled = true;
		} else {
			plotCollapseEnabled = false;
		}
		disablePlot();
	} else {
		if (plotCollapseEnabled == true) {
			enablePlot();
		}
	}
}

function enablePlot() {
	plotEnabled = '1';
	document.getElementById("time_scale").disabled = false;
	document.getElementById("plotting").checked = true;
}

function disablePlot() {
	plotEnabled = '0';
	document.getElementById("time_scale").disabled = true;
	document.getElementById("plotting").checked = false;
}


// ------------------------------------------------------------------------ //

$(document).ready(function () {
	$('[data-toggle="tooltip_help"]').tooltip({
		trigger: 'hover',
		placement: 'auto bottom'
	});
	$('[data-toggle="tooltip"]').tooltip({
		trigger: 'hover',
		placement: 'auto top',
	});
});

$(function () {

	window.onfocus = function () {
		isActive = true;
	};

	window.onblur = function () {
		isActive = false;
	};


	// PLOTTING

	// enable checkbox
	$("#plotting").change(function () {
		if ($("#plotting:checked").length > 0) {
			enablePlot();
		} else {
			disablePlot();
		}
	});

	// BUTTONS

	// load conf button
	$("#load_default").click(function () {
		loadDefault = true;
	});

	// deselect button
	$("#deselect").click(function () {
		channels.fill(false);
		updateChannels();
	});

	// select button
	$("#select").click(function () {
		channels.fill(true);
		updateChannels();
	});

	// date to filename button
	$("#date_to_filename").click(function () {

		// check filename for date
		var patt = /\d{8}_\d{6}_/;
		if (patt.test(filename)) {
			filename = filename.slice(16);
		}

		// add current date
		var currentDate = new Date();
		var year = currentDate.getUTCFullYear().toString();
		var month = (currentDate.getUTCMonth() + 1).toString();
		if (month < 10) {
			month = "0" + month;
		}
		var day = currentDate.getUTCDate().toString();
		if (day < 10) {
			day = "0" + day;
		}
		var hour = currentDate.getUTCHours().toString();
		if (hour < 10) {
			hour = "0" + hour;
		}
		var minute = currentDate.getUTCMinutes().toString();
		if (minute < 10) {
			minute = "0" + minute;
		}
		var second = currentDate.getUTCSeconds().toString();
		if (second < 10) {
			second = "0" + second;
		}
		filename = year + month + day + "_" + hour + minute + second + "_" + filename;
		$("#filename").val(filename);
	});

	// time scale
	$("#time_scale").change(function () {
		if ($("#plotting:checked").length <= 0) {
			document.getElementById("time_scale").selectedIndex = tScale;
			alert("Time scale change is not possible with deactivated plot update!");
		}
	});


	// FILE HANDLING

	// file name
	$("#filename").val(filename).change(function () { // change filename
		var v = $(this).val();
		if (v) {
			filename = v;
			$(this).val(filename);
		}
	});


	$("#file_format").change(function () { // swap all filenames between csv and rld
		var e = document.getElementById("file_format");
		if (e.options[e.selectedIndex].value == "bin") {
			filename = filename.slice(0, -4) + ".rld";
		} else {
			filename = filename.slice(0, -4) + ".csv";
		}
		$("#filename").val(filename);
	});



	// calibration ignore checkbox
	$("#ignore_calibration").change(function () {
		if ($("#ignore_calibration:checked").length) {
			$("#ignore_config").addClass("calibration-warning");
		} else {
			$("#ignore_config").removeClass("calibration-warning");
		}
		if (state == RL_RUNNING) {
			alert("This will not affect the current measurement!");
		}
	});


	document.addEventListener('keydown', function (event) {

		// filter writes to file-name form
		eventId = event.target.id;
		if (eventId == "filename" || eventId == "comment") {
			return;
		}

		// start/stop
		if (event.keyCode == KEY_S) {
			if (state == RL_RUNNING) {
				stop();
			} else {
				start();
			}
			// load default conf
		} else if (event.keyCode == KEY_L) {
			loadDefault = true;
			// store default conf
		} else if (event.keyCode == KEY_D) {
			setDefault();
			// pause/unpause plot
		} else if (event.keyCode == KEY_P) {
			if (plotEnabled == '1') {
				disablePlot();
			} else {
				enablePlot();
			}
			// time scale
		} else if (event.keyCode == KEY_1 || event.keyCode == KEY_2 || event.keyCode == KEY_3) {

			var e = document.getElementById("time_scale");
			e.selectedIndex = event.keyCode - 49;
		}
	});

	window.onresize = function (event) {
		updatePlot();
	}

	// reset data
	resetData();

	// reset plots
	resetVPlot();
	resetIPlot();
	resetDigPlot();

	// never ending update function
	update();
});
