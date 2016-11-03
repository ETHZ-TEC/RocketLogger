<?php

    if (isset($_POST['command'])) {
		switch($_POST['command']) {
			case 'start':
				$command = 'sudo rocketlogger cont -w -u 1' . $_POST['file'] . $_POST['file_format'] . $_POST['rate'] . $_POST['channels'] . $_POST['force'] . $_POST['digital_inputs'] . ' > /dev/null &';
				exec($command);
				break;
            case 'status':
			$command = 'rocketloggers ' . $_POST['id'] . ' ' . $_POST['fetchData'] . ' ' . $_POST['timeScale'] . ' ' . $_POST['time'];
				exec($command, $output);
				echo json_encode($output);
				break;
			case 'stop':
				echo exec('sudo rocketlogger stop');
				break;
            case 'get_data':
				exec('sudo rocketlogger data', $output);
				echo json_encode($output);
				break;
			case 'calibrate':
				exec('sudo rocketlogger calibrate');
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
	