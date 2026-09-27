exec(open('tools/fc4_feature_probe.py').read().replace('r.close()',''))
o=0x1f6a34b51c0
for text in ['CAnimalAgentFC3','CAnimalAgent','CFCXCountersComponentAnimal','CFCXCountersComponent']:
 crc=zlib.crc32(text.encode());c=comp(o,crc);print(text,hex(crc),hex(c),rd(q(c+0x40)+0x18,4).hex())
g=comp(o,0x035982c6);buf=q(g+0xb0);cnt=ui(g+0xb8);print('graphic',hex(g),'bones',cnt)
if cnt<2048:
 raw=rd(buf,cnt*0x70);Path('build-Release/fc4-research/bird-bones.bin').write_bytes(raw)
 print([(hex(struct.unpack_from('<I',raw,i*0x70)[0]),struct.unpack_from('<3f',raw,i*0x70+0x60)) for i in range(cnt)])
print('look +680',struct.unpack('<6f',rd(state+0x680,24)),'desired +480',struct.unpack('<6f',rd(state+0x480,24)),'eye',struct.unpack('<3f',rd(state+0x1d0,12)))
r.close()
