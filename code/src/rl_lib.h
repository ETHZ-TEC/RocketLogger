#ifndef RL_LIB_H
#define RL_LIB_H

#include "lib_util.h"
#include "types.h"
#include "rl_hw.h"
//#include "rl_low_level.h" //TODO: remove

// ---------------------------------------------- UPDATE RATES ------------------------------------------------------// 

/*#define HZ1 1
#define HZ2 2
#define HZ5 5
#define HZ10 10*/



// ---------------------------------------------- Functions ---------------------------------------------------------// 

// TODO: update


void print_config(struct rl_conf_new* conf, int web);

void rl_reset_calibration();

int rl_get_data();

void print_json(float data[], int length);

int rl_get_status(int print, int web);
int rl_get_status_web();

int rl_sample(struct rl_conf_new* confn);
int rl_continuous(struct rl_conf_new* confn);
int rl_meter(struct rl_conf_new* confn);

int rl_stop();

#endif