$(function() {
	
		// TODO
		
		// csv - files
		// currents: less average!
		// avg-values in array (for 3 buffers)
		
		
		// CONSTANTS
		RL_RUNNING = "1";
		RL_OFF = "0";
		RL_ERROR = "-1";
		
		NO_FILE = "0";
		CSV = "1";
		BIN = "2";
		
		BUFFER_SIZE  = 100;
		TIME_DIV = 10;
		
		NUM_PLOT_CHANNELS = 12;
		NUM_DIG_CHANNELS = 6;
		
		NUM_CHANNELS = 8;
		NUM_I_CHANNELS = 2;
		
		UPDATE_INTERVAL = 500;
		STATUS_TIMEOUT_TIME = 3000;
		STOP_TIMEOUT_TIME = 3000;
		MISMATCH_TIMEOUT_TIME = 3000;
		
		CHANNEL_NAMES = ["DigIn1", "DigIn2", "DigIn3", "DigIn4", "DigIn5", "DigIn6", "I1", "V1", "V2", "I2", "V3", "V4"];
		CHANNEL_COLORS = ["#0072BD","#D95319","#EDB120","#7E2F8E","#77AC30", "#4DBEEE", "#0072BD","#0072BD","#D95319","#D95319","#EDB120","#77AC30"];
		
		DIG_DIST_FACTOR = 1.5;
		
		tScales = [1,10,100];
		
		
		
		
		// GLOBAL VARIABLES
		
		var state = RL_OFF;
		var stopping = 0;
		var starting = 0;
		var reqId = 0;
		var plotEnabled = 1;
		var tScale = 0;
		var maxBufferCount = TIME_DIV * tScales[tScale];
		var currentTime = 0;
		var filename = "data.rld";
		var loadDefault = false;
		var freeSpace;
		
		var vScale = 1;
		var iScale = 1;
		
		var timeOut;
		var idMismatch;
		
		// plots
		var vPlot;
		var iPlot;
		
		var digPlot = [];
		
		// data
		var plotBufferCount = 0;
		var plotData = [[],[],[],[],[],[]];
		
		// ajax post object
		var statusObj;
		var cmd_obj = {command: 'start', file: ' -f data.rld', file_format: ' -format bin', rate: ' -r 1', channels: ' -ch 0,1,2,3,4,5,6,7', force: ' -fhr 1,2', digital_inputs: ' -d'};
		
		// channel information
		var channels = [true, true, true, true, true, true, true, true];
		var forceHighChannels = [false, false];
		var plotChannels = [false, false, false, false, false, false, false, false, false, false, false, false];
		var displayChannels = [true, true, true, true, true, true, false, false, false, false, false, false];
		var numDigDisplayed;
		isCurrent = [false, false, false, false, false, false, true, false, false, true, false, false];
		isDigital = [true, true, true, true, true, true, false, false, false, false, false, false];
		
		var vAxisLabel = "Voltage [V]";
		var iAxisLabel = "Current [µA]"
		
		var isActive = true;

		window.onfocus = function () { 
		  isActive = true; 
		}; 

		window.onblur = function () { 
		  isActive = false; 
		}; 
		
		// UPDATE
		
		function update() {

			// get status
			if(isActive || $("#active:checked").length > 0) {
				reqId++;
				getStatus();
			}
			
			timeOut = setTimeout(update, STATUS_TIMEOUT_TIME);			
			
		}
		
		// STATUS CHECK
		function getStatus() {
			
			var e = document.getElementById("time_scale");
			var tempTScale = e.options[e.selectedIndex].value;
			if(tempTScale != tScale) {
				currentTime = 0;
			}
			
			statusObj = {command: 'status', id: reqId.toString(), fetchData: plotEnabled.toString(), timeScale: tempTScale.toString(), time: currentTime.toString()};
			
			$.ajax({
				type: "post",
				url:'rl.php',
				dataType: 'json',
				data: statusObj,
				
				complete: function (response) {
					$('#output').html(response.responseText);
					var tempState = JSON.parse(response.responseText);
					
					// clear time-out
					clearTimeout(timeOut);
					clearTimeout(idMismatch);
					
					// extract state
					respId = tempState[0];
					if (respId == reqId) {
						state = tempState[1];
						
						// free disk space
						freeSpace = tempState[2];
						document.getElementById("free_space").innerHTML = 'Free Disk Space: ' + (parseInt(freeSpace)/1070599167).toFixed(3) + 'GB';
						
						// left sampling time 
						if($("#enable_storing:checked").length > 0) {
							showSamplingTime();
						} else {
							document.getElementById("time_left").innerHTML = "Sampling Time Left: ∞";
						}
						
						// display status on page
						if (state == RL_RUNNING) {
							// parse status and data
							starting = 0;
							if(stopping == 1) {
								document.getElementById("status").innerHTML = 'Status: STOPPING';
							} else {
								document.getElementById("status").innerHTML = 'Status: RUNNING';
							}
							parseStatus(tempState);
							
						} else {
							if(state == RL_ERROR) {
								document.getElementById("status").innerHTML = 'Status: ERROR';
							} else if(state == RL_OFF) {
								document.getElementById("status").innerHTML = 'Status: IDLE';
								stopping = 0;
							} else {
								document.getElementById("status").innerHTML = 'Status: UNKNOWN';
							}
							
							// load default
							if(loadDefault) {
								parseStatus(tempState);
								loadDefault = false;
							}
							
							// reset displays
							document.getElementById("dataAvailable").innerHTML = "";
							document.getElementById("webserver").innerHTML = "";
							
							// set timer
							setTimeout(update, UPDATE_INTERVAL);
						}
					} else {
						
						idMismatch = setTimeout(mismatch, MISMATCH_TIMEOUT_TIME);	
					}		
				}
			});
		}
		
		function showSamplingTime() {
			
			parseChannels();
			
			var numChannelsActivated = 0;
			for(var i=0; i<NUM_CHANNELS; i++) {
				if(channels[i]) {
					numChannelsActivated++;
				}
			}
			var e = document.getElementById("sample_rate");
			var tempSampleRate = e.options[e.selectedIndex].value;
			var rate = (numChannelsActivated+1) * 4 * tempSampleRate * 1000;
			var timeLeft = freeSpace/rate;
			var date = new Date(timeLeft * 1000);
			var d = date.getDate()-1;
			var h = date.getUTCHours();
			var m = date.getUTCMinutes();
			var t = m+"min";
			if(h>0) {
				t = h + "h " + t;
			}
			if(d>0) {
				t = d + "d " + t;
			}
			document.getElementById("time_left").innerHTML = "Sampling Time Left: ≈ " + t;
		}
		
		function mismatch() {
			// ID mismatch -> error
			document.getElementById("status").innerHTML = 'Status: ERROR';
			// set timer
			setTimeout(update, UPDATE_INTERVAL);
		}
		
		function parseStatus(tempState) {
			
			// EXTRACT STATUS INFO
			var sampleRate = tempState[3];
			var digitalInputs = tempState[5];
			var fileFormat = tempState[6];
			var tempFilename = tempState[7];
			var tempChannels = JSON.parse(tempState[8]);
			var tempForceHighChannels = JSON.parse(tempState[9]);
			var samplesTaken = tempState[10];
			var dataAvailable = tempState[11];
			var newData = tempState[12];
			
			// PARSE STATUS INFO
			
			// sample rate
			var e = document.getElementById("sample_rate");
			// switch statement didn't work ...
			if (sampleRate == 1) {
				e.selectedIndex = 0;
			}
			if (sampleRate == 2) {
				e.selectedIndex = 1;
			}
			if (sampleRate == 4) {
				e.selectedIndex = 2;
			}
			if (sampleRate == 8) {
				e.selectedIndex = 3;
			}
			if (sampleRate == 16) {
				e.selectedIndex = 4;
			}
			if (sampleRate == 32) {
				e.selectedIndex = 5;
			}
			if (sampleRate == 64) {
				e.selectedIndex = 6;
			}
			
			// digital inputs
			document.getElementById("digital_inputs").checked = (digitalInputs == "1");
			var digInps = (digitalInputs == "1");
			
			
			
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
			filename = tempFilename.slice(14);
			$("#filename").val(filename);
			
			// channels
			for (var i=0; i<NUM_CHANNELS; i++) {
				if (tempChannels[i] == 1) {
					channels[i] = true;
				} else {
					channels[i] = false;
				}
			}
			updateChannels();
			
			plotChannels = [digInps, digInps, digInps, digInps, digInps, digInps, channels[0] || channels[1], channels[2], channels[3], channels[4] || channels[5], channels[6], channels[7]];
			displayChannels = [$("#plot_d1:checked").length > 0, $("#plot_d2:checked").length > 0, $("#plot_d3:checked").length > 0, $("#plot_d4:checked").length > 0, $("#plot_d5:checked").length > 0, $("#plot_d6:checked").length > 0, $("#plot_i1:checked").length > 0, $("#plot_v1:checked").length > 0, $("#plot_v2:checked").length > 0, $("#plot_i2:checked").length > 0, $("#plot_v3:checked").length > 0, $("#plot_v4:checked").length > 0];
			
			// fhrs
			for (var i=0; i<NUM_I_CHANNELS; i++) {
				if (tempForceHighChannels[i] == 1) {
					forceHighChannels[i] = true;
				} else {
					forceHighChannels[i] = false;
				}
			}
			updateFhrs();
			
			// samples taken
			if(state == RL_RUNNING) {
				document.getElementById("webserver").innerHTML = 'Samples taken: ' + samplesTaken;
			} else {
				document.getElementById("webserver").innerHTML = '';
			}
			
			// data
			if (dataAvailable == "1") {
				document.getElementById("dataAvailable").innerHTML = "";
				if (plotEnabled == 1 && newData == "1") {
					// handle data
					dataReceived(tempState);
					
					// re-update
					update();
				} else {
					setTimeout(update, UPDATE_INTERVAL);
				}
			} else {
				document.getElementById("dataAvailable").innerHTML = "No data available!";
				setTimeout(update, UPDATE_INTERVAL);
			}
		}
		
		
		// DATA HANDLING
		
		function resetData() {
			
			plotBufferCount = 0;
			plotData = [];
			plotData = [[],[],[],[],[],[],[],[],[],[],[],[]];
			
		}
		
		
		function dataReceived (tempState) {
			
			// extract information
			var tempTScale = tempState[13];
			currentTime = parseInt(tempState[14]);
			var bufferCount = parseInt(tempState[15]);
			var bufferSize = parseInt(tempState[16]);
			
			if (tempTScale != tScale) {
				resetData();
				tScale = tempTScale;
				maxBufferCount = TIME_DIV * tScales[tScale];
			}
			
			// process data
			if (bufferCount > 0) {
				
				plotBufferCount += bufferCount;
				
				if(plotBufferCount > maxBufferCount) {
					// client buffer already full
					for (var i = 0; i < NUM_PLOT_CHANNELS; i++) {
						plotData[i] = plotData[i].slice((plotBufferCount-maxBufferCount) * bufferSize);
					}
					plotBufferCount = maxBufferCount;
				}
				
				for (var i = 0; i < bufferCount * bufferSize; i++) {
					var tempData = JSON.parse(tempState[17 + i]);
					var k = 0;
					for (var j = 0; j < NUM_PLOT_CHANNELS; j++) {
						if(plotChannels[j]) {
							if(!isDigital[j]) {
								if(isCurrent[j]) {
									plotData[j].push([currentTime-1000*(bufferCount-1) + 1000/bufferSize*i, tempData[k]/1000]);
								} else {
									plotData[j].push([currentTime-1000*(bufferCount-1) + 1000/bufferSize*i, tempData[k]/1000]);
								}
							} else {
								plotData[j].push([currentTime-1000*(bufferCount-1) + 1000/bufferSize*i, parseInt(tempData[k])]);
							}
							k++;
						}
					}
				}
				updatePlot();
			}
		}

		
		// convert data for plotting
		function getVData() {
			
			// generate for plot
			var vData = [];
			
			var max = maxVValue(plotData);
			
			for (var i = 0; i < NUM_PLOT_CHANNELS; i++) {
				if(plotChannels[i] && displayChannels[i] && !isCurrent[i] && !isDigital[i]) {
					
					// adapt values
					var tempData = [];
					
					var e = document.getElementById("voltage_range");
					vRange = e.options[e.selectedIndex].value;
					if(vRange == 0) {
						// auto ranging
						if(max < 1) {
							vScale = 1000;
							vAxisLabel = "Voltage [uV]";
						} else if(max > 1000) {
							vScale = 0.001;
							vAxisLabel = "Voltage [V]";
						} else {
							vScale = 1;
							vAxisLabel = "Voltage [mV]";
						}
					} else {
						if(vRange == 0.1 || vRange ==1) {
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
					for(var j=0; j< plotData[i].length; j++) {
						tempData.push([plotData[i][j][0], plotData[i][j][1] * vScale]);
					}
					
					var plotChannel = {color: CHANNEL_COLORS[i], label: CHANNEL_NAMES[i], data: tempData};//plotData[i]};//
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
				if(plotChannels[i] && displayChannels[i] && isCurrent[i] && !isDigital[i]) {
					
					// adapt values
					var tempData = [];
					
					var e = document.getElementById("current_range");
					iRange = e.options[e.selectedIndex].value;
					if(iRange == 0) {
							// auto ranging
						if(max < 1) {
							iScale = 1000;
							iAxisLabel = "Current [nA]";
						} else if (max > 1000){
							iScale = 0.001;
							iAxisLabel = "Current [mA]";
						} else {
							iScale = 1;
							iAxisLabel = "Current [µA]";
						}
					} else {
						if(iRange == 0.01 || iRange == 0.1 || iRange == 1) {
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
					
					for(var j=0; j< plotData[i].length; j++) {
						tempData.push([plotData[i][j][0], plotData[i][j][1] * iScale]);
					}
					
					var plotChannel = {color: CHANNEL_COLORS[i], label: CHANNEL_NAMES[i], data: tempData};
					iData.push(plotChannel);
				}
			}
			
			return iData;
			
		}
		
		function getDigData() {
			// generate for plot
			var digData = [];
			var digDataRev = [];
			
			var k=0;
			for (var i = NUM_PLOT_CHANNELS-1; i >=0 ; i--) {
				if(plotChannels[i] && displayChannels[i] && isDigital[i]) {
					
					var tempData = [];
					for(var j=0; j< plotData[i].length; j++) {
						tempData.push([plotData[i][j][0], plotData[i][j][1] + DIG_DIST_FACTOR*k]);
					}
					var plotChannel = {color: CHANNEL_COLORS[i], label: CHANNEL_NAMES[i], data: tempData};
					digData.push(plotChannel);
					
					k++;
				}
			}
			
			for(var j=0; j< digData.length; j++) {
				digDataRev.push(digData[digData.length-1-j]);
			}
			
			return digDataRev;
		}
		
		
		// PLOTTING
		
		// enable checkbox
		$("#plotting").change(function () {
			if ($("#plotting:checked").length > 0) {
				plotEnabled = 1;
			} else {
				plotEnabled = 0;
			}
		});
		
		//  update plot when range changed
		$("#voltage_range").change(function () {
			updatePlot();
		});
		$("#current_range").change(function () {
			updatePlot();
		});
		
		// reset plot
		resetData();
		
		function maxVValue(data) {
			
			var max = 0;
			
			for(var i=0; i<data.length; i++) {
				if(plotChannels[i] && displayChannels[i] && !isCurrent[i] && !isDigital[i]) {
					var tempData = data[i];
					var tempMax = 0;
					for(var j=1; j<tempData.length; j++) {
						if(Math.abs(tempData[j][1]) > tempMax){
							tempMax = Math.abs(tempData[j][1]);
						}
					}
					if(tempMax>max) {
						max = tempMax;
					}
				}
			}
			
			return max;
		}
		
		function maxIValue(data) {
			
			var max = 0;
			
			for(var i=0; i<data.length; i++) {
				if(plotChannels[i] && displayChannels[i] && isCurrent[i] && !isDigital[i]) {
					var tempData = data[i];
					var tempMax = 0;
					for(var j=1; j<tempData.length; j++) {
						if(Math.abs(tempData[j][1]) > tempMax){
							tempMax = Math.abs(tempData[j][1]);
						}
					}
					if(tempMax>max) {
						max = tempMax;
					}
				}
			}
			
			return max;
		}
		
		
		
		function updatePlot () {
			
			// vPlot
			vPlot.setData(getVData());
			var e = document.getElementById("voltage_range");
			vRange = e.options[e.selectedIndex].value;
			if(vRange > 0) {
				vPlot.getOptions().yaxes[0].min = -vRange*vScale;
				vPlot.getOptions().yaxes[0].max = vRange*vScale;
			} else {
				resetVPlot();
			}
			vPlot.getOptions().xaxes[0].min = currentTime - 1000 * (maxBufferCount - 1);
            vPlot.getOptions().xaxes[0].max = currentTime + 1000 - 10*tScales[tScale];
			vPlot.setupGrid();
			vPlot.draw();
			
			// iPlot
			iPlot.setData(getIData());
			var e = document.getElementById("current_range");
			iRange = e.options[e.selectedIndex].value;
			if(iRange > 0) {
				iPlot.getOptions().yaxes[0].min = -iRange*iScale;
				iPlot.getOptions().yaxes[0].max = iRange*iScale;
			} else {
				resetIPlot();
			}
			iPlot.getOptions().xaxes[0].min = currentTime - 1000 * (maxBufferCount - 1);
            iPlot.getOptions().xaxes[0].max = currentTime + 1000 - 10*tScales[tScale];
			iPlot.setupGrid();
			iPlot.draw();
			
			// digPlot
			numDigDisplayed = 0;
			for(var i=0; i<NUM_DIG_CHANNELS; i++) {
				if(displayChannels[i] == true) {
					numDigDisplayed++;
				}
			}
			var size = (70*numDigDisplayed + 50).toString() + "px"; // TODO constants
			document.getElementById("dig_plot").style.height=size;
			resetDigPlot();
			digPlot.getOptions().yaxes[0].max = numDigDisplayed*DIG_DIST_FACTOR-0.4,
			digPlot.setData(getDigData());
			digPlot.getOptions().xaxes[0].min = currentTime - 1000 * (maxBufferCount - 1);
            digPlot.getOptions().xaxes[0].max = currentTime + 1000 - 10*tScales[tScale];
			digPlot.setupGrid();
			digPlot.draw();
			
		}
		
		function resetVPlot() {
			vPlot = $.plot("#vPlaceholder", getVData(), {
				series: {
					shadowSize: 0
				},
				xaxis: {
					mode: "time",
					show: true
				},
				yaxis: {
					tickFormatter: function(val, axis) { return val < axis.max ? val.toFixed(2) : vAxisLabel;}
				}
			});
		}
		
		function resetIPlot() {
			iPlot = $.plot("#iPlaceholder", getIData(), {
				series: {
					shadowSize: 0
				},
				xaxis: {
					mode: "time",
					show: true
				},
				yaxis: {
					tickFormatter: function(val, axis) { return val < axis.max ? val.toFixed(2) : iAxisLabel;}
				}
			});
		}
		
		function resetDigPlot() {
			
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
					max: NUM_DIG_CHANNELS*DIG_DIST_FACTOR+0.1,
					ticks: [[0,'Low'],[1,'High'],[1.5,'Low'],[2.5,'High'],[3,'Low'],[4,'High'],[4.5,'Low'],[5.5,'High'],[6,'Low'],[7,'High'],[7.5,'Low'],[8.5,'High']]
				}
			});
		}
		
		
		// BUTTONS
		
		// load conf button
		$("#load_default").click(function () {
			loadDefault = true;
		});
		
		// start button
		$("#start").click(function () {
			start();
		});
		
		function start() {
		
			if(state == RL_RUNNING) {
				alert("Rocketlogger already running.\nPress Stop!");
				return false;
			}
			if(starting == 1) {
				alert("Rocketlogger already starting!");
				return false;
			}
		
			if(!parseConf()){
				return false;
			}
			
			// reset data
			resetData();
			starting = 1;
			
			$.ajax({
				type: "post",
				url:'rl.php',
				dataType: 'json',
				data: cmd_obj,
				
				complete: function (response) {
					// do nothing
				}
			});
		}

		// stop button
		$("#stop").click(function () {
			stop();
		});
		
		function stop() {
			if (state == RL_OFF) {
				alert("RocketLogger not running!");
				return;
			} 
			if (stopping == 1) {
				alert("You already pressed stop!");
				return;
			} else {
				stopping = 1;
				$.ajax({
					type: "post",
					url:'rl.php',
					dataType: 'json',
					data: {command: 'stop'},
					
					complete: function (response) {
					}
				});
				// set time-out, if stopping fails
				setTimeout(stopFailed, STOP_TIMEOUT_TIME);
			}
		}
		
		function stopFailed() {
			stopping = 0;
		}
		
		// calibrate button
		$("#calibrate").click(function () {
			calibrate();
		});
		
		function calibrate() {
			if(state == RL_RUNNING) {
				alert("Rocketlogger already running.\nPress Stop!");
				return false;
			}
			
			if(!confirm("Warning: This will delete existing calibration data!")) {
				return false;
			}
			
			$.ajax({
				type: "post",
				url:'rl.php',
				dataType: 'json',
				data: {command: 'calibrate'},
				
				complete: function (response) {
					// do nothing
				}
			});
		}
		
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
		
		function updateChannels () {
			document.getElementById("i1h").checked = channels[0];
			document.getElementById("i1l").checked = channels[1];
			document.getElementById("v1").checked = channels[2];
			document.getElementById("v2").checked = channels[3];
			document.getElementById("i2h").checked = channels[4];
			document.getElementById("i2l").checked = channels[5];
			document.getElementById("v3").checked = channels[6];
			document.getElementById("v4").checked = channels[7];
		}
		
		function updateFhrs () {
			
			document.getElementById("fhr1").checked = forceHighChannels[0];
			document.getElementById("fhr2").checked = forceHighChannels[1];
		}
		
		// CONFIGURATION PARSING
		
		
		function parseChannels() {
			var numChannels = 0;
			channels.fill(false);
			
			cmd_obj.channels = " -ch ";
			if ($("#i1h:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "0";
				} else {
					cmd_obj.channels += ",0";
				}
				channels[0] = true;
				numChannels++;
			}
			if ($("#i1l:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "1";
				} else {
					cmd_obj.channels += ",1";
				}
				channels[1] = true;
				numChannels++;
			}
			if ($("#v1:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "2";
				} else {
					cmd_obj.channels += ",2";
				}
				channels[2] = true;
				numChannels++;
			}
			if ($("#v2:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "3";
				} else {
					cmd_obj.channels += ",3";
				}
				channels[3] = true;
				numChannels++;
			}
			
			if ($("#i2h:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "4";
				} else {
					cmd_obj.channels += ",4";
				}
				channels[4] = true;
				numChannels++;
			}
			if ($("#i2l:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "5";
				} else {
					cmd_obj.channels += ",5";
				}
				channels[5] = true;
				numChannels++;
			}
			if ($("#v3:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "6";
				} else {
					cmd_obj.channels += ",6";
				}
				channels[6] = true;
				numChannels++;
			}
			if ($("#v4:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "7";
				} else {
					cmd_obj.channels += ",7";
				}
				channels[7] = true;
				numChannels++;
			}
			
			return numChannels;
		}
		
		function parseConf() {
			
			// rate
			var e = document.getElementById("sample_rate");
			cmd_obj.rate = " -r " + e.options[e.selectedIndex].value;
			
			
			// file
			if ($("#enable_storing:checked").length > 0) {
				cmd_obj.file = " -f /var/www/data/" +  filename;
			} else {
				cmd_obj.file = " -f 0"; // no storing
			}
			
			// file format
			var e = document.getElementById("file_format");
			if (e.options[e.selectedIndex].value == "bin") {
				cmd_obj.file_format = " -format bin";
			} else {
				cmd_obj.file_format = " -format csv";
				var r = document.getElementById("sample_rate");
				if (r.options[r.selectedIndex].value == "64" || r.options[r.selectedIndex].value == "32") {
					if(!confirm("Warning: Using CSV-files with high data rates may cause overruns!")) {
						return false;
					}
				}
			}
			
			// channels
			if(parseChannels() == 0) {
				alert("No channel selected!");
				return false;
			}
			
			// force-channels
			var first = 1;
			if ($("#fhr1:checked").length > 0) {
				if (first == 1) {
					cmd_obj.force = " -fhr 1";
					first = 0;
				} else {
					cmd_obj.force += ",1";
				}
			}
			if ($("#fhr2:checked").length > 0) {
				if (first == 1) {
					cmd_obj.force = " -fhr 2";
					first = 0;
				} else {
					cmd_obj.force += ",2";
				}
			}
			if(first == 1) { // no forcing
				cmd_obj.force = " -fhr 0";
			}
			
			
			// digital inputs
			if ($("#digital_inputs:checked").length > 0) {
				cmd_obj.digital_inputs = " -d";
			} else {
				cmd_obj.digital_inputs = " -d 0";
			}
			
			// set as default
			if ($("#set_default:checked").length > 0) {
				cmd_obj.digital_inputs += " -s";
			}
			
			return true;
		}
		
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
				filename = filename.slice(0,-4) + ".rld";
			} else {
				filename = filename.slice(0,-4) + ".csv";
			}
			$("#filename").val(filename);
		});
		
		
		// download button
		$("#download").click(function () {
			file = 'data/' + filename;
			window.open(file);
		});
		
		// log download
		$("#download_log").click(function () {
			file = 'log/log.txt';
			window.open(file);
		});
		
		// delete button
		$("#delete").click(function () {
			
			if(state == RL_RUNNING) {
				alert("Rocketlogger running.\nPress 'Stop' before deleting files!");
				return;
			}
			
			var deleteFile = "/var/www/data/" +  filename;
			$.ajax({
				type: "post",
				url:'rl.php',
				dataType: 'json',
				data: {command: 'delete', filename: deleteFile},
				
				complete: function (response) {
					document.getElementById("webserver").innerHTML = 'File Deleted!';
				}
			});
		});
		
		// never ending update function
		resetVPlot();
		resetIPlot();
		// TODO:
		resetDigPlot();
		update();

});