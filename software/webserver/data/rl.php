<?php
/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

    if (isset($_POST['command'])) {
		switch($_POST['command']) {
			
            case 'start':
				// command to post
				$command = "rocketlogger cont";
				
				$config = parsePost();
				
				// return error if parse error
				if($config != false) {
					exec($command . $config . " > /dev/null &");
					echo json_encode("SUCCESS");
				} else {
					echo json_encode(["ERROR", "Argument parse errror"]);
				}
				break;
			
			case 'set_conf':
				// command to post
				$command = "rocketlogger set";
				
				$config = parsePost();
				
				// return error if parse error
				if($config != false) {
					exec($command . $config);
					echo json_encode("SUCCESS");
				} else {
					echo json_encode(["ERROR", "Argument parse errror"]);
				}
				break;
			
			case 'status':
				$parse_error = false;
				
				if (isset($_POST['id'])) { // filter empty posts
				
					// check attributes
					if(preg_match("/^\d+$/", $_POST['id']) != 1) {
						$parse_error = true;
					}
					if(preg_match("/^\d$/", $_POST['fetchData']) != 1) {
						$parse_error = true;
					}
					if(preg_match("/^\d$/", $_POST['timeScale']) != 1) {
						$parse_error = true;
					}
					if(preg_match("/^\d+$/", $_POST['time']) != 1) {
						$parse_error = true;
					}
				} else {
					$parse_error = true;
				}
				
				if($parse_error == false) {
					
					$command = 'rocketloggers ' . $_POST['id'] . ' ' . $_POST['fetchData'] . ' ' . $_POST['timeScale'] . ' ' . $_POST['time'];
					exec($command, $output);
					echo json_encode($output);
					
				} else {
					echo json_encode("ERROR");
				}
				
				break;
			
			case 'stop':
				echo exec('rocketlogger stop');
				break;
			
			default:
               echo json_encode(["ERROR", "Undefined error"]);
               break;
        }
    }
	
	function parsePost() {
		$config = '';
		
		// parse and check attributes
		if(preg_match("/^\d+[k]?$/", $_POST['sampleRate']) == 1) {
			$command = $command . " -r " . $_POST['sampleRate'];
		} else {
			return false;
		}
		if(preg_match("/^\d+$/", $_POST['updateRate']) == 1) {
			$command = $command . " -u " . $_POST['updateRate'];
		} else {
			return false;
		}
		if(preg_match("/^\d(,\d)*$/", $_POST['channels']) == 1) {
			$command = $command . " -ch " . $_POST['channels'];
		} else {
			return false;
		}
		if(preg_match("/^\d(,\d)?$/", $_POST['forceHigh']) == 1) {
			$command = $command . " -fhr " . $_POST['forceHigh'];
		} else {
			return false;
		}
		if($_POST['ignoreCalibration'] == '1') {
			$command = $command . " -c 0";
		} else {
			$command = $command . " -c";
		}
		if($_POST['fileName'] == '0') {
			$command = $command . " -f 0";
		} else if(preg_match("/^[\w-]+\.[A-Za-z]{3}$/", $_POST['fileName']) == 1) {
			$command = $command . " -f /var/www/data/" . $_POST['fileName'];
		} else {
			return false;
		}
		if($_POST['fileFormat'] == "bin") {
			$command = $command . " -format bin";
		} else if($_POST['fileFormat'] == "csv") {
			$command = $command . " -format csv";
		}
		if($_POST['fileSize'] == '0') {
			$command = $command . " -size 0";
		} else if(preg_match("/^\d+[mg]?$/", $_POST['fileSize']) == 1) {
			$command = $command . " -size " . $_POST['fileSize'];
		} else {
			return false;
		}
		if($_POST['digitalInputs'] == '1') {
			$command = $command . " -d";
		} else {
			$command = $command . " -d 0";
		}
		if($_POST['webServer'] == '1') {
			$command = $command . " -w";
		} else {
			$command = $command . " -w 0";
		}
		if($_POST['setDefault'] == '1') {
			$command = $command . " -s";
		}
		if(isset($_POST['fileComment'])) {
			$command = $command . " -C '" . str_replace("'", "\'", $_POST['fileComment']) . "'";
		}

		return $command;
	}

?>
	