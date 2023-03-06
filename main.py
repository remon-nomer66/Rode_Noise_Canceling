import sounddevice as sd
import numpy as np

device_list = sd.query_devices()
print(device_list)

def callback(indata, frames, time, status):
    print(indata)

while True:
    duration = 10
    with sd.InputStream(
            channels=1,
            dtype='float32',
            callback=callback
        ):
        sd.sleep(int(duration * 1000))