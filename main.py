import sounddevice as sd
import numpy as np

# サンプリング周波数を設定
fs = 44100  # サンプリング周波数

def callback(indata, outdata, frames, time, status):
    if status:
        print(status, file=sys.stderr)
    # 入力音声を逆相に反転
    reverse = -1 * np.ones_like(indata)
    # 反転音声を出力
    outdata[:] = reverse * indata

# マイク入力音声をリアルタイムで反転再生
with sd.Stream(channels=1, blocksize=1024, samplerate=fs, callback=callback):
    print("録音開始")
    while True:
        response = input("録音を終了しますか？ y/n: ")
        if response == 'y':
            break