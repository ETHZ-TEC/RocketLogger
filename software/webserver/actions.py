"""
RocketLogger Data Import Support.

File reading support for RocketLogger data (rld) files.

Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import json
import os
from subprocess import run, Popen, PIPE, CalledProcessError, TimeoutExpired
from flask import make_response

ROCKETLOGGER_CMD_TIMEOUT = 500
ROCKETLOGGER_DATA_PATH = '/var/www/rocketlogger/data'


def _config_to_args(config):
    args = []
    args.append('--samples=0')  # no sample limit
    args.append('--update=1')  # hardcode update rate for now
    # args.append('--update={}'.format(config['update_rate']))

    args.append('--ambient={}'.format(config['ambient_enable']))
    args.append('--channel={}'.format(','.join(config['channel_enable'])))
    args.append(
        '--high-range={}'.format(','.join(config['channel_force_range'])))
    args.append('--digital={}'.format(config['digital_enable']))
    if config['file'] is None:
        args.append('--output=0')
    else:
        filename = os.path.join(ROCKETLOGGER_DATA_PATH,
                                config['file']['filename'])
        args.append('--output={}'.format(filename))
        args.append('--format={}'.format(config['file']['format']))
        args.append('--size={}'.format(config['file']['size']))
        args.append('--comment=\'{}\''.format(config['file']['comment']))
    args.append('--rate={}'.format(config['sample_rate']))
    args.append('--web={}'.format(config['web_enable']))

    return args


def invalid_request(message, action=None, data=None):
    json_reply = {
        'request': {
            'action': action,
            'data': data,
        },
        'reply': {
            'error': message,
            'warning': None,
            'message': None,
        },
    }

    return (json_reply, 400)


def invalid_response(message, action=None, data=None):
    json_response, status = invalid_request(message, action, data)
    response = json.dumps(json_response)
    return make_response(response, status)


def start(data):
    json_reply = {
        'request': {
            'action': 'start',
            'data': data,
        },
        'reply': {
            'error': None,
            'warning': None,
            'message': None,
        },
    }

    # parse configuration
    try:
        config_args = _config_to_args(data['config'])
    except ValueError:
        return invalid_request('Failed decoding JSON configuration data.',
                               action='start', data=data)

    # call rocketlogger CLI to start measurement
    json_reply['reply']['start'] = None
    try:
        cmd = ['rocketlogger', 'start', '--background'] + config_args
        start_proc = Popen(cmd, text=True, stdout=PIPE)
        (start_stdout, start_stderr) = start_proc.communicate(
            timeout=ROCKETLOGGER_CMD_TIMEOUT)
        json_reply['reply']['start'] = start_stdout
    except FileNotFoundError:
        json_reply['reply']['error'] = 'RocketLogger binary not found. '\
                                       'Check your system configruation.'
        return (json_reply, 501)
    except TimeoutExpired:
        json_reply['reply']['error'] = 'Starting measurement failed'
        json_reply['reply']['message'] = 'Starting measurement failed'
        return (json_reply, 503)

    # start: 200 ok
    return (json_reply, 200)


def stop(data):
    json_reply = {
        'request': {
            'action': 'stop',
            'data': data,
        },
        'reply': {
            'error': None,
            'warning': None,
            'message': None,
        },
    }

    # call rocketlogger CLI to stop measurement
    json_reply['reply']['stop'] = None
    try:
        cmd = ['rocketlogger', 'stop']
        stop_cmd = run(cmd, check=True, text=True, capture_output=True,
                       timeout=ROCKETLOGGER_CMD_TIMEOUT)
        json_reply['reply']['stop'] = stop_cmd.stdout
    except FileNotFoundError:
        json_reply['reply']['error'] = 'RocketLogger binary not found. '\
                                       'Check your system configruation.'
        return (json_reply, 501)
    except CalledProcessError:
        json_reply['reply']['error'] = 'Stopping measurement failed.'
        json_reply['reply']['message'] = ' '.join(cmd)
        return (json_reply, 503)

    # stop: 200 ok
    return (json_reply, 200)


def status(data):
    json_reply = {
        'request': {
            'action': 'status',
            'data': data,
        },
        'reply': {
            'error': None,
            'warning': None,
            'message': None,
        },
    }

    # call rocketlogger CLI for status
    json_reply['reply']['status'] = None
    try:
        cmd = ['rocketlogger', 'status', '--json']
        status_cmd = run(cmd, check=True, text=True, capture_output=True,
                         timeout=ROCKETLOGGER_CMD_TIMEOUT)
        json_reply['reply']['status'] = json.loads(status_cmd.stdout)
    except FileNotFoundError:
        json_reply['reply']['error'] = 'RocketLogger binary not found. '\
                                       'Check your system configruation.'
        return (json_reply, 501)
    except CalledProcessError:
        json_reply['reply']['error'] = 'Getting status failed.'
        json_reply['reply']['message'] = ' '.join(cmd)
        return (json_reply, 503)

    # get status: 200 ok
    return (json_reply, 200)


def config(data):
    json_reply = {
        'request': {
            'action': 'config',
            'data': data,
        },
        'reply': {
            'error': None,
            'warning': None,
            'message': None,
        },
    }

    # call rocketlogger CLI to set/get default config
    json_reply['reply']['config'] = None

    # decode additinal config if reqested setting new default
    config_args = []
    if data['set_default']:
        # parse configuration
        try:
            config_args = _config_to_args(data['config']) + ['--default']
        except ValueError:
            return invalid_request('Failed decoding JSON configuration data.',
                                   action='start', data=data)

    # get/set config command
    try:
        cmd = ['rocketlogger', 'config', '--json'] + config_args
        config_cmd = run(cmd, check=True, text=True, capture_output=True,
                         timeout=ROCKETLOGGER_CMD_TIMEOUT)
        json_reply['reply']['config'] = json.loads(config_cmd.stdout)
    except FileNotFoundError:
        json_reply['reply']['error'] = 'RocketLogger binary not found. ' \
                                       'Check your system configruation.'
        return (json_reply, 501)
    except:
        json_reply['reply']['error'] = 'Setting configuration failed.'
        json_reply['reply']['message'] = ' '.join(cmd)
        return (json_reply, 503)

    # strip the path from the filename
    if json_reply['reply']['config'] and json_reply['reply']['config']['file']:
        json_reply['reply']['config']['file']['filename'] = \
            os.path.basename(
                json_reply['reply']['config']['file']['filename'])

    # generate success response
    if data['set_default']:
        # set config: 201 created
        return (json_reply, 201)
    else:
        # get config: 200 ok
        return (json_reply, 200)


def calibrate(data):
    json_reply = {
        'request': {
            'action': 'start',
            'data': data,
        },
        'reply': {
            'error': None,
            'warning': None,
            'message': None,
        },
    }

    raise NotImplementedError()

    # calibrate: 202 accepted (handled later)
    return (json_reply, 202)
