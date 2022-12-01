#!/usr/bin/python3

import os
import subprocess
import filecmp
import time
import argparse
import hashlib
import lxml.html
import lxml.etree
import re

parser = argparse.ArgumentParser()
parser.add_argument('--lsd-dir', dest='lsd_dir')
parser.add_argument('--lsa-dir', dest='lsa_dir')
parser.add_argument('--duden-dir', dest='duden_dir')
parser.add_argument('--lsd2dsl-path', dest='lsd2dsl_path', required=True)
parser.add_argument('--report-path', dest='report_path', required=True)
parser.add_argument('--ignore-zip-hash', dest='ignore_zip_hash', action='store_true')
args = parser.parse_args()

tmp_dir = '/tmp/lsd2dsl-regression'

class Reporter:
    _props = []
    _outputs = []
    _time = ''
    _html = '<table style="width:100%">'

    def md5hash(self, path):
        block = 2 << 20
        md5 = hashlib.md5()
        with open(path, 'rb') as f:
            while True:
                buf = f.read(block)
                if len(buf) == 0:
                    break
                md5.update(buf)
        return md5.hexdigest()

    def finish_dict(self, path, result):
        self._html += '<tr>'

        colors = {
            0: '#90ee90',
            1: '#ee9090',
            2: 'yellow'
        }
        self._html += f'<td style="background:{colors[result]}">'
        self._html += f'<b>{path}</b><br>'
        self._html += '</td>'

        self._html += '<td>'
        for name, value in sorted(self._props):
            self._html += f'{name}: {value}<br>\n'
        self._html += '</td>'

        self._html += '<td>'
        for path, size, h in sorted(self._outputs):
            self._html += f'{path} <b>{size} ({size//(1<<20)} MB)</b> {h}<br>\n'
        self._html += '</td>'

        self._html += '</tr>'
        self._props = []
        self._outputs = []

    def add_prop(self, name, value):
        self._props.append((name, value))

    def add_outputs(self, path):
        for root, dirnames, filenames in os.walk(path):
            for f in filenames:
                if args.ignore_zip_hash and f.endswith('.zip'):
                    md5 = ''
                else:
                    md5 = self.md5hash(f'{root}/{f}')
                self._outputs.append((f, os.path.getsize(f'{root}/{f}'), md5))

    def html(self):
        self._html += "</table>"
        root = lxml.html.fromstring(self._html)
        return lxml.etree.tostring(root, encoding='unicode', pretty_print=True)

def clear_dir(path):
    if os.path.exists(path):
        subprocess.check_call(['rm', '-rf', path])
    os.makedirs(path)

def get_files(path, ext):
    matches = []
    for root, dirnames, filenames in os.walk(path):
        for filename in filenames:
            if os.path.splitext(filename)[1].lower() == ext.lower():
                matches.append(os.path.join(root, filename))
    return matches

def convert_lsd(exe, path, lsd):
    past = time.time()
    try:
        output = subprocess.check_output([exe, '--lsd', lsd, '--out', path]).decode()
    except:
        return (1, time.time() - past, [])
    props = [('version', re.compile('Version:\\s+(\\d+)').search(output).group(1)),
             ('headings', re.compile('Headings:\\s+(\\d+)').search(output).group(1)),
             ('source', re.compile('Source:\\s+(.*?)').search(output).group(1)),
             ('target', re.compile('Target:\\s+(.*?)').search(output).group(1)),
             ('name', re.compile('Name:\\s+(.*?)').search(output).group(1))]
    return (0, time.time() - past, props)

def convert_lsa(exe, path, lsd):
    past = time.time()
    try:
        output = subprocess.check_output([exe, '--lsa', lsd, '--out', path])
    except:
        return (1, time.time() - past, [])
    return (0, time.time() - past, [])

def convert_inf(exe, path, inf):
    past = time.time()
    try:
        output = subprocess.check_output([exe, '--duden', inf, '--out', path]).decode('utf8')
    except:
        return (1, time.time() - past, [])
    rx = re.compile('done converting: (\\d+) articles \\((\\d+) errors\\), (\\d+) tables, (\\d+) resources, (\\d+) audio files')
    m = rx.search(output)

    mVersion = re.compile('Version:\\s+(\\d+) \\(HIC (\\d+)\\)').search(output)

    props = [('articles', m.group(1)),
             ('errors', m.group(2)),
             ('tables', m.group(3)),
             ('resources', m.group(4)),
             ('audio', m.group(5)),
             ('version', mVersion.group(1)),
             ('hic version', mVersion.group(2))]
    return (0, time.time() - past, props)

def lsdpath_to_dslpath(lsdpath, temp):
    (_, filename) = os.path.split(lsdpath)
    (name, _) = os.path.splitext(filename)
    return temp + '/' + name + '.dsl'

def process_dicts(reporter, extension, path, parser):
    for i, f in enumerate(get_files(path, extension)):
        print(f'{i}: {f}')
        clear_dir(tmp_dir)
        code, elapsed, props = parser(args.lsd2dsl_path, tmp_dir, f)
        errors = 0
        for name, value in props:
            if name == 'errors':
                errors = int(value)
            reporter.add_prop(name, value)
        reporter.add_outputs(tmp_dir)
        result = 1 if code == 1 or errors > 20 else 2 if errors > 0 else 0
        reporter.finish_dict(f, result)

if not args.lsd2dsl_path:
    print(parser.print_help())
    exit(0)

reporter = Reporter()

if args.duden_dir:
    process_dicts(reporter, '.inf', args.duden_dir, convert_inf)

if args.lsd_dir:
    process_dicts(reporter, '.lsd', args.lsd_dir, convert_lsd)

if args.lsa_dir:
    process_dicts(reporter, '.lsa', args.lsa_dir, convert_lsa)

with open(args.report_path, 'w') as report:
    report.write(reporter.html())
