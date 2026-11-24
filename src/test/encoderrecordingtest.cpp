#include <gtest/gtest.h>
#include <sndfile.h>

#include <QSemaphore>
#include <QTemporaryFile>
#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "audio/types.h"
#include "encoder/encoder.h"
#include "encoder/encodercallback.h"
#include "engine/sidechain/enginesidechain.h"
#include "engine/sidechain/sidechainworker.h"
#include "recording/defs_recording.h"
#include "test/mixxxtest.h"
#include "util/defs.h"

namespace {

class FileCallback : public EncoderCallback {
  public:
    explicit FileCallback(QTemporaryFile* pFile)
            : m_pFile(pFile) {
    }

    void write(const unsigned char* header,
            const unsigned char* body,
            int headerLen,
            int bodyLen) override {
        if (!m_pFile->isOpen()) {
            return;
        }
        if (headerLen > 0) {
            m_pFile->write(reinterpret_cast<const char*>(header), headerLen);
        }
        if (bodyLen > 0) {
            m_pFile->write(reinterpret_cast<const char*>(body), bodyLen);
        }
    }

    std::int64_t tell() override {
        return m_pFile->isOpen() ? m_pFile->pos() : -1;
    }
    void seek(std::int64_t pos) override {
        if (m_pFile->isOpen()) {
            m_pFile->seek(pos);
        }
    }
    std::int64_t filelen() override {
        return m_pFile->isOpen() ? m_pFile->size() : 0;
    }

  private:
    QTemporaryFile* m_pFile;
};

class EncoderRecordingTest : public MixxxTest,
                             public testing::WithParamInterface<const char*> {
};

TEST_P(EncoderRecordingTest, RoundTripsSurroundChannels) {
    m_pConfig->setValue(ConfigKey(RECORDING_PREF_KEY, "channel_mode"),
            mixxx::recording::kSurround51);
    auto format = EncoderFactory::getFactory().getFormatFor(QString::fromLatin1(GetParam()));
    ASSERT_TRUE(format.supportsSurround51);

    QTemporaryFile file;
    FileCallback callback(&file);
    auto encoder = EncoderFactory::getFactory().createRecordingEncoder(
            format, m_pConfig, &callback);
    ASSERT_TRUE(encoder);
    QString error;
    ASSERT_EQ(0, encoder->initEncoder(mixxx::audio::SampleRate(48000), &error))
            << error.toStdString();
    ASSERT_TRUE(file.open());

    constexpr int kFrames = 3000;
    const std::array<float, 6> channels = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
    std::vector<CSAMPLE> input(kFrames * channels.size());
    for (int frame = 0; frame < kFrames; ++frame) {
        for (std::size_t channel = 0; channel < channels.size(); ++channel) {
            input[frame * channels.size() + channel] = channels[channel];
        }
    }
    encoder->encodeBuffer(input.data(), input.size());
    encoder->flush();
    encoder.reset();
    file.flush();
    ASSERT_TRUE(file.seek(0));
    SF_INFO info{};
    SNDFILE* pSoundFile = sf_open_fd(file.handle(), SFM_READ, &info, SF_FALSE);
    ASSERT_NE(nullptr, pSoundFile) << sf_strerror(nullptr);
    EXPECT_EQ(6, info.channels);
    EXPECT_EQ(kFrames, info.frames);
    if (QString::fromLatin1(GetParam()) == ENCODING_WAVE) {
        std::array<int, 6> channelMap{};
        ASSERT_EQ(SF_TRUE,
                sf_command(pSoundFile,
                        SFC_GET_CHANNEL_MAP_INFO,
                        channelMap.data(),
                        sizeof(channelMap)));
        const std::array<int, 6> expectedMap = {
                SF_CHANNEL_MAP_LEFT,
                SF_CHANNEL_MAP_RIGHT,
                SF_CHANNEL_MAP_CENTER,
                SF_CHANNEL_MAP_LFE,
                SF_CHANNEL_MAP_REAR_LEFT,
                SF_CHANNEL_MAP_REAR_RIGHT};
        EXPECT_EQ(expectedMap, channelMap);
    }
    std::vector<float> output(input.size());
    EXPECT_EQ(kFrames, sf_readf_float(pSoundFile, output.data(), kFrames));
    EXPECT_EQ(0, sf_close(pSoundFile));
    for (std::size_t i = 0; i < input.size(); ++i) {
        EXPECT_NEAR(input[i], output[i], 0.00005f) << i;
    }
}

INSTANTIATE_TEST_SUITE_P(SurroundFormats,
        EncoderRecordingTest,
        testing::Values(ENCODING_WAVE, ENCODING_FLAC));

TEST_F(EncoderRecordingTest, RejectsUnsupportedSurroundFormats) {
    m_pConfig->setValue(ConfigKey(RECORDING_PREF_KEY, "channel_mode"),
            mixxx::recording::kSurround51);
    QTemporaryFile file;
    FileCallback callback(&file);
    for (const auto& format : EncoderFactory::getFactory().getFormats()) {
        if (!format.supportsSurround51) {
            EXPECT_EQ(nullptr,
                    EncoderFactory::getFactory().createRecordingEncoder(
                            format, m_pConfig, &callback));
        }
    }
}

class CaptureSidechain : public SideChainWorker {
  public:
    explicit CaptureSidechain(QSemaphore* pReady)
            : m_pReady(pReady) {
    }

    void process(const CSAMPLE*, std::size_t) override {
    }

    void processWithSurround(const CSAMPLE* pStereo,
            const CSAMPLE* pSurround,
            std::size_t frames,
            bool) override {
        if (m_captured || frames == 0) {
            return;
        }
        std::copy_n(pStereo, m_stereo.size(), m_stereo.begin());
        std::copy_n(pSurround, m_surround.size(), m_surround.begin());
        m_captured = true;
        m_pReady->release();
    }

    void shutdown() override {
    }

    std::array<CSAMPLE, 2> m_stereo{};
    std::array<CSAMPLE, 6> m_surround{};

  private:
    QSemaphore* m_pReady;
    bool m_captured = false;
};

TEST_F(EncoderRecordingTest, SidechainKeepsStereoAndSurroundChannelsSeparate) {
    QSemaphore ready;
    std::vector<CSAMPLE> stereo(kMaxEngineFrames * 2);
    std::array<std::vector<CSAMPLE>, 3> pairBuffers;
    for (auto& pair : pairBuffers) {
        pair.resize(kMaxEngineFrames * 2);
    }
    for (unsigned int frame = 0; frame < kMaxEngineFrames; ++frame) {
        stereo[frame * 2] = 0.7f;
        stereo[frame * 2 + 1] = 0.8f;
        for (int pair = 0; pair < 3; ++pair) {
            pairBuffers[pair][frame * 2] = static_cast<float>(pair * 2 + 1);
            pairBuffers[pair][frame * 2 + 1] = static_cast<float>(pair * 2 + 2);
        }
    }
    const std::array<const CSAMPLE*, 3> pairs = {
            pairBuffers[0].data(), pairBuffers[1].data(), pairBuffers[2].data()};
    EngineSideChain sidechain(m_pConfig, stereo.data());
    auto worker = std::make_unique<CaptureSidechain>(&ready);
    auto* pWorker = worker.get();
    sidechain.addSideChainWorker(worker.release());
    bool captured = false;
    for (int i = 0; i < 50 && !captured; ++i) {
        sidechain.writeSamples(stereo.data(), pairs, static_cast<int>(kMaxEngineFrames));
        captured = ready.tryAcquire(1, 100);
    }
    ASSERT_TRUE(captured);
    EXPECT_FLOAT_EQ(0.7f, pWorker->m_stereo[0]);
    EXPECT_FLOAT_EQ(0.8f, pWorker->m_stereo[1]);
    for (std::size_t channel = 0; channel < pWorker->m_surround.size(); ++channel) {
        EXPECT_FLOAT_EQ(static_cast<float>(channel + 1), pWorker->m_surround[channel]);
    }
}

} // namespace
