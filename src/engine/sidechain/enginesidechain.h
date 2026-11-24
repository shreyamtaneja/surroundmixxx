#pragma once

#include <QList>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <array>
#include <atomic>
#include <vector>

#include "preferences/usersettings.h"
#include "recording/defs_recording.h"
#include "soundio/soundmanagerutil.h"
#include "util/fifo.h"
#include "util/mutex.h"
#include "util/types.h"

class SideChainWorker;

class EngineSideChain : public QThread, public AudioDestination {
    Q_OBJECT
  public:
    EngineSideChain(UserSettingsPointer pConfig, CSAMPLE* sidechainMix);
    ~EngineSideChain() override;

    // Not thread-safe, wait-free. Submit buffer of samples to the sidechain for
    // processing. Should only be called from a single writer thread (typically
    // the engine callback).
    void writeSamples(const CSAMPLE* pStereo,
            const std::array<const CSAMPLE*, 3>& surroundPairs,
            int iFrames);

    void setExternalInputConnected(bool connected) {
        m_externalInputConnected.store(connected, std::memory_order_relaxed);
    }
    void setSurroundRecordingRequested(bool requested) {
        m_surroundRecordingRequested.store(requested, std::memory_order_relaxed);
    }
    bool surroundRecordingRequested() const {
        return m_surroundRecordingRequested.load(std::memory_order_relaxed);
    }

    // Thin wrapper around writeSamples that is used by SoundManager when receiving
    // from a sound card input instead of the engine
    void receiveBuffer(const AudioInput& input,
            const CSAMPLE* pBuffer,
            unsigned int iFrames) override;

    // Thread-safe, blocking.
    void addSideChainWorker(SideChainWorker* pWorker);

    static constexpr int SIDECHAIN_BUFFER_SIZE = 65536;

  private:
    void run() override;

    UserSettingsPointer m_pConfig;
    // Indicates that the thread should exit.
    volatile bool m_bStopThread;

    struct Frame {
        std::array<CSAMPLE, 2> stereo;
        std::array<CSAMPLE, mixxx::recording::kSurroundChannelCount> surround;
    };
    FIFO<Frame> m_sampleFifo;
    std::vector<Frame> m_writeBuffer;
    std::vector<Frame> m_workBuffer;
    std::vector<CSAMPLE> m_stereoBuffer;
    std::vector<CSAMPLE> m_surroundBuffer;
    CSAMPLE* m_pSidechainMix;
    std::atomic<bool> m_externalInputConnected{false};
    std::atomic<bool> m_surroundRecordingRequested{false};

    // Provides thread safety around the wait condition below.
    QMutex m_waitLock;
    // Allows sleeping until we have samples to process.
    QWaitCondition m_waitForSamples;

    // Sidechain workers registered with EngineSideChain.
    MMutex m_workerLock;
    QList<SideChainWorker*> m_workers GUARDED_BY(m_workerLock);
};
