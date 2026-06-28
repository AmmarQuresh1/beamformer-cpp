import numpy as np 
import matplotlib.pyplot as plt 
import pyroomacoustics as pra 
from scipy.io import wavfile
import os
import scipy

fs, audio = wavfile.read('data/arctic_b0540.wav') # returns rate, data 
fs2, audio2 = wavfile.read('data/airplane_noise.wav')
n = round(len(audio2) * fs / fs2)          # preserve duration: 24k -> 16k
audio2 = scipy.signal.resample(audio2, n)
audio2 = audio2.mean(axis=1)
room_dim = [10, 10, 10] # metres


rt60 = 0.5
e_absorption, max_order = pra.inverse_sabine(rt60, room_dim)

room = pra.ShoeBox(room_dim, fs=fs, materials=pra.Material(e_absorption), max_order=max_order, use_rand_ism=True, air_absorption=True)

room.add_source([5,5,5], signal=audio)
room.add_source([5,9,2], signal=audio2)

mic_locs = np.c_[
    [2, 2, 2], # mic 1
    [2, 2.05, 2], # mic 2
    [2, 2.10, 2], # mic 3
    [2, 2.15, 2], # mic 4
]

room.add_microphone_array(mic_locs)

room.simulate()

audio_reverb = room.mic_array.to_wav('data/sim.wav', norm=True, bitdepth=np.int16)

rt60_m = room.measure_rt60()


data_file = open("data/info_sim.wav.txt", "w")
data_file.write("The desired RT60 was {}".format(rt60))
data_file.write("\nThe measured RT60 is {}".format(rt60_m[1, 0]))
data_file.write(f"\nThe frequency is {fs}")
data_file.close()

