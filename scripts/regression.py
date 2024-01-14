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
import zipfile
import shutil

parser = argparse.ArgumentParser()
parser.add_argument('--tmp-dir')
parser.add_argument('--lsd-dir')
parser.add_argument('--lsa-dir')
parser.add_argument('--duden-dir')
parser.add_argument('--lsd2dsl-path', required=True)
parser.add_argument('--report-path', required=True)
args = parser.parse_args()

class Reporter:
    def __init__(self):
        self._props = []
        self._outputs = []
        self._time = ''
        self._html = ['<table style="width:100%">']

    def md5hash_file(self, path):
        block = 2 << 20
        md5 = hashlib.md5()
        with open(path, 'rb') as f:
            while True:
                buf = f.read(block)
                if len(buf) == 0:
                    break
                md5.update(buf)
        return md5.hexdigest()

    def md5hash_content(self, content):
        md5 = hashlib.md5()
        md5.update(content)
        return md5.hexdigest()

    def finish_dict(self, path, result):
        self._html.append('<tr>')

        colors = {
            0: '#90ee90',
            1: '#ee9090',
            2: 'yellow'
        }
        self._html.append(f'<td style="background:{colors[result]}">')
        self._html.append(f'<b>{path}</b><br>')
        self._html.append('</td>')

        self._html.append('<td>')
        for name, value in sorted(self._props):
            self._html.append(f'{name}: {value}<br>\n')
        self._html.append('</td>')

        self._html.append('<td>')
        for path, size, h in sorted(self._outputs):
            self._html.append(f'{path} <b>{size} ({size//(1<<20)} MB)</b> {h}<br>\n')
        self._html.append('</td>')

        self._html.append('</tr>')
        self._props = []
        self._outputs = []

    def add_prop(self, name, value):
        self._props.append((name, value))

    def add_outputs(self, path):
        for root, dirnames, filenames in os.walk(path):
            for f in filenames:
                fullpath = os.path.join(root, f)
                if f.endswith('.zip'):
                    with zipfile.ZipFile(fullpath) as zip:
                        for name in sorted(zip.namelist()):
                            content = zip.read(name)
                            if re.match(r'rendered_table_\d{4}\.(png|bmp)', name):
                                md5 = 'table'
                                filesize = 0
                            else:
                                md5 = self.md5hash_content(content)
                                filesize = len(content)
                            self._outputs.append((os.path.join(f, name), filesize, md5))
                else:
                    self._outputs.append((f, os.path.getsize(fullpath), self.md5hash_file(fullpath)))

    def html(self):
        self._html.append("</table>")
        root = lxml.html.fromstring(''.join(self._html))
        return lxml.etree.tostring(root, encoding='unicode', pretty_print=True)

    def savetofile(self, path):
        with open(path, 'w', encoding="utf-8") as report:
            report.write(self.html())

def clear_dir(path):
    if os.path.exists(path):
        shutil.rmtree(path)
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
        clear_dir(args.tmp_dir)
        code, elapsed, props = parser(args.lsd2dsl_path, args.tmp_dir, f)
        errors = 0
        for name, value in props:
            if name == 'errors':
                errors = int(value)
            reporter.add_prop(name, value)
        reporter.add_outputs(args.tmp_dir)
        result = 1 if code == 1 or errors > 20 else 2 if errors > 0 else 0
        reporter.finish_dict(f, result)

if not args.lsd2dsl_path:
    print(parser.print_help())
    exit(0)

reporter = Reporter()
if args.duden_dir:
    process_dicts(reporter, '.inf', args.duden_dir, convert_inf)
reporter.savetofile(f'{args.report_path}.duden.html')

reporter = Reporter()
if args.lsd_dir:
    process_dicts(reporter, '.lsd', args.lsd_dir, convert_lsd)
reporter.savetofile(f'{args.report_path}.lsd.html')

reporter = Reporter()
if args.lsa_dir:
    process_dicts(reporter, '.lsa', args.lsa_dir, convert_lsa)
reporter.savetofile(f'{args.report_path}.lsa.html')
