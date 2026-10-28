"""Exercise the production decoder with a native L2TB packet and corrupt variants.
Usage: python tests/telemetry_binary_contract_test.py native.l2t
No game or database mutations. Build StatsIngest Release first.
"""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DLL = ROOT / 'integrations/StatsIngest/bin/Release/net10.0/StatsIngest.dll'
source = Path(sys.argv[1]).read_bytes()
header_size, cold_size, count, vehicles = struct.unpack_from('<4i', source, 8)
assert count >= 2
start = 24 + header_size + cold_size
cases = {'valid': source}
for end in (0, 4, 23, 24, len(source) - 1):
    cases[f'truncated-{end}'] = source[:end]
for name, offset, value in [('magic', 0, 0), ('version', 4, 2), ('negative-header', 8, -1), ('huge-header', 8, 2147483647), ('negative-cold', 12, -1), ('huge-count', 16, 2147483647), ('negative-vehicles', 20, -1), ('entity', start + 4, 0), ('flags', start + 8, 0x20000), ('stance', start + 8, 3), ('duplicate-sequence', start + 16, struct.unpack_from('<i', source, start)[0]), ('sequence-hole', start, 999999), ('nan', start + count * 16, 0x7fc00000)]:
    data = bytearray(source)
    struct.pack_into('<i', data, offset, value)
    cases[name] = data
cases['trailing-byte'] = source + b'\0'
data = bytearray(source)
data[24] = 0xff
cases['invalid-utf8'] = data
with tempfile.TemporaryDirectory(prefix='Lobby2BinaryContract-') as temp:
    folder = Path(temp)
    for name, data in cases.items():
        packet = folder / (name + '.l2t')
        packet.write_bytes(data)
        output = folder / (name + '.json')
        result = subprocess.run(['dotnet', str(DLL), '--decode-telemetry', str(packet), str(output)], capture_output=True, text=True)
        assert (result.returncode == 0) == (name == 'valid'), name + ': ' + result.stdout + result.stderr
        assert output.exists() == (name == 'valid'), name
        if name == 'valid':
            decoded = json.loads(output.read_text())
            assert sum(r['type'] == 'state' for r in decoded['records']) == count
print(json.dumps({'validNativePackets': 1, 'malformedPacketsRejected': len(cases) - 1}))
