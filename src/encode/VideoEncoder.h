#ifndef CMI_ENCODE_VIDEOENCODER_H
#define CMI_ENCODE_VIDEOENCODER_H

#include <QObject>
#include <QSize>
#include <QString>
#include <QList>
#include <QMutex>
#include <atomic>
#include <thread>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
struct SwrContext;
struct AVAudioFifo;

namespace cmi {

struct AudioDeviceInfo {
    QString id;       // Device ID passed to demuxer (e.g. "default", "DummyMic", "plughw:0,0", ":0")
    QString name;     // Friendly display name
    QString backend;  // "pulse", "alsa", "avfoundation", "dshow"
};

// Records the active video stream to H264/MP4 via libavformat, muxing audio
// captured from PulseAudio/PipeWire, ALSA, CoreAudio, or DirectShow in a background thread.
class VideoEncoder : public QObject {
    Q_OBJECT
public:
    explicit VideoEncoder(QObject *parent = nullptr);
    ~VideoEncoder() override;

    bool isRecording() const { return m_recording.load(); }
    bool hasAudio() const { return m_withAudio; }

    QString audioDevice() const { return m_audioDevice; }
    QString audioBackend() const { return m_audioBackend; }
    void setAudioDevice(const QString &device, const QString &backend = QString()) {
        m_audioDevice = device;
        m_audioBackend = backend;
    }

    static QList<AudioDeviceInfo> availableAudioDevices();
    static bool isAudioInputAvailable();

public slots:
    bool start(const QString &path, QSize size, int fps, bool withAudio);
    void stop();
    // Feed one RGBA frame; must be called from any thread (internally synced).
    void writeFrame(const uchar *pixels, int bytes, QSize size);

signals:
    void errorOccurred(const QString &message);
    void recordingStarted(const QString &path);
    void recordingStopped(const QString &path);
    void audioInputOpened(const QString &deviceName);
    void audioInputFailed(const QString &reason);

private:
    bool initVideoStream(QSize size, int fps);
    bool initAudioStream();
    bool openAudioInput();
    void closeAudioInput();
    void audioThreadMain();
    void cleanup();

    QString m_path;
    std::atomic<bool> m_recording{false};
    bool m_withAudio = false;
    QString m_audioDevice;
    QString m_audioBackend;

    QMutex m_mutex;
    AVFormatContext *m_fmt = nullptr;
    AVCodecContext *m_videoCtx = nullptr;
    AVCodecContext *m_audioCtx = nullptr;
    AVFrame *m_videoFrame = nullptr;
    AVFrame *m_audioFrame = nullptr;
    SwsContext *m_sws = nullptr;
    SwrContext *m_swr = nullptr;
    AVAudioFifo *m_audioFifo = nullptr;

    int m_videoStreamIdx = -1;
    int m_audioStreamIdx = -1;
    int64_t m_videoPts = 0;
    int64_t m_audioPts = 0;
    int m_fps = 30;

    // Audio capture demuxer context
    AVFormatContext *m_audioInput = nullptr;
    std::thread *m_audioThread = nullptr;
    std::atomic<bool> m_audioStop{false};

    int m_inSampleRate = 48000;
    int m_inChannels = 2;
    int m_inBytesPerSample = 2;
    int m_inSampleFmt = 0;
};

} // namespace cmi

#endif // CMI_ENCODE_VIDEOENCODER_H
