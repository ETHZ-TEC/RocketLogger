#include "sem.h"


int create_sem() {
	int sem_id = semget(SEM_KEY, NUM_SEMS, IPC_CREAT | S_IRWXU); // TODO: adapt permissions
	if(sem_id < 0) {
		rl_log(ERROR, "failed to create semaphores. Errno = %d", errno);
	}
	return sem_id;
}

int remove_sem(int sem_id) {
	// remove semaphores
	if(semctl(sem_id, DATA_SEM, IPC_RMID) < 0) {
		rl_log(ERROR, "failed to remove semaphores. Errno = %d", errno);
		return FAILURE;
	}
	return SUCCESS;
}

int open_sem() {
	int sem_id = semget(SEM_KEY, NUM_SEMS, S_IRWXU); // TODO: adapt permissions
	if(sem_id < 0) {
		rl_log(ERROR, "failed to open semaphores. Errno = %d", errno);
	}
	return sem_id;
}

int wait_sem(int sem_id, int sem_num, int time_out) {
	// operation on semaphore
	int num_ops = 1;
	int sem_op = -1;
	struct sembuf sem_ops = {sem_num, sem_op, NO_FLAG};
	struct timespec t_out = {time_out, 0};
	
	if(semtimedop(sem_id, &sem_ops, num_ops, &t_out) < 0) {
		if(errno == EAGAIN) {
			rl_log(ERROR, "time-out waiting on semaphore");
			return TIME_OUT;
		} else if(errno == EIDRM) {
			rl_log(INFO, "waiting on semaphore failed: semaphore removed");
			return TIME_OUT;
		} else if(errno == EINVAL) {
			rl_log(WARNING, "waiting on semaphore failed: semaphore not existing");
			return FAILURE;
		} else {
			rl_log(ERROR, "failed doing operation on semaphore. Errno = %d", errno);
			return FAILURE;
		}
	}
	
	return SUCCESS;
}

int set_sem(int sem_id, int sem_num, int val) {
	// operation on semaphore
	int num_ops = 1;
	int sem_op = val;
	struct sembuf sem_ops = {sem_num, sem_op, NO_FLAG};
	struct timespec time_out = {SEM_SET_TIME_OUT, 0};
	
	if(semtimedop(sem_id, &sem_ops, num_ops, &time_out) < 0) {
		if(errno == EAGAIN) {
			rl_log(ERROR, "time-out waiting on semaphore");
			return TIME_OUT;
		} else {
			rl_log(ERROR, "failed doing operation on semaphore. Errno = %d", errno);
			return FAILURE;
		}
	}
	
	return SUCCESS;
}