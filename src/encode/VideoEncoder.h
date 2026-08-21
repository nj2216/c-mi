#ifndef CMI_ENCODE_VIDEOENCODER_H
#define CMI_ENCODE_VIDEOENCODER_H

#include <QObject>
#include <QSize>
#include <QString>
#include <QMutex>
#include <atomic>
#include <thread>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
struct SwrContext;

namespace cmi {

// Records the active video stream to H264/MP4 via libavformat, muxing audio
// captured from the default PipeWire/Pulse input (via FFmpeg's pulse demuxer)
// in a background thread.
class VideoEncoder : public QObject {
    Q_OBJECT
public:
    explicit VideoEncoder(QObject *parent = nullptr);
    ~VideoEncoder() override;

    bool isRecording() const { return m_recording.load(); }

public slots:
    bool start(const QString &path, QSize size, int fps, bool withAudio);
    void stop();
    // Feed one RGBA frame; must be called from any thread (internally synced).
    void writeFrame(const uchar *pixels, int bytes, QSize size);

signals:
    void errorOccurred(const QString &message);
    void recordingStarted(const QString &path);
    void recordingStopped(const QString &path);

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

    QMutex m_mutex;
    AVFormatContext *m_fmt = nullptr;
    AVCodecContext *m_videoCtx = nullptr;
    AVCodecContext *m_audioCtx = nullptr;
    AVFrame *m_videoFrame = nullptr;
    AVFrame *m_audioFrame = nullptr;
    SwsContext *m_sws = nullptr;
    SwrContext *m_swr = nullptr;
    int m_videoStreamIdx = -1;
    int m_audioStreamIdx = -1;
    int64_t m_videoPts = 0;
    int64_t m_audioPts = 0;
    int m_fps = 30;

    // Audio capture from default Pulse/PipeWire-pulse source.
    AVFormatContext *m_audioInput = nullptr;
    std::thread *m_audioThread = nullptr;
    std::atomic<bool> m_audioStop{false};
};

} // namespace cmi

#endif // CMI_ENCODE_VIDEOENCODER_H
