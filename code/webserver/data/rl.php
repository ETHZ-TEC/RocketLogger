<?php

    if (isset($_POST['command'])) {
		switch($_POST['command']) {
			case 'start':
				$command = 'rocketlogger cont -w -u 1' . $_POST['file'] . $_POST['file_format'] . $_POST['rate'] . $_POST['channels'] . $_POST['force'] . $_POST['digital_inputs'] . ' > /dev/null &';
				exec($command);
				break;
            case 'status':
				if (isset($_POST['id'])) { // filter empty posts
					$command = 'rocketloggers ' . $_POST['id'] . ' ' . $_POST['fetchData'] . ' ' . $_POST['timeScale'] . ' ' . $_POST['time'];
					exec($command, $output);
					echo json_encode($output);
				}
				break;
			case 'stop':
				echo exec('rocketlogger stop');
				break;
            case 'get_data':
				exec('rocketlogger data', $output);
				echo json_encode($output);
				break;
			case 'delete':
				$command = 'rm ' . $_POST['filename'];
				exec($command);
				break;
			default:
               echo "Error";
               break;
        }
    }

?>
	