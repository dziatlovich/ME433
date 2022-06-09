import board
import time
import digitalio
import analogio
from ulab import numpy as np # to get access to ulab numpy functions

sin = np.zeros((1024,1))
fft = np.zeros((1024,1))

for i in range(1024):
    sin[i] = np.sin(i)+np.sin(2*i)+np.sin(3*i)
    fft[i] = np.fft.fft(sin[i])
    print((fft[i,0],))
    time.sleep(0.1)
