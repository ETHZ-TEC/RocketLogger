<?php

    if (isset($_POST['command'])) {
		switch($_POST['command']) {
			case 'start':
				$command = 'sudo rocketlogger cont' . $_POST['file'] . $_POST['rate'] . $_POST['update_rate'] . $_POST['channels'] . $_POST['force'] . ' > /dev/null &';
				exec($command);
				break;
            case 'status':
				exec('sudo rocketlogger status -w',$output);
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
	