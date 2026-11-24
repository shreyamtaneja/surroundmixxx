// This class provides a way to do audio processing that does not need
// to be executed in real-time. For example, broadcast encoding
// and recording encoding can be done here. This class uses double-buffering
// to increase the amount of time the CPU has to do whatever work needs to
// be done, and that work is executed in a separate thread. (Threading
// allows the next buffer to be filled while processing a buffer that's is
// already full.)

#include "engine/sidechain/enginesidechain.h"

#include <QtDebug>

#include "engine/engine.h"
#include "engine/sidechain/sidechainworker.h"
#include "moc_enginesidechain.cpp"
#include "util/counter.h"
#include "util/defs.h"
#include "util/event.h"
#include "util/sample.h"
#include "util/trace.h"

EngineSideChain::EngineSideChain(
        UserSettingsPointer pConfig,
        CSAMPLE* sidechainMix)
        : m_pConfig(pConfig),
          m_bStopThread(false),
          m_sampleFifo(SIDECHAIN_BUFFER_SIZE / 2),
          m_writeBuffer(kMaxEngineFrames),
          m_workBuffer(SIDECHAIN_BUFFER_SIZE / 2),
          m_stereoBuffer(SIDECHAIN_BUFFER_SIZE),
          m_surroundBuffer(SIDECHAIN_BUFFER_SIZE / 2 * mixxx::recording::kSurroundChannelCount),
          m_pSidechainMix(sidechainMix) {
    // We use HighPriority to prevent starvation by lower-priority processes (Qt
    // main thread, analysis, etc.). This used to be LowPriority but that is not
    // a suitable choice since we do semi-realtime tasks
    // in the sidechain thread. To get reliable timing, it's important
    // that this work be prioritized over the GUI and non-realtime tasks. See
    // discussion on issue #7272 and https://bugs.launchpad.net/mixxx/1.11/+bug/1194543.
    start(QThread::HighPriority);
}

EngineSideChain::~EngineSideChain() {
    m_waitLock.lock();
    m_bStopThread = true;
    m_waitForSamples.wakeAll();
    m_waitLock.unlock();

    // Wait until the thread has finished.
    wait();

    MMutexLocker locker(&m_workerLock);
    while (!m_workers.empty()) {
        SideChainWorker* pWorker = m_workers.takeLast();
        pWorker->shutdown();
        delete pWorker;
    }
    locker.unlock();
}

void EngineSideChain::addSideChainWorker(SideChainWorker* pWorker) {
    MMutexLocker locker(&m_workerLock);
    m_workers.append(pWorker);
}

void EngineSideChain::receiveBuffer(const AudioInput& input,
        const CSAMPLE* pBuffer,
        unsigned int iFrames) {
    VERIFY_OR_DEBUG_ASSERT(input.getType() == AudioPathType::RecordBroadcast) {
        qDebug() << "WARNING: AudioInput type is not RECORD_BROADCAST. Ignoring incoming buffer.";
        return;
    }
    // Just copy the received samples form the sound card input to the
    // engine. After processing we get it back via writeSamples()
    SampleUtil::copy(m_pSidechainMix, pBuffer, iFrames * mixxx::kEngineChannelOutputCount);
}

void EngineSideChain::writeSamples(const CSAMPLE* pStereo,
        const std::array<const CSAMPLE*, 3>& surroundPairs,
        int iFrames) {
    Trace sidechain("EngineSideChain::writeSamples");
    VERIFY_OR_DEBUG_ASSERT(iFrames >= 0 &&
            static_cast<std::size_t>(iFrames) <= m_writeBuffer.size()) {
        return;
    }
    for (int frame = 0; frame < iFrames; ++frame) {
        auto& dst = m_writeBuffer[frame];
        for (int channel = 0; channel < 2; ++channel) {
            dst.stereo[channel] = pStereo[frame * 2 + channel];
        }
        for (int pair = 0; pair < 3; ++pair) {
            dst.surround[pair * 2] = surroundPairs[pair][frame * 2];
            dst.surround[pair * 2 + 1] = surroundPairs[pair][frame * 2 + 1];
        }
    }
    const int framesWritten = m_sampleFifo.write(m_writeBuffer.data(), iFrames);

    if (framesWritten != iFrames) {
        Counter("EngineSideChain::writeSamples buffer overrun").increment();
    }

    if (m_sampleFifo.writeAvailable() < SIDECHAIN_BUFFER_SIZE / 10) {
        // Signal to the sidechain that samples are available.
        Trace wakeup("EngineSideChain::writeSamples wake up");
        m_waitForSamples.wakeAll();
    }
}

void EngineSideChain::run() {
    // the id of this thread, for debugging purposes //XXX copypasta (should
    // factor this out somehow), -kousu 2/2009
    unsigned static id = 0;
    QThread::currentThread()->setObjectName(QString("EngineSideChain %1").arg(++id));
    static const QString tag("EngineSideChain");
    Event::start(tag);
    while (!m_bStopThread) {
        // Sleep until samples are available.
        m_waitLock.lock();

        Event::end(tag);
        m_waitForSamples.wait(&m_waitLock);
        m_waitLock.unlock();
        Event::start(tag);

        int framesRead;
        while ((framesRead = m_sampleFifo.read(m_workBuffer.data(),
                        static_cast<int>(m_workBuffer.size())))) {
            Trace process("EngineSideChain::process");
            for (int frame = 0; frame < framesRead; ++frame) {
                for (int channel = 0; channel < 2; ++channel) {
                    m_stereoBuffer[frame * 2 + channel] = m_workBuffer[frame].stereo[channel];
                }
                for (int channel = 0; channel < mixxx::recording::kSurroundChannelCount;
                        ++channel) {
                    m_surroundBuffer[frame * mixxx::recording::kSurroundChannelCount + channel] =
                            m_workBuffer[frame].surround[channel];
                }
            }
            MMutexLocker locker(&m_workerLock);
            foreach (SideChainWorker* pWorker, m_workers) {
                pWorker->processWithSurround(m_stereoBuffer.data(),
                        m_surroundBuffer.data(),
                        framesRead,
                        m_externalInputConnected.load(std::memory_order_relaxed));
            }
        }

        // Check to see if we're supposed to exit/stop this thread.
        if (m_bStopThread) {
            return;
        }
    }
}
