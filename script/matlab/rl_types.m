%%
%% Copyright (c) 2016-2018, ETH Zurich, Computer Engineering Group
%% All rights reserved.
%% 
%% Redistribution and use in source and binary forms, with or without
%% modification, are permitted provided that the following conditions are met:
%% 
%% * Redistributions of source code must retain the above copyright notice, this
%%   list of conditions and the following disclaimer.
%% 
%% * Redistributions in binary form must reproduce the above copyright notice,
%%   this list of conditions and the following disclaimer in the documentation
%%   and/or other materials provided with the distribution.
%% 
%% * Neither the name of the copyright holder nor the names of its
%%   contributors may be used to endorse or promote products derived from
%%   this software without specific prior written permission.
%% 
%% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
%% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
%% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
%% DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
%% FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
%% DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
%% SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
%% CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
%% OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
%% OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
%%

%% CONSTANTS FOR RL DATA

% header
RL_FILE_MAGIC = 1145852453;
RL_FILE_MAGIC_OLD = 626150466;
MAC_ADDRESS_LENGTH = 6;

% channels
RL_UNIT_NONE = 0;
RL_UNIT_VOLT = 1;
RL_UNIT_AMPERE = 2;
RL_UNIT_BINARY = 3;
RL_UNIT_RANGE_VALID = 4;
RL_UNIT_LUX = 5;
RL_UNIT_DEG_C = 6;
RL_UNIT_INTEGER = 7;
RL_UNIT_PERCENT = 8;
RL_UNIT_PASCAL = 9;
RL_UNIT_UNDEFINED = 2^32-1;

%RL_UNIT_MERGED = 5;

NO_VALID_CHANNEL = 2^16-1;

RL_FILE_CHANNEL_NAME_LENGTH = 16;

RL_FILE_SAMPLE_SIZE = 4; % sample size in byte

% time stamp
TIME_STAMP_SIZE = 4; % 4 * int64

UNIT_NAMES = {'unitless', 'voltage', 'current', 'digital', 'valid', ...
    'illuminance', 'temerature', 'integer', 'percent', 'preasure'};

RANGE_MARGIN = 5;
