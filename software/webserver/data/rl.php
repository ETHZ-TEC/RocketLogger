<?php
/*
 * Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
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
			$command = $command . " -f /var/www/rocketlogger/data/" . $_POST['fileName'];
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
