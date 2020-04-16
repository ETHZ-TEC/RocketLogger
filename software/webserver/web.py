"""
RocketLogger web interface main Flask app.

Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

import os
import json
from datetime import datetime, timezone
from socket import gethostname
from subprocess import check_output
from flask import Flask, make_response, render_template, request, \
    send_from_directory, url_for
from flask_socketio import SocketIO, emit

import actions


ROCKETLOGGER_DATA_DIR = '/home/rocketlogger/public_html/media'
ROCKETLOGGER_LOG_FILE = '/var/log/rocketlogger.log'
WEB_VERSION = 1.99
ZMQ_LISTENING_PORT = 5555

application = Flask(__name__)
# sockets = Sockets(application)

# context = zmq.Context()


def render_page(template_name_or_list, **context):
    error_messages = []
    warning_messages = []
    try:
        version_string = check_output(['rocketlogger', '--version'], text=True)
        binary_version = version_string.split('\n')[0].split(' ')[-1]
        if str(WEB_VERSION) != binary_version:
            warning_messages.append(
                'Potentially incompatible binary and web interface versions '
                '(interface: v{}, binary: v{})'.format(WEB_VERSION,
                                                       binary_version)
            )
    except FileNotFoundError:
        version_string = None
        error_messages.append('RocketLogger binary was not found. '
                              'Check your system configuration!')
    except:
        version_string = None
        error_messages.append('Failed getting RocketLogger binary version. '
                              'Check your system configuration!')
    context['error_messages'] = (error_messages +
                                 context.get('error_messages', []))
    context['warning_messages'] = (warning_messages +
                                   context.get('warning_messages', []))
    return render_template(
        template_name_or_list,
        **context,
        today=datetime.now(timezone.utc),
        hostname=gethostname(),
        version=WEB_VERSION,
        version_string=version_string,
    )


@application.route('/')
def home():
    # return render_page('control.html')
    return render_page('base.html')


@application.route('/control/')
def control():
    return render_page('control.html')


@application.route('/control/<action>', methods=['POST'])
def control_action(action):
    # action is required to be POST requests
    if action != request.form.get('action', None):
        return actions.invalid_response('ambiguous action in the request data')

    # check for available request data
    try:
        data = json.loads(request.form['data'])
    except KeyError:
        return actions.invalid_response('Invalid request: no data')

    if action == 'start':
        json_response, status = actions.start(data)
    elif action == 'stop':
        json_response, status = actions.stop(data)
    elif action == 'status':
        json_response, status = actions.status(data)
    elif action == 'config':
        json_response, status = actions.config(data)
    elif action == 'calibrate':
        json_response, status = actions.calibrate(data)
    else:
        return actions.invalid_response(
            'Invalid request: action {}'.format(action))

    response = json.dumps(json_response)
    return make_response(response, status)


@application.route('/calibration/')
def calibration():
    return render_page('calibration.html')


@application.route('/data/')
def data():
    error = []
    files = []
    try:
        for filename in os.listdir(ROCKETLOGGER_DATA_DIR):
            file_path = os.path.join(ROCKETLOGGER_DATA_DIR, filename)
            if not os.path.isfile(file_path):
                continue
            try:
                mtime = os.path.getmtime(file_path)
            except OSError:
                file_modified = datetime.fromtimestamp(0)
            file_modified = datetime.fromtimestamp(mtime)
            try:
                size = os.path.getsize(file_path)
            except OSError:
                size = 0
            file_size = int(size)
            file_dict = {
                'name': filename,
                'modified': file_modified.strftime('%Y-%m-%d %H:%M:%S'),
                'size': file_size,
                'filename': file_path,
                'href_delete': url_for('data_delete', filename=filename),
                'href_download': url_for('data_download', filename=filename)
            }
            files.append(file_dict)
    except FileNotFoundError:
        error.append('Configured measurement file directory does not exist. '
                     'Check your systems configuration!')

    return render_page('data.html', files=files, error_messages=error)


@application.route('/data/delete/<filename>')
def data_delete(filename):
    filename_abs = os.path.join(ROCKETLOGGER_DATA_DIR, filename)
    if os.path.isfile(filename_abs):
        os.remove(filename_abs)
        return 'Deleted file {}.'.format(filename)
    else:
        return 'File {} does not exist!'.format(filename)


@application.route('/data/download/<filename>')
def data_download(filename):
    filename_abs = os.path.join(ROCKETLOGGER_DATA_DIR, filename)
    if os.path.isfile(filename_abs):
        return 'download link to {}'.format(filename)


# @sockets.route('/ws/')
# def stream(ws):
#     socket = context.socket(zmq.SUB)
#     socket.connect('tcp://localhost:{PORT}'.format(PORT=ZMQ_LISTENING_PORT))
#     socket.setsockopt(zmq.SUBSCRIBE, "")
#     gevent.sleep()
#     while True:
#         data = socket.recv_json()
#         logger.info(data)
#         ws.send(json.dumps(data))
#         gevent.sleep()


@application.route('/log/')
def log():
    return 'Hello log!'


@application.route('/robots.txt')
@application.route('/sitemap.xml')
def static_from_root():
    return send_from_directory(application.static_folder, request.path[1:])


if __name__ == "__main__":
    from gevent import pywsgi
    from geventwebsocket.handler import WebSocketHandler
    server = pywsgi.WSGIServer(('', 5000), app, handler_class=WebSocketHandler)
    server.serve_forever()
    # application.run()
