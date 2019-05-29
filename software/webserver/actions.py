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
import re
from subprocess import check_output
from flask import make_response, request


def _config_decode(config):
    return []


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
    reply = json.dumps(json_reply)
    return make_response(reply, 400)


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
        config_args = _config_decode(data['config'])
    except ValueError:
        return invalid_request('Failed decoding JSON configuration data.',
                               action='start', data=data)

    # call rocketlogger CLI
    try:
        start_output = check_output(['rocketlogger', 'cont'] + config_args,
                                    timeout=1000, text=True)
        json_reply['reply']['output'] = start_output
        json_reply['reply']['start'] = 'OK'
    except FileNotFoundError:
        json_reply['reply']['error'] = 'RocketLogger binary not found. '\
                                       'Check your system configruation.'
        json_reply['reply']['start'] = 'FAILED'
    except:
        json_reply['reply']['error'] = 'Starting measurement failed'
        json_reply['reply']['start'] = 'FAILED'

    # start: 200 ok
    reply = json.dumps(json_reply)
    return make_response(reply, 200)


def stop(data):
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

    # call rocketlogger CLI
    try:
        start_output = check_output(['rocketlogger', 'stop'],
                                    timeout=1000, text=True)
        json_reply['reply']['output'] = start_output
        json_reply['reply']['stop'] = 'OK'
    except FileNotFoundError:
        json_reply['reply']['error'] = 'RocketLogger binary not found. '\
                                       'Check your system configruation.'
        json_reply['reply']['stop'] = 'FAILED'
    except:
        json_reply['reply']['error'] = 'Stopping measurement failed.'
        json_reply['reply']['stop'] = 'FAILED'

    # stop: 200 ok
    reply = json.dumps(json_reply)
    return make_response(reply, 200)


def status(data):
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

    # call rocketlogger web CLI for status
    try:
        status = check_output(['rocketloggers', 'status'],
                              timeout=1000, text=True)
        json_reply['reply']['status'] = json.loads(status)
    except FileNotFoundError:
        json_reply['reply']['error'] = 'RocketLogger binary not found. '\
                                       'Check your system configruation.'
        json_reply['reply']['status'] = None
    except:
        json_reply['reply']['error'] = 'Getting status failed.'
        json_reply['reply']['status'] = None

    # get status: 200 ok
    reply = json.dumps(json_reply)
    return make_response(reply, 200)


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
    if data['set_default']:
        # reply with stored config
        json_reply['reply']['config'] = data['config']
        # set config: 201 created
        reply = json.dumps(json_reply)
        return make_response(reply, 201)

    # load and reply with stored configuration
    json_reply['reply']['config'] = data['config']

    # get config: 200 ok
    reply = json.dumps(json_reply)
    return make_response(reply, 200)


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
    reply = json.dumps(json_reply)
    return make_response(reply, 202)
