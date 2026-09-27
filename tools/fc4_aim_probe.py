exec(open('tools/fc4_feature_probe.py').read().replace('r.close()',''))
import math
f=lambda p,n:struct.unpack('<'+'f'*n,rd(p,4*n))
print('pose quat',f(state+0x1c0,4),'look base',f(state+0x680,3),'look world',f(state+0x68c,3),'look recoil',f(state+0x6ec,3),'muzzle offset',f(state+0x6ac,3))
x,y,z,wq=f(state+0x1c0,4);print('quat forward',(2*(x*y-z*wq),1-2*(x*x+z*z),2*(y*z+x*wq)))
man=q(r.base+0x2e44a88);cam=q(q(q(man+0x138)+8)+0x60);m=f(cam+0x170,16)
print('camera',hex(cam),'forward',(m[3],m[7],m[11]),'matrix',m)
x,y,z=f(state+0x68c,3);print('look forward',(-math.sin(z)*math.cos(x)+math.cos(z)*math.sin(y)*math.sin(x),math.cos(z)*math.cos(x)+math.sin(z)*math.sin(y)*math.sin(x),math.cos(y)*math.sin(x)))
r.close()
