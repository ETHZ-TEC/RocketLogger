$(function() {
		
		// GLOBAL VARIABLES
		var state = "OFF";
		var webserverEnabled = "0";
		var stopping = 0;
		updateInterval = 500;
		dataTimeoutTime = 2000;
		stopTimeoutTime = 3000;
		var timeOut;
		
		var filename = "data.dat";
		var normalFilename = "data.dat";
		var networkFilename = "xxx/yyy/zzz.dat";
		
		var currentData = [];
		var vData = [];
		var iData = [];
		bufferSize = 100;	// number of values per buffer
		timeDiv = 10;		// time span displayed
		totalPoints = timeDiv * bufferSize;
		
		// ajax post object
		var cmd_obj = {command: 'start', file: ' -f data.csv -b -w', rate: ' -r 1', update_rate: ' -u 1', channels: ' -ch 0,1,2,3,4,5,6,7,8,9', force: ' -fhr 1,2'};
			
		var vChannels = [true, true, true, true];
		var iChannels = [true, true, true, true, true, true];
		var numChannels = 0;
		var numVChannels = 0;
		var numIChannels = 0;
		maxVChannels = 4;
		maxIChannels = 2;
		vNames = ["V1 [mV]", "V2 [mV]", "V3 [mV]", "V4 [mV]"];
		iNames = ["I1 [uA]", "I2 [uA]"];
		
		// UPDATE
		
		function update() {

			// get status
			getStatus();
			
			if (state != "OFF" && webserverEnabled == "1" && stopping != 1) {
				// fetch data
				fetchData();
				timeOut = setTimeout(update, dataTimeoutTime);
			} else {
				// set timer
				setTimeout(update, updateInterval);
			}
		}
		
		function dataReceived () {
			// clear time-out
			clearTimeout(timeOut);
			// update data arrays
			updateData();
			// plot
			updatePlot();
			// re-update
			update();
		}
		
		// STATUS CHECK
		function getStatus() {
			$.ajax({
				type: "post",
				url:'rl.php',
				dataType: 'json',
				data: {command: 'status'},
				
				complete: function (response) {
					$('#output').html(response.responseText);
					var tempState = JSON.parse(response.responseText);
					
					// extract state
					state = tempState[0];
					// display status on page
					document.getElementById("status").innerHTML = 'Status: ' + state;
					
					if (state == "OFF") {
						stopping = 0;
						// no info about webserver plotting
						document.getElementById("webserver").innerHTML = '';
					} else if (state == "RUNNING") {
						// get additional status info
						webserverEnabled = tempState[1];
						var currentRate = tempState[2];
						var activeVChannels = JSON.parse(tempState[5]);
						var activeIChannels = JSON.parse(tempState[6]);
						var tempFilename = tempState[7];
						var binary_file = tempState[8];
						
						
						// display webserver plotting info
						if(webserverEnabled == "1") {
							webEn = "ENABLED";
						} else {
							webEn = "DISABLED";
						}
						document.getElementById("webserver").innerHTML = 'Webserver Plotting: ' + webEn;
						
						if (webserverEnabled == "1") {
							// parse channel info
							for (var i=0; i<4; i++) { // voltages
								if (activeVChannels[i] == 1) {
									vChannels[i] = true;
								} else {
									vChannels[i] = false;
								}
							}
							
							for (var i=0; i<6; i++) { // currents
								if (activeIChannels[i] == 1) {
									iChannels[i] = true;
								} else {
									iChannels[i] = false;
								}
							}
							updateChannels();
							
							// parse filename
							normalFilename = tempFilename.slice(14);
							//if ($("#network_store:checked").length > 0) {
							filename = normalFilename;
							//}
							$("#filename").val(filename);
							
							// parse file format
							var e = document.getElementById("file_format");
							if (binary_file == "1") {
								e.selectedIndex = 0;
							} else {
								e.selectedIndex = 1;
							}
							
							// sample rate
							var e = document.getElementById("sample_rate");
							// switch statement didn't work ...
							if (currentRate == 1) {
								e.selectedIndex = 0;
							}
							if (currentRate == 2) {
								e.selectedIndex = 1;
							}
							if (currentRate == 4) {
								e.selectedIndex = 2;
							}
							if (currentRate == 8) {
								e.selectedIndex = 3;
							}
							if (currentRate == 16) {
								e.selectedIndex = 4;
							}
							if (currentRate == 32) {
								e.selectedIndex = 5;
							}
							if (currentRate == 64) {
								e.selectedIndex = 6;
							}
						}
					}
				}
			});
		}
		
		
		// DATA HANDLING
		
		// fetch data from server
		function fetchData() {
			
			var tempData;
			currentData = [];
			
			$.ajax({
				type: "post",
				url:'rl.php',
				dataType: 'json',
				data: {command: 'get_data'},
				
				complete: function (response) {
					$('#output').html(response.responseText);
					tempData = JSON.parse(response.responseText);
					for (var i = 0; i < bufferSize; i++) {
						currentData.push(JSON.parse(tempData[i]));
					}
					
					// debug outputs
					//document.getElementById("test").innerHTML = "" + currentData[2];
					
					// callback function
					dataReceived();
				}
			});
		}
		
		function updateData() {
			
			// remove old data
			if (vData.length > 0) {
				vData = vData.slice(bufferSize);
			}
			if (iData.length > 0) {
				iData = iData.slice(bufferSize);
			}
			var vDataArray;
			var iDataArray;
			// extract data
			for (var i = 0; i < bufferSize; i++ ) {
				vDataArray = currentData[i].slice(1,3).concat(currentData[i].slice(4));
				iDataArray = currentData[i].slice(0,1).concat(currentData[i].slice(3,4));
				iData.push(iDataArray);
				vData.push(vDataArray);
			}
			
		}
		
		// reset data
		function resetData() {
			// volts
			vData = [];
			var dataArray = [];
			for (var i=0; i < maxVChannels; i++) {
					dataArray.push(0);
			}
			while (vData.length < totalPoints) {
				vData.push(dataArray);
			}
			// currents
			iData = [];
			var dataArray = [];
			for (var i=0; i < maxIChannels; i++) {
					dataArray.push(0);
			}
			while (iData.length < totalPoints) {
				iData.push(dataArray);
			}
		}
		
		// convert data for plotting
		function getVData() {
			
			// generate for plot
			var plotData = [];
			
			for (var j = 0; j < maxVChannels; j++) {
				if( vChannels[j] ) {
					var channelData = [];
					for (var i = 0; i < vData.length; ++i) { // Zip the generated y values with the x values
						channelData.push([i, vData[i][j]])
					}
					var channelLabel = vNames[j];
					var plotChannel = {label: channelLabel, data: channelData};
					plotData.push(plotChannel);
				}
			}
			return plotData;
		}
		
		// convert data for plotting
		function getIData() {
			
			// generate for plot
			var plotData = [];
			
			for (var j = 0; j < maxIChannels; j++) {
				if( iChannels[j] || iChannels[j+2] || iChannels[j+4] ) {
					var channelData = [];
					for (var i = 0; i < iData.length; ++i) { // Zip the generated y values with the x values
						channelData.push([i, iData[i][j]])
					}
					var channelLabel = iNames[j];
					var plotChannel = {label: channelLabel, data: channelData};
					plotData.push(plotChannel);
				}
			}
			return plotData;
		}
		
		// PLOTTING
		
		// reset plot
		resetData();
		
		function updatePlot () {
			vPlot.setupGrid();
			vPlot.setData(getVData());
			vPlot.draw();
			iPlot.setupGrid();
			iPlot.setData(getIData());
			iPlot.draw();
		}

		var vPlot = $.plot("#vPlaceholder", getVData(), {
			series: {
				shadowSize: 0	// Drawing is faster without shadows
			},
			xaxis: {
				show: true
			}
		});
		
		var iPlot = $.plot("#iPlaceholder", getIData(), {
			series: {
				shadowSize: 0	// Drawing is faster without shadows
			},
			xaxis: {
				show: true
			}
		});
		
		
		// BUTTONS
		
		// start button
		$("#start").click(function () {
			start();
		});
		
		function start() {
		
			if(state != "OFF") {
				alert("Rocketlogger already running.\nPress Stop!");
				return false;
			}
		
			if(!parse_conf()){
				return false;
			}
			
			resetData();
			
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
			if (state == "OFF") {
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
				setTimeout(stopped, stopTimeoutTime);
			}
		}
		
		function stopped() {
			stopping = 0;
		}
		
		// calibrate button
		$("#calibrate").click(function () {
			calibrate();
		});
		
		function calibrate() {
			if(state != "OFF") {
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
			vChannels.fill(false);
			iChannels.fill(false);
			updateChannels();
		});
		
		// select button
		$("#select").click(function () {
			vChannels.fill(true);
			iChannels.fill(true);
			updateChannels();
		});
		
		function updateChannels () {
			document.getElementById("v1").checked = vChannels[0];
			document.getElementById("v2").checked = vChannels[1];
			document.getElementById("v3").checked = vChannels[2];
			document.getElementById("v4").checked = vChannels[3];
			document.getElementById("i1l").checked = iChannels[0];
			document.getElementById("i2l").checked = iChannels[1];
			document.getElementById("i1m").checked = iChannels[2];
			document.getElementById("i2m").checked = iChannels[3];
			document.getElementById("i1h").checked = iChannels[4];
			document.getElementById("i2h").checked = iChannels[5];
		}
		
		// CONFIGURATION PARSING
		
		function parse_conf() {
			
			// rate
			var e = document.getElementById("sample_rate");
			cmd_obj.rate = " -r " + e.options[e.selectedIndex].value;
			
			// update rate (ToDo: remove?)
			//var e = document.getElementById("update_rate");
			//cmd_obj.update_rate = " -u " + e.options[e.selectedIndex].value;
			
			// file
			if ($("#enable_storing:checked").length > 0) {
				//if ($("#network_store:checked").length > 0) {
				//	cmd_obj.file = " -f " + filename;
				//} else {
					cmd_obj.file = " -f /var/www/data/" +  filename;
				//}
			} else {
				cmd_obj.file = " -f 0"; // no storing
			}
			// file format
			var e = document.getElementById("file_format");
			if (e.options[e.selectedIndex].value == "dat") {
				cmd_obj.file += " -b";
			} else {
				var r = document.getElementById("sample_rate");
				if (r.options[r.selectedIndex].value == "64" || r.options[r.selectedIndex].value == "32") {
					if(!confirm("Warning: Using CSV-files with high data rates may cause overruns!")) {
						return false;
					}
				}
			}
			
			// webserver plotting
			if ($("#plotting:checked").length > 0) {
				cmd_obj.file += " -w";
			}
			
			// channels
			numChannels = 0;
			vChannels.fill(false);
			iChannels.fill(false);
			cmd_obj.channels = " -ch ";
			if ($("#i1h:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "0";
				} else {
					cmd_obj.channels += ",0";
				}
				iChannels[4] = true;
				numChannels++;
			}
			if ($("#i1m:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "1";
				} else {
					cmd_obj.channels += ",1";
				}
				iChannels[2] = true;
				numChannels++;
			}
			if ($("#i1l:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "2";
				} else {
					cmd_obj.channels += ",2";
				}
				iChannels[0] = true;
				numChannels++;
			}
			if ($("#v1:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "3";
				} else {
					cmd_obj.channels += ",3";
				}
				vChannels[0] = true;
				numChannels++;
			}
			if ($("#v2:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "4";
				} else {
					cmd_obj.channels += ",4";
				}
				vChannels[1] = true;
				numChannels++;
			}
			
			if ($("#i2h:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "5";
				} else {
					cmd_obj.channels += ",5";
				}
				iChannels[5] = true;
				numChannels++;
			}
			if ($("#i2m:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "6";
				} else {
					cmd_obj.channels += ",6";
				}
				iChannels[3] = true;
				numChannels++;
			}
			if ($("#i2l:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "7";
				} else {
					cmd_obj.channels += ",7";
				}
				iChannels[1] = true;
				numChannels++;
			}
			if ($("#v3:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "8";
				} else {
					cmd_obj.channels += ",8";
				}
				vChannels[2] = true;
				numChannels++;
			}
			if ($("#v4:checked").length > 0) {
				if (numChannels == 0) {
					cmd_obj.channels += "9";
				} else {
					cmd_obj.channels += ",9";
				}
				vChannels[3] = true;
				numChannels++;
			}
			if(numChannels == 0) {
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
				cmd_obj.force = "";
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
		
		$("#file_format").change(function () { // swap all filenames between csv and dat
			var e = document.getElementById("file_format");
			if (e.options[e.selectedIndex].value == "dat") {
				networkFilename = networkFilename.slice(0,-4) + ".dat";
				normalFilename = normalFilename.slice(0,-4) + ".dat";
				filename = filename.slice(0,-4) + ".dat";
			} else {
				networkFilename = networkFilename.slice(0,-4) + ".csv";
				normalFilename = normalFilename.slice(0,-4) + ".csv";
				filename = filename.slice(0,-4) + ".csv";
			}
			$("#filename").val(filename);
		});
		
		/*$("#network_store").change(function () { // swap between network and normal filename
			if ($("#network_store:checked").length > 0) {
				normalFilename = filename;
				filename = networkFilename;
			} else {
				networkFilename = filename;
				filename = normalFilename;
			}
			$("#filename").val(filename);
		});*/
		
		// download button
		$("#download").click(function () {
			/*if ($("#network_store:checked").length > 0) {
				alert("Network download does not work yet!"); //ToDo: network download
				return false;
			}*/
			file = 'data/' + filename;
			window.open(file);
		});
		
		// log download
		/*$("#download_log").click(function () {
			file = 'log/log.txt';
			window.open(file);
		});*/
		
		// delete button
		$("#delete").click(function () {
			
			if(state != "OFF") {
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
		update();

});