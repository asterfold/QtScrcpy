"""Exercise the actual async connector against a controlled ADB process."""
import json
import os
import shlex
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
qt = Path('/opt/homebrew/opt/qt@5')
core = root / 'QtScrcpy/QtScrcpyCore'
flags = (root / 'build/QtScrcpy/CMakeFiles/QtScrcpy.dir/flags.make').read_text()
includes = shlex.split(next(line.split(' = ', 1)[1] for line in flags.splitlines()
                           if line.startswith('CXX_INCLUDES = ')))
fake_source = '''#!/usr/bin/python3
import json, os, sys
from pathlib import Path
args = sys.argv[1:]
with open(os.environ['WIFI_TRACE'], 'a') as f: f.write(json.dumps(args) + '\\n')
mode = os.environ['WIFI_MODE']
cmd = args[2:] if args[:1] == ['-s'] else args
if cmd == ['devices', '-l']:
    print('List of devices attached')
    if mode != 'no_usb': print('PHONE device usb:1 model:Test')
    print('192.168.1.2:39999 offline')
elif cmd[:2] == ['shell', 'ip']:
    print('1: wlan0 inet 192.168.1.2/24')
elif cmd == ['mdns', 'services']:
    if mode != 'legacy':
        trace = Path(os.environ['WIFI_TRACE']).read_text()
        port = '45000' if mode == 'changed' and '"connect"' in trace else '40001'
        print('adb-PHONE-random (2) _adb-tls-connect._tcp 192.168.1.2:' + port)
        print('adb-OTHER-random _adb-tls-connect._tcp. 192.168.1.3:40002')
elif cmd[:1] == ['connect']:
    if mode == 'changed' and cmd[1].endswith(':40001'): print('failed to connect: Connection refused')
    else: print('connected to ' + cmd[1])
elif cmd == ['get-state']: print('offline' if mode == 'offline' else 'device')
elif cmd == ['shell', 'getprop', 'ro.serialno']: print('OTHER' if mode == 'wrong_phone' else 'PHONE')
elif cmd == ['tcpip', '5555']: print('restarting in TCP mode port: 5555')
else: sys.exit(20)
'''

with tempfile.TemporaryDirectory(prefix='qtscrcpy-wifi-test-') as folder:
    folder = Path(folder)
    fake = folder / 'adb'
    fake.write_text(fake_source)
    fake.chmod(0o755)
    binary = folder / 'wifi_tests'
    sources = [str(core / 'src/adb/adbprocess.cpp'), str(core / 'src/adb/adbprocessimpl.cpp')]
    for header in [core / 'include/adbprocess.h', core / 'src/adb/adbprocessimpl.h']:
        moc = folder / ('moc_' + header.stem + '.cpp')
        subprocess.run([str(qt / 'bin/moc'), '-I' + str(core / 'include'), '-f' + str(header), str(header), '-o', str(moc)], check=True)
        sources.append(str(moc))
    subprocess.run(['/usr/bin/c++', '-std=c++11', '-fPIC', *includes, '-I' + str(core / 'include'),
                    '-F' + str(qt / 'lib'), str(Path(__file__).with_name('wifi_connection_tests.cpp')),
                    *sources, '-framework', 'QtCore', '-o', str(binary)], check=True)
    for mode, expected in [('changed', '192.168.1.2:45000'), ('legacy', '192.168.1.2:5555'),
                           ('offline', 'error'), ('wrong_phone', 'error'), ('no_usb', '192.168.1.2:40001')]:
        trace = folder / (mode + '.jsonl')
        env = {**os.environ, 'QTSCRCPY_ADB_PATH': str(fake), 'WIFI_MODE': mode,
               'WIFI_TRACE': str(trace), 'WIFI_EXPECT': expected,
               'WIFI_PREFERRED': '192.168.1.2:39999' if mode == 'no_usb' else 'PHONE'}
        result = subprocess.run([str(binary)], env=env, capture_output=True, text=True, timeout=15)
        if result.returncode:
            raise RuntimeError(mode + ': ' + result.stderr)
        calls = [json.loads(line) for line in trace.read_text().splitlines()]
        assert ['connect', '192.168.1.2:39999'] not in calls
        assert ['connect', '192.168.1.3:40002'] not in calls
        if mode != 'legacy': assert ['-s', 'PHONE', 'tcpip', '5555'] not in calls
        print('PASS:', mode)
