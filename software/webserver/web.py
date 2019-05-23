import os
from datetime import datetime, timezone
from socket import gethostname
from subprocess import check_output
from flask import Flask, render_template, request, send_from_directory, url_for


ROCKETLOGGER_DATA_DIR = '/var/www/rocketlogger/data'
ROCKETLOGGER_LOG_FILE = '/var/www/rocketlogger/log/rocketlogger.log'
WEB_VERSION = 1.99

app = Flask(__name__)


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
                              'Check your systems configuration!')
    except:
        version_string = None
        error_messages.append('Failed getting RocketLogger binary version. '
                              'Check your systems configuration!')
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


@app.route('/')
def home():
    return render_page('base.html')


@app.route('/control/')
def control():
    return render_page('control.html')


@app.route('/calibration/')
def calibration():
    return render_page('calibration.html')


@app.route('/data/')
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


@app.route('/data/delete/<filename>')
def data_delete(filename):
    filename_abs = os.path.join(ROCKETLOGGER_DATA_DIR, filename)
    if os.path.isfile(filename_abs):
        os.remove(filename_abs)
        return 'Deleted file {}.'.format(filename)
    else:
        return 'File {} does not exist!'.format(filename)


@app.route('/data/download/<filename>')
def data_download(filename):
    filename_abs = os.path.join(ROCKETLOGGER_DATA_DIR, filename)
    if os.path.isfile(filename_abs):
        return 'download link to {}'.format(filename)


@app.route('/log/')
def log():
    return 'Hello log!'


@app.route('/robots.txt')
@app.route('/sitemap.xml')
def static_from_root():
    return send_from_directory(app.static_folder, request.path[1:])
