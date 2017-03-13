<?php
    include_once('./rl_version.php');
    $hostname = php_uname('n');
?>
<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
    <title>RocketLogger<?php if ($hostname) echo " ($hostname)"; ?></title>
	<meta name="description" content="">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png">
	<link rel="icon" type="image/png" href="/favicon-32x32.png" sizes="32x32">
	<link rel="icon" type="image/png" href="/favicon-16x16.png" sizes="16x16">
	<link rel="manifest" href="/manifest.json">
	<link rel="mask-icon" href="/safari-pinned-tab.svg" color="#f08018">
	<meta name="apple-mobile-web-app-title" content="RocketLogger">
	<meta name="application-name" content="RocketLogger">
	<meta name="theme-color" content="#ff0000">
    <link rel="stylesheet" href="css/bootstrap.min.css">
    <link rel="stylesheet" href="css/main.css">
</head>
<body>
	<div class="container">
		<div class="row top-buffer">
			<div class="col-md-4 col-xs-6">
				<img class="img-responsive" src="img/eth_logo_small.png" alt="Logo ETH Zurich">
			</div>
			<div class="col-md-4">
			</div>
			<div class="col-md-4 col-xs-6">
				<img class="img-responsive" src="img/tik_logo_small.png" alt="Logo TIK" align="right">
			</div>
		</div>

		<div id="content">
			<div class="page-header">
				<div class="row">
					<div class="col-xs-10">
						<h1 style="color:black">RocketLogger Remote Control<?php if ($hostname) echo " ($hostname)"; ?></h1>
					</div>
					<div class="col-xs-2" align="right">
						<h1 class="glyphicon glyphicon-question-sign" data-toggle="tooltip_help" title="RocketLogger Hotkeys:
	s:	Start/Stop
	d:	Store Configuration as Default
	l:	Load Default Configuration
	p:	Un-/Pause Plot Update
	1-3:	Change Plot Time Scale">
						</h1>
					</div>

				</div>
			</div>

			<div class="panel panel-default">
				<h2 class="panel-heading" style="color:black">
					Status & Control
				</h2>

				<div class="panel-body">
					<div class="row top-buffer">
						<div class="col-md-3">
							<div id="status">Status: </div>
						</div>
						<div class="col-md-4">
							<div class="row">
								<div class="col-md-7 col-sm-4 col-xs-6">
									<div id="samples_taken"></div>
								</div>
								<div class="col-md-5 col-sm-8 col-xs-6">
									<div id="samples_taken_val"></div>
								</div>
							</div>
							<div class="row">
								<div class="col-md-7 col-sm-4 col-xs-6">
									<div id="time_sampled"></div>
								</div>
								<div class="col-md-5 col-sm-8 col-xs-6">
									<div id="time_sampled_val"></div>
								</div>
							</div>
						</div>
						<div class="col-md-5">
							<div class="row">
								<div class="col-md-6 col-sm-4 col-xs-6">
									Last Calibration:
								</div>
								<div class="col-md-6 col-sm-4 col-xs-6">
									<div id="calibration"></div>
								</div>
							</div>
							<div class="row">
								<div class="col-md-6 col-sm-4 col-xs-6">
									<div>Free Disk Space:</div>
								</div>
								<div class="col-md-6 col-sm-8 col-xs-6">
									<div id="free_space"></div>
								</div>
							</div>
							<div class="row">
								<div class="col-md-6 col-sm-4 col-xs-6">
									<div>Sampling Time Left:</div>
								</div>
								<div class="col-md-6 col-sm-8 col-xs-6">
									<div id="time_left"></div>
								</div>
							</div>
						</div>
					</div>
					<div class="row top-buffer">
						<div class="col-md-2">
							<div class="row">
								<div class="col-md-6 col-sm-4 col-xs-6">
									<button id="start" class="btn btn-default" data-toggle="tooltip" title="Start new measurement" onclick="start()">Start</button>
								</div>
								<div class="col-md-6 col-sm-8 col-xs-6">
									<button id="stop" class="btn btn-default" data-toggle="tooltip" title="Stop current measurement" onclick="stop()">Stop</button>
								</div>
							</div>
						</div>
						<div class="col-md-1">
						</div>
						<div class="col-md-6">
							<div class="checkbox">
								<label data-toggle="tooltip" title="Refresh the web interface in background"><input id="active" type="checkbox" checked="checked">Refresh in Background</label>
							</div>
						</div>
					</div>
				</div>
			</div>

			<div class="panel-group" id="accordion" >
				<div class="panel panel-default">
					<div class="panel-heading">
						<h2 class="accordion-toggle" data-toggle="collapse" href="#collapse2">
							 Configuration
						</h2>
					</div>
					<div id="collapse2" class="panel-collapse collapse in">
						<div class="panel-body">
							<div class="row top-buffer">
								<div class="col-sm-4">
									<button id="set_default" class="btn btn-default" data-toggle="tooltip" title="Store current configuration as default" onclick="setDefault()">Store Configuration</button>
								</div>
								<div class="col-sm-4">
									<button id="load_default" class="btn btn-default" data-toggle="tooltip" title="Restore default configuration">Restore Configuration</button>
								</div>
							</div>

							<!-- File Section -->
							<div class="row top-buffer">
								<div class="col-md-6">
									<h3>File</h3>
									<div class="row">
										<div class="col-md-6">
											<div class="checkbox">
												<label data-toggle="tooltip" title="Store data to file"><input id="enable_storing" type="checkbox" checked="checked" onclick="enableDisableConf()">Enable File Storing</label>
											</div>
										</div>
									</div>
									<div class="row">
										<div class="col-md-3">
											File Format:
										</div>
										<div class="col-md-3">
											<select class="form-control"  id="file_format">
												<option value="bin">binary</option>
												<option value="csv">csv</option>
											</select>
										</div>
										<div class="col-md-1">
										</div>
										<div class="col-md-3">
											<button id="date_to_filename" class="btn btn-default" data-toggle="tooltip" title="Add date prefix to file name">Add Date Prefix</button>
										</div>
									</div>
									<div class="row small-top-buffer">
										<div class="col-md-3">
											Filename:
										</div>
										<div class="col-md-7">
											<input class="form-control" id="filename" placeholder=filename type="text" value="">
										</div>
									</div>
									<div class="row small-top-buffer">
										<div class="col-md-3">
										</div>
										<div class="col-md-4 col-sm-4">
											<button id="download" class="btn btn-default" data-toggle="tooltip" title="Download current file" onclick="download()">Download</button>
										</div>
										<div class="col-md-3 col-sm-8">
											<button id="list" class="btn btn-default" data-toggle="tooltip" title="Show all files" onclick="browseFiles()">Browse Files</button>
										</div>
									</div>
									<div class="row top-buffer">
										<div class="col-md-3">
											<div class="checkbox">
												<label data-toggle="tooltip" title="Split data files, if larger than maximum"><input id="file_size_limited" type="checkbox" onclick="enableDisableConf()">Split Files</label>
											</div>
										</div>
										<div class="col-md-2">
											<input type="text" class="form-control" id="file_size" value="1" data-toggle="tooltip" title="Maximum file size" disabled>
										</div>
										<div class="col-md-3">
											<select class="form-control"  id="file_size_unit" disabled>
												<option value="m">MB</option>
												<option selected="selected" value="g">GB</option>
											</select>
										</div>
									</div>
									<div class="row top-buffer">
										<div class="col-md-3">
											Log File:
										</div>
										<div class="col-md-3">
											<button id="download_log" class="btn btn-default" data-toggle="tooltip" title="Show log file" onclick="showLog()">Open Log File</button>
										</div>
									</div>
								</div>

								<!-- Channels Section -->
								<div class="col-md-6">
									<h3>Analog Channels</h3>
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox">
												<label data-toggle="tooltip" title="Ignore existing calibration"><input id="ignore_calibration" type="checkbox">Calibration Measurement (Ignore Calibration)</label>
											</div>
										</div>
									</div>
									<div class="row top-buffer">
										<div class="col-md-4">
											<div class="row">
												<div class="col-md-12">
													<b>Voltage</b>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample V1"><input id="v1" type="checkbox" checked="checked">V1</label>
													</div>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample V2"><input id="v2" type="checkbox" checked="checked">V2</label>
													</div>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample V3"><input id="v3" type="checkbox" checked="checked">V3</label>
													</div>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample V4"><input id="v4" type="checkbox" checked="checked">V4</label>
													</div>
												</div>
											</div>
										</div>
										<div class="col-md-4">
											<div class="row">
												<div class="col-md-12">
													<b>Current 1</b>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample I1H"><input id="i1h" type="checkbox" checked="checked">I1H</label>
													</div>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample I1L"><input id="i1l" type="checkbox" checked="checked">I1L</label>
													</div>
												</div>
											</div>
											<div class="row">
												<p></p>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Force I1 to high range"><input id="fhr1" type="checkbox">Force High Range</label>
													</div>
												</div>
											</div>
										</div>
										<div class="col-md-4">
											<div class="row">
												<div class="col-md-12">
													<b>Current 2</b>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample I2H"><input id="i2h" type="checkbox" checked="checked">I2H</label>
													</div>
												</div>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Sample I2L"><input id="i2l" type="checkbox" checked="checked">I2L</label>
													</div>
												</div>
											</div>
											<div class="row">
												<p></p>
											</div>
											<div class="row">
												<div class="col-md-12">
													<div class="checkbox">
														<label data-toggle="tooltip" title="Force I2 to high range"><input id="fhr2" type="checkbox">Force High Range</label>
													</div>
												</div>
											</div>
										</div>
									</div>
									<div class="row">
										<div class="col-md-4">
											<button id="select" class="btn btn-default" data-toggle="tooltip" title="Select all analog channels">Select All</button>
										</div>
										<div class="col-md-4">
											<button id="deselect" class="btn btn-default" data-toggle="tooltip" title="Deselect all analog channels">Deselect All</button>
										</div>
									</div>
								</div>
							</div>

							<!-- Rate Section -->
							<div class="row">
								<div class="col-md-6">
									<h3>Sampling</h3>
									<div class="row">
										<div class="col-md-3">
											Sample Rate:
										</div>
										<div class="col-md-4">
											<select class="form-control"  id="sample_rate" data-toggle="tooltip" title="Data sampling rate">
												<option value=1>1Sps</option>
												<option value=10>10Sps</option>
												<option value=100>100Sps</option>
												<option selected="selected" value=1000>1kSps</option>
												<option value=2000>2kSps</option>
												<option value=4000>4kSps</option>
												<option value=8000>8kSps</option>
												<option value=16000>16kSps</option>
												<option value=32000>32kSps</option>
												<option value=64000>64kSps</option>
											</select>
										</div>
									</div>
								</div>
								<div class="col-md-6">
									<h3>Digital Channels</h3>
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox">
												<label data-toggle="tooltip" title="Sample digital inputs"><input id="digital_inputs" type="checkbox" checked="checked">Sample Digital Inputs</label>
											</div>
										</div>
									</div>
								</div>
							</div>
						</div>
					</div>
				</div>

				<div class="panel panel-default">
					<div class="panel-heading">
						<h2 class="accordion-toggle" data-toggle="collapse" href="#collapse3" onclick="plotsCollapsed()">
							 Plots
						</h2>
					</div>
					<div id="collapse3" class="panel-collapse collapse in">
						<div class="panel-body">
							<div class="row">
								<div class="col-md-2">
									<div class="checkbox">
										<label data-toggle="tooltip" title="Activate plot update"><input id="plotting" type="checkbox" checked="checked">Update Plot</label>
									</div>
								</div>
							</div>
							<div class="row">
								<div class="col-md-2">
									Time Scale:
								</div>
								<div class="col-md-2">
									<select class="form-control" id="time_scale" data-toggle="tooltip" title="Plot time scale">
										<option value=0>1s/div</option>
										<option value=1>10s/div</option>
										<option value=2>100s/div</option>
									</select>
								</div>
								<div class="col-md-2">
									<div id="dataAvailable" class="text-danger"></div>
								</div>
							</div>
							<div class="row">
								<h3>Voltages:</h3>
							</div>
							<div class="row">
								<div class="col-sm-2">
								</div>
								<div class="col-sm-2">
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#0072BD">
												<label data-toggle="tooltip" title="Show V1 in plot"><input id="plot_v1" type="checkbox"  checked="checked" onclick="updatePlot()">Plot V1</label>
											</div>
										</div>
									</div>
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#D95319">
												<label data-toggle="tooltip" title="Show V2 in plot"><input id="plot_v2" type="checkbox"  checked="checked" onclick="updatePlot()">Plot V2</label>
											</div>
										</div>
									</div>
								</div>
								<div class="col-sm-2">
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#EDB120">
												<label data-toggle="tooltip" title="Show V3 in plot"><input id="plot_v3" type="checkbox"  checked="checked" onclick="updatePlot()">Plot V3</label>
											</div>
										</div>
									</div>
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#77AC30">
												<label data-toggle="tooltip" title="Show V4 in plot"><input id="plot_v4" type="checkbox"  checked="checked" onclick="updatePlot()">Plot V4</label>
											</div>
										</div>
									</div>
								</div>
								<div class="col-sm-2">
									Voltage Range:
								</div>
								<div class="col-sm-2">
									<select class="form-control" id="voltage_range" onChange="updatePlot()">
										<option value=0>Auto</option>
										<option value=0.1>100µV</option>
										<option value=1>1mV</option>
										<option value=10>10mV</option>
										<option value=100>100mV</option>
										<option value=1000>1V</option>
										<option value=6000>6V</option>
										<option value=10000>10V</option>
									</select>
								</div>
							</div>
							<div class="row">
								<div id="v_plot" class="analog-container">
									<div id="vPlaceholder" class="plot-placeholder"></div>
								</div>
							</div>
							<div class="row">
								<h3>Currents:</h3>
							</div>
							<div class="row">
								<div class="col-sm-2">
								</div>
								<div class="col-sm-2">
									<div class="checkbox" style="color:#0072BD">
										<label data-toggle="tooltip" title="Show I1 in plot"><input id="plot_i1" type="checkbox"  checked="checked" onclick="updatePlot()">Plot I1</label>
									</div>
								</div>
								<div class="col-sm-2">
									<div class="checkbox" style="color:#D95319">
										<label data-toggle="tooltip" title="Show I2 in plot"><input id="plot_i2" type="checkbox"  checked="checked" onclick="updatePlot()">Plot I2</label>
									</div>
								</div>
								<div class="col-sm-2">
									Current Range:
								</div>
								<div class="col-sm-2">
									<select class="form-control" id="current_range" onChange="updatePlot()">
										<option value=0>Auto</option>
										<option value=0.01>10nA</option>
										<option value=0.1>100nA</option>
										<option value=1>1µA</option>
										<option value=10>10µA</option>
										<option value=100>100µA</option>
										<option value=1000>1mA</option>
										<option value=10000>10mA</option>
										<option value=100000>100mA</option>
										<option value=1000000>1A</option>
									</select>
								</div>
							</div>
							<div class="row">
								<div id="i_plot" class="analog-container">
									<div id="iPlaceholder" class="plot-placeholder"></div>
								</div>
							</div>
							<div class="row">
								<h3>Digital Signals:</h3>
							</div>
							<div class="row">
								<div class="col-sm-2">
								</div>
								<div class="col-sm-2">
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#0072BD">
												<label data-toggle="tooltip" title="Show DI1 in plot"><input id="plot_d1" type="checkbox"  checked="checked" onclick="updatePlot()">Plot DI1</label>
											</div>
										</div>
									</div>
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#D95319">
												<label data-toggle="tooltip" title="Show DI2 in plot"><input id="plot_d2" type="checkbox"  checked="checked" onclick="updatePlot()">Plot DI2</label>
											</div>
										</div>
									</div>
								</div>
								<div class="col-sm-2">
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#EDB120">
												<label data-toggle="tooltip" title="Show DI3 in plot"><input id="plot_d3" type="checkbox"  checked="checked" onclick="updatePlot()">Plot DI3</label>
											</div>
										</div>
									</div>
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#7E2F8E">
												<label data-toggle="tooltip" title="Show DI4 in plot"><input id="plot_d4" type="checkbox"  checked="checked" onclick="updatePlot()">Plot DI4</label>
											</div>
										</div>
									</div>
								</div>
								<div class="col-sm-2">
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#77AC30">
												<label data-toggle="tooltip" title="Show DI5 in plot"><input id="plot_d5" type="checkbox"  checked="checked" onclick="updatePlot()">Plot DI5</label>
											</div>
										</div>
									</div>
									<div class="row">
										<div class="col-md-12">
											<div class="checkbox" style="color:#4DBEEE">
												<label data-toggle="tooltip" title="Show DI6 in plot"><input id="plot_d6" type="checkbox"  checked="checked" onclick="updatePlot()">Plot DI6</label>
											</div>
										</div>
									</div>
								</div>
							</div>
							<div class="row">
								<div id="dig_plot" class="digital-container">
									<div id="digPlaceholder" class="plot-placeholder"></div>
								</div>
							</div>
						</div>
					</div>
				</div>
			</div>
		</div>

        <footer id="footer">
            <p>RocketLogger version <?php echo ROCKETLOGGER_VERSION; ?>  - <a href="http://rocketlogger.ethz.ch/">https://rocketlogger.ethz.ch/</a><br />
                &copy; <?php echo date("Y"); ?>, ETH Zurich, Computer Engineering Group.
            </p>
        </footer>
		
        <script src="//ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js"></script>
        <script>window.jQuery || document.write('<script src="js/vendor/jquery-3.1.1.min.js"><\/script>')</script>
        <script src="js/vendor/bootstrap.min.js"></script>
		<script src="js/vendor/jquery.flot.min.js"></script>
		<script src="js/vendor/jquery.flot.time.min.js"></script>
        <script src="js/main.js"></script>
	</div>
</body>
</html>
