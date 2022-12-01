import zlib
import struct

def deflate(data):
    obj = zlib.compressobj(
        0,
        zlib.DEFLATED,
        -zlib.MAX_WBITS,
        zlib.DEF_MEM_LEVEL,
        0)
    return obj.compress(data) + obj.flush()

input_path = '/tmp/decoded.dump'
idx_path = '/tmp/test.idx'
bof_path = '/tmp/test.bof'

block_size = 8192

input = open(input_path, 'rb').read()
input_len = len(input)

bof = bytearray()
idx = bytearray()

pos = 0
while True:
    block = input[pos:pos+block_size]
    if len(block) == 0:
        break
    pos += block_size
    idx += struct.pack('<I', len(bof))
    bof += deflate(block)

idx += struct.pack('<I', len(bof))
idx += struct.pack('<I', len(bof))
idx += struct.pack('<I', input_len)

open(idx_path, 'wb').write(idx)
open(bof_path, 'wb').write(bof)
