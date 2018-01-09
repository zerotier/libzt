import libzt
import time

print('joining virtual network...')
libzt.zts_startjoin('whatev_config', 0x1234567890ABCDEF)
print('fd = ' + str(libzt.zts_socket(1,2,3)))

print('looping forever, ping me')
while True:
    time.sleep(1)