#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <dr_wav.h>

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

const std::vector<std::vector<float>> deInterleave(const std::vector<float>& interleavedPCMSamples, size_t numFrames, int numChannels){
    std::vector<std::vector<float>> deInterleavedPCMSamples(numChannels, std::vector<float>(numFrames));

    for (size_t i = 0; i < numFrames; ++i) {
        for (int j = 0; j < numChannels; ++j) {
            deInterleavedPCMSamples[j][i] = interleavedPCMSamples[i * numChannels + j];
        }
    }

    return deInterleavedPCMSamples;
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
    for (int mic = 0; mic < channels; ++mic) {
        std::cout << "mic " << mic << ": ";
        for (int t = 1000; t < 1010; ++t) {
            std::cout << decodedDeInterleavedPCMSamples[mic][t] << ' ';
        }
        std::cout << '\n';
    }
    
    /*
    
    */

    return 0;

}