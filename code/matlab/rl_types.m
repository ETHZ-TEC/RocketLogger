%% CONSTANTS FOR RL DATA

% header
RL_FILE_MAGIC = 626150466;
MAC_ADDRESS_LENGTH = 6;

% channels
RL_UNIT_UNDEFINED = 0;
RL_UNIT_VOLT = 1;
RL_UNIT_AMPERE = 2;
RL_UNIT_BINARY = 3;
RL_UNIT_RANGE_VALID = 4;

NO_VALID_CHANNEL = 65535;

RL_FILE_CHANNEL_NAME_LENGTH = 16;

RL_FILE_SAMPLE_SIZE = 4; % sample size in byte

% time stamp
TIME_STAMP_SIZE = 4; % 4 * int64

UNIT_NAMES = ['undefined'; 'voltage  '; 'current  '; 'digital  '; 'valid    '];
