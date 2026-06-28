#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <dr_wav.h>

class WavWriter {
private:
    drwav wav;
    float* samples;
public:
    WavWriter(float* pSamples, const char* filename) {
        samples = pSamples;
        drwav_data_format format;
        format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT;          // <-- Any of the DR_WAVE_FORMAT_* codes.
        format.channels = 1;
        format.sampleRate = 16000;
        format.bitsPerSample = 32;
        if (drwav_init_file_write(&wav, filename, &format, NULL) == false) {
            std::cout<<"Failed to write";
        }
    } 
    drwav_uint64 writeFrames(drwav_uint64 framesCount) {
        return drwav_write_pcm_frames(&wav, framesCount, samples);
    }
    ~WavWriter() {
        drwav_uninit(&wav);
    }
};

class WavReader {
private:
    drwav wav;
    const char* wav_f;
    std::vector<float> decodedInterleavedPCMSamples{};
    drwav_uint64 numberOfFramesDecoded;
public: 
    WavReader(const char* wavfile) : wav_f{wavfile} {
        if (!drwav_init_file(&wav, wav_f, NULL)) {
            throw std::invalid_argument("File cannot be found.");
        }

        decodedInterleavedPCMSamples = std::vector<float> (wav.totalPCMFrameCount * wav.channels);
        numberOfFramesDecoded = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, decodedInterleavedPCMSamples.data());
    }

    ~WavReader() {
        drwav_uninit(&wav);
    }

    const std::vector<float>& getDecodedInterleavedPCMSamples() const { return decodedInterleavedPCMSamples; }
    size_t getFramesCount() const { return numberOfFramesDecoded; }
    drwav_uint16 getWavChannels() const { return wav.channels; }
};

const std::vector<std::vector<float>> deInterleave(const std::vector<float>& interleavedPCMSamples, const size_t numFrames, const int numChannels){
    std::vector<std::vector<float>> deInterleavedSamples(numChannels, std::vector<float>(numFrames));

    for (size_t i = 0; i < numFrames; ++i) {
        for (int j = 0; j < numChannels; ++j) {
            deInterleavedSamples[j][i] = interleavedPCMSamples[i * numChannels + j];
        }
    }

    return deInterleavedSamples;
}

int main(int argc, char** argv) {
    /*
    artic_b0540.wav:
    16000Hz, 1 sample = 1/16000 seconds
    0.01513119534 seconds to reach microphone (~5.19m at 343m/s) , first ~242 frames will be near zero (0.0151... * 16000 = 242) 
    75422 frames = 4.7~ seconds of audio
    */
    if (argc < 2) { std::cout<<"Provide wav file path\n"; return -1; }

    const char* path = argv[1];
    
    WavReader reader(path);

    const std::vector<float>& decodedInterleavedPCMSamples = reader.getDecodedInterleavedPCMSamples();
    const std::vector<std::vector<float>> decodedDeInterleavedPCMSamples = deInterleave(decodedInterleavedPCMSamples, reader.getFramesCount(), reader.getWavChannels());

    int channels = reader.getWavChannels();
    
    /*
    Based on mic and source setup in simulate.py:
    source(x,y,z): [5,5,5], speed of sound 343 m/s, fs = 16000 Hz (16 samples/ms)
    Reference = mic 4 (nearest -> earliest arrival -> delay 0).

    mic    pos            dist     TOA        TDOA vs mic4   samples (xfs)
    # mic 1: [2, 2,    2]  5.196 m  15.149 ms  0.248 ms        4
    # mic 2: [2, 2.05, 2]  5.167 m  15.066 ms  0.165 ms        3
    # mic 3: [2, 2.10, 2]  5.139 m  14.983 ms  0.082 ms        1
    # mic 4: [2, 2.15, 2]  5.111 m  14.901 ms  0.000 ms        0
    */

    int mic1_delay = 4, mic2_delay = 3, mic3_delay = 1;
    size_t num_frames = reader.getFramesCount();
    
    std::vector<float> correctedMic1(reader.getFramesCount());
    for (size_t i = mic1_delay; i < std::size(correctedMic1); ++i) { correctedMic1[i-mic1_delay] = decodedDeInterleavedPCMSamples[0][i]; }
    
    std::vector<float> correctedMic2(reader.getFramesCount());
    for (size_t i = mic2_delay; i < std::size(correctedMic2); ++i) { correctedMic2[i-mic2_delay] = decodedDeInterleavedPCMSamples[1][i]; }
    
    std::vector<float> correctedMic3(reader.getFramesCount());
    for (size_t i = mic3_delay; i < std::size(correctedMic3); ++i){ correctedMic3[i-mic3_delay] = decodedDeInterleavedPCMSamples[2][i]; }
    
    
    std::vector<float> beamformed(num_frames);
    for (size_t i = 0; i < std::size(beamformed); ++i) {
        beamformed[i] = (correctedMic1[i] + correctedMic2[i] + correctedMic3[i] + decodedDeInterleavedPCMSamples[3][i]) / channels;
    }
    WavWriter writer(beamformed.data(), "../data/delay_and_sum_sim.wav"); 
    writer.writeFrames(reader.getFramesCount());
    
    std::cout<<"beamformed\n";
    for (size_t i = 1000; i < 1010; ++i) {
        std::cout<<beamformed[i]<<" ";
    }
    std::cout<<"\nnon-beamformed\n";
    for (size_t i = 1000; i < 1010; ++i) {
        std::cout<<decodedInterleavedPCMSamples[i*channels];
    }
    std::cout<<"\n";

    auto rms = [](const std::vector<float>& v) {
        double acc = 0;
        for (float x : v) acc += double(x) * x;   // square and accumulate
        return std::sqrt(acc / v.size());          // mean, then root
    };

    std::cout<<"beamformed RMS: "<<rms(beamformed)<<'\n';
    std::cout<<"raw RMS: "<<rms(decodedDeInterleavedPCMSamples[3])<<'\n';

    // aligned sum, no averaging
    std::vector<float> alignedSum(num_frames), rawSum(num_frames);
    for (size_t i = 0; i < num_frames; ++i) {
        alignedSum[i] = correctedMic1[i] + correctedMic2[i] + correctedMic3[i] + decodedDeInterleavedPCMSamples[3][i];
        rawSum[i]     = decodedDeInterleavedPCMSamples[0][i] + decodedDeInterleavedPCMSamples[1][i]
                    + decodedDeInterleavedPCMSamples[2][i] + decodedDeInterleavedPCMSamples[3][i];
    }
    std::cout << "aligned sum RMS: " << rms(alignedSum) << '\n';
    std::cout << "raw sum RMS:     " << rms(rawSum) << '\n';

    return 0;
}