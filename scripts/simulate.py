import numpy as np 
import matplotlib.pyplot as plt 
import pyroomacoustics as pra 
from scipy.io import wavfile
import os

fs, audio = wavfile.read('data/arctic_b0540.wav') # returns rate, data 
room_dim = [10, 10, 10] # metres

rt60 = 0.5
e_absorption, max_order = pra.inverse_sabine(rt60, room_dim)

room = pra.ShoeBox(room_dim, fs=fs, materials=pra.Material(e_absorption), max_order=max_order, use_rand_ism=True, air_absoption=True)

room.add_source([5,5,5], signal=audio)

mic_locs = np.c_[
    [2, 2, 2], # mic 1
    [2, 2.05, 2], # mic 2
    [2, 2.10, 2], # mic 3
    [2, 2.15, 2], # mic 4
]

room.add_microphone_array(mic_locs)

room.simulate()

audio_reverb = room.mic_array.to_wav('data/sim_arctic_b0540.wav', norm=True, bitdepth=np.int16)
