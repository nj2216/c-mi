#include "VideoEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
}

#include <cstring>
#include <thread>
#include <algorithm>
#include <QFile>
#include <QTextStream>

namespace cmi {

namespace {
// Not every FFmpeg build ships libx264/libopenh264; try known H264 encoders
// by name first, then fall back to a codec that's always built in so
// recording still works (at reduced compression efficiency).
const AVCodec *findVideoEncoder(AVCodecID *outId)
{
    static const char *kH264Names[] = {
        "libx264", "libopenh264", "h264_v4l2m2m", "h264_vaapi", "h264_nvenc",
    };
    for (const char *name : kH264Names) {
        if (const AVCodec *c = avcodec_find_encoder_by_name(name)) {
            *outId = AV_CODEC_ID_H264;
            return c;
        }
    }
    if (const AVCodec *c = avcodec_find_encoder(AV_CODEC_ID_H264)) {
        *outId = AV_CODEC_ID_H264;
        return c;
    }
    if (const AVCodec *c = avcodec_find_encoder(AV_CODEC_ID_MPEG4)) {
        *outId = AV_CODEC_ID_MPEG4;
        return c;
    }
    *outId = AV_CODEC_ID_NONE;
    return nullptr;
}
} // namespace

VideoEncoder::VideoEncoder(QObject *parent) : QObject(parent) {}

VideoEncoder::~VideoEncoder()
{
    stop();
}

QList<AudioDeviceInfo> VideoEncoder::availableAudioDevices()
{
    avdevice_register_all();
    QList<AudioDeviceInfo> list;

    // Check pulse backend
    if (av_find_input_format("pulse")) {
        list.append({QStringLiteral("default"),
                     QStringLiteral("Default Microphone (PulseAudio / PipeWire)"),
                     QStringLiteral("pulse")});
    }

    // Check alsa backend
    if (av_find_input_format("alsa")) {
        list.append({QStringLiteral("default"),
                     QStringLiteral("Default Microphone (ALSA)"),
                     QStringLiteral("alsa")});

        // Query ALSA capture devices from procfs if available
        QFile pcmFile(QStringLiteral("/proc/asound/pcm"));
        if (pcmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream ts(&pcmFile);
            while (!ts.atEnd()) {
                QString line = ts.readLine();
                if (line.contains(QStringLiteral("capture"), Qt::CaseInsensitive)) {
                    // Line format: "00-00: ALC892 Analog : ALC892 Analog : playback 1 : capture 1"
                    QStringList parts = line.split(QLatin1Char(':'));
                    if (parts.size() >= 2) {
                        QString cardDev = parts[0].trimmed();
                        QString name = parts[1].trimmed();
                        QStringList cd = cardDev.split(QLatin1Char('-'));
                        if (cd.size() == 2) {
                            int card = cd[0].toInt();
                            int dev = cd[1].toInt();
                            QString hwId = QStringLiteral("hw:%1,%2").arg(card).arg(dev);
                            list.append({hwId,
                                         QStringLiteral("%1 (%2)").arg(name, hwId),
                                         QStringLiteral("alsa")});
                        }
                    }
                }
            }
        }
    }

    return list;
}

bool VideoEncoder::isAudioInputAvailable()
{
    return !availableAudioDevices().isEmpty();
}

bool VideoEncoder::initVideoStream(QSize size, int fps)
{
    AVCodecID codecId = AV_CODEC_ID_NONE;
    const AVCodec *codec = findVideoEncoder(&codecId);
    if (!codec) {
        emit errorOccurred(QStringLiteral("No usable video encoder found (H264/MPEG4)"));
        return false;
    }

    AVStream *st = avformat_new_stream(m_fmt, nullptr);
    if (!st) {
        emit errorOccurred(QStringLiteral("Cannot allocate video stream"));
        return false;
    }
    m_videoStreamIdx = st->index;

    m_videoCtx = avcodec_alloc_context3(codec);
    m_videoCtx->width = size.width() & ~1;   // yuv420p needs even dims
    m_videoCtx->height = size.height() & ~1;
    m_videoCtx->time_base = {1, fps};
    m_videoCtx->framerate = {fps, 1};
    m_videoCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_videoCtx->gop_size = fps * 2;
    // Calculate high quality bitrate (e.g. 6 Mbps for 1080p, 3.5 Mbps for 720p, 1.5 Mbps for 480p)
    int64_t pixels = static_cast<int64_t>(m_videoCtx->width) * m_videoCtx->height;
    int64_t targetBitrate = std::clamp(pixels * 3, static_cast<int64_t>(1500000), static_cast<int64_t>(8000000));
    m_videoCtx->bit_rate = targetBitrate;
    m_videoCtx->rc_max_rate = targetBitrate * 12 / 10;
    m_videoCtx->rc_buffer_size = targetBitrate * 2;

    if (codec->name && std::strcmp(codec->name, "libx264") == 0) {
        av_opt_set(m_videoCtx->priv_data, "preset", "veryfast", 0);
        av_opt_set(m_videoCtx->priv_data, "crf", "18", 0); // High visually lossless quality
    }

    if (m_fmt->oformat->flags & AVFMT_GLOBALHEADER)
        m_videoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(m_videoCtx, codec, nullptr) < 0) {
        emit errorOccurred(QStringLiteral("Cannot open %1 codec")
                               .arg(QString::fromUtf8(codec->name ? codec->name : "video")));
        return false;
    }
    avcodec_parameters_from_context(st->codecpar, m_videoCtx);
    st->time_base = m_videoCtx->time_base;

    m_sws = sws_getContext(size.width(), size.height(), AV_PIX_FMT_RGBA,
                           m_videoCtx->width, m_videoCtx->height,
                           AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!m_sws) {
        emit errorOccurred(QStringLiteral("sws_getContext failed"));
        return false;
    }

    m_videoFrame = av_frame_alloc();
    m_videoFrame->format = AV_PIX_FMT_YUV420P;
    m_videoFrame->width = m_videoCtx->width;
    m_videoFrame->height = m_videoCtx->height;
    if (av_frame_get_buffer(m_videoFrame, 32) < 0) {
        emit errorOccurred(QStringLiteral("Cannot allocate video frame"));
        return false;
    }
    return true;
}

bool VideoEncoder::initAudioStream()
{
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        emit errorOccurred(QStringLiteral("AAC encoder not found"));
        return false;
    }

    AVStream *st = avformat_new_stream(m_fmt, nullptr);
    if (!st) {
        emit errorOccurred(QStringLiteral("Cannot allocate audio stream"));
        return false;
    }
    m_audioStreamIdx = st->index;

    m_audioCtx = avcodec_alloc_context3(codec);
    if (!m_audioCtx) {
        emit errorOccurred(QStringLiteral("Cannot allocate audio codec context"));
        return false;
    }

    m_audioCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    m_audioCtx->bit_rate = 128000;
    m_audioCtx->sample_rate = 48000;
    av_channel_layout_default(&m_audioCtx->ch_layout, 2);
    m_audioCtx->time_base = {1, m_audioCtx->sample_rate};

    if (m_fmt->oformat->flags & AVFMT_GLOBALHEADER)
        m_audioCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(m_audioCtx, codec, nullptr) < 0) {
        emit errorOccurred(QStringLiteral("Cannot open AAC codec"));
        return false;
    }
    avcodec_parameters_from_context(st->codecpar, m_audioCtx);
    st->time_base = m_audioCtx->time_base;

    const int frameSize = m_audioCtx->frame_size > 0 ? m_audioCtx->frame_size : 1024;

    m_audioFrame = av_frame_alloc();
    m_audioFrame->format = m_audioCtx->sample_fmt;
    m_audioFrame->sample_rate = m_audioCtx->sample_rate;
    av_channel_layout_copy(&m_audioFrame->ch_layout, &m_audioCtx->ch_layout);
    m_audioFrame->nb_samples = frameSize;
    if (av_frame_get_buffer(m_audioFrame, 0) < 0) {
        emit errorOccurred(QStringLiteral("Cannot allocate audio frame"));
        return false;
    }

    m_audioFifo = av_audio_fifo_alloc(m_audioCtx->sample_fmt, m_audioCtx->ch_layout.nb_channels, frameSize * 4);
    if (!m_audioFifo) {
        emit errorOccurred(QStringLiteral("Cannot allocate audio FIFO"));
        return false;
    }

    return true;
}

namespace {
static int audioInterruptCallback(void *opaque)
{
    auto *stopFlag = static_cast<std::atomic<bool>*>(opaque);
    return (stopFlag && stopFlag->load()) ? 1 : 0;
}
} // namespace

bool VideoEncoder::openAudioInput()
{
    avdevice_register_all();

    QString deviceToOpen = m_audioDevice.trimmed();
    if (deviceToOpen.isEmpty())
        deviceToOpen = QStringLiteral("default");

    QString chosenBackend;

    auto tryOpen = [&](const char *fmtName, const QString &devName) -> bool {
        const AVInputFormat *inFmt = av_find_input_format(fmtName);
        if (!inFmt)
            return false;

        m_audioInput = avformat_alloc_context();
        if (!m_audioInput)
            return false;

        m_audioInput->interrupt_callback.callback = audioInterruptCallback;
        m_audioInput->interrupt_callback.opaque = &m_audioStop;

        AVDictionary *opts = nullptr;
        int ret = avformat_open_input(&m_audioInput, devName.toUtf8().constData(), inFmt, &opts);
        av_dict_free(&opts);

        if (ret == 0 && m_audioInput) {
            chosenBackend = QString::fromUtf8(fmtName);
            return true;
        }

        if (m_audioInput) {
            avformat_close_input(&m_audioInput);
            m_audioInput = nullptr;
        }
        return false;
    };

    if (deviceToOpen.startsWith(QStringLiteral("hw:")) || deviceToOpen.startsWith(QStringLiteral("plughw:"))) {
        tryOpen("alsa", deviceToOpen);
    } else if (deviceToOpen == QStringLiteral("default")) {
        if (!tryOpen("pulse", QStringLiteral("default"))) {
            tryOpen("alsa", QStringLiteral("default"));
        }
    } else {
        if (!tryOpen("pulse", deviceToOpen)) {
            tryOpen("alsa", deviceToOpen);
        }
    }

    if (!m_audioInput) {
        emit audioInputFailed(QStringLiteral("No audio input device available"));
        return false;
    }

    if (avformat_find_stream_info(m_audioInput, nullptr) < 0 || m_audioInput->nb_streams == 0) {
        emit audioInputFailed(QStringLiteral("Cannot read audio input stream info"));
        avformat_close_input(&m_audioInput);
        m_audioInput = nullptr;
        return false;
    }

    AVStream *inSt = m_audioInput->streams[0];
    AVCodecParameters *par = inSt->codecpar;

    // Channels / Channel Layout
    AVChannelLayout inChLayout;
    std::memset(&inChLayout, 0, sizeof(inChLayout));
    if (par->ch_layout.nb_channels > 0) {
        av_channel_layout_copy(&inChLayout, &par->ch_layout);
    } else {
        av_channel_layout_default(&inChLayout, 2);
    }

    int inSampleRate = par->sample_rate > 0 ? par->sample_rate : 48000;

    AVSampleFormat inSampleFmt = (AVSampleFormat)par->format;
    if (inSampleFmt == AV_SAMPLE_FMT_NONE) {
        switch (par->codec_id) {
        case AV_CODEC_ID_PCM_S16LE:
        case AV_CODEC_ID_PCM_S16BE:
            inSampleFmt = AV_SAMPLE_FMT_S16;
            break;
        case AV_CODEC_ID_PCM_S32LE:
        case AV_CODEC_ID_PCM_S32BE:
            inSampleFmt = AV_SAMPLE_FMT_S32;
            break;
        case AV_CODEC_ID_PCM_F32LE:
        case AV_CODEC_ID_PCM_F32BE:
            inSampleFmt = AV_SAMPLE_FMT_FLT;
            break;
        case AV_CODEC_ID_PCM_U8:
            inSampleFmt = AV_SAMPLE_FMT_U8;
            break;
        default:
            inSampleFmt = AV_SAMPLE_FMT_S16;
            break;
        }
    }

    m_inSampleRate = inSampleRate;
    m_inChannels = inChLayout.nb_channels > 0 ? inChLayout.nb_channels : 2;
    m_inSampleFmt = inSampleFmt;
    m_inBytesPerSample = av_get_bytes_per_sample(inSampleFmt);
    if (m_inBytesPerSample <= 0)
        m_inBytesPerSample = 2;

    // Resampler: input -> FLTP 48k stereo
    AVChannelLayout outChLayout;
    av_channel_layout_default(&outChLayout, 2);

    int swrRet = swr_alloc_set_opts2(&m_swr,
                                     &outChLayout, AV_SAMPLE_FMT_FLTP, 48000,
                                     &inChLayout, inSampleFmt, inSampleRate,
                                     0, nullptr);
    av_channel_layout_uninit(&inChLayout);
    av_channel_layout_uninit(&outChLayout);

    if (swrRet < 0 || !m_swr || swr_init(m_swr) < 0) {
        emit audioInputFailed(QStringLiteral("Audio resampler init failed"));
        if (m_swr) {
            swr_free(&m_swr);
            m_swr = nullptr;
        }
        avformat_close_input(&m_audioInput);
        m_audioInput = nullptr;
        return false;
    }

    emit audioInputOpened(QStringLiteral("%1 [%2]").arg(deviceToOpen, chosenBackend));
    return true;
}

void VideoEncoder::closeAudioInput()
{
    if (m_audioInput) {
        avformat_close_input(&m_audioInput);
        m_audioInput = nullptr;
    }
}

bool VideoEncoder::start(const QString &path, QSize size, int fps, bool withAudio)
{
    if (m_recording.load())
        return true;

    m_path = path;
    m_fps = fps;
    m_withAudio = withAudio;
    m_videoPts = 0;
    m_audioPts = 0;

    if (avformat_alloc_output_context2(&m_fmt, nullptr, "mp4",
                                       path.toUtf8().constData()) < 0 || !m_fmt) {
        emit errorOccurred(QStringLiteral("Cannot allocate MP4 output context"));
        return false;
    }

    if (!initVideoStream(size, fps)) {
        cleanup();
        return false;
    }

    bool audioOk = false;
    if (withAudio) {
        m_audioStop.store(false);
        if (openAudioInput()) {
            if (initAudioStream()) {
                audioOk = true;
            } else {
                closeAudioInput();
            }
        }
    }
    m_withAudio = audioOk;

    if (!(m_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_fmt->pb, path.toUtf8().constData(), AVIO_FLAG_WRITE) < 0) {
            emit errorOccurred(QStringLiteral("Cannot open output file %1").arg(path));
            cleanup();
            return false;
        }
    }

    if (avformat_write_header(m_fmt, nullptr) < 0) {
        emit errorOccurred(QStringLiteral("Cannot write container header"));
        cleanup();
        return false;
    }

    if (audioOk) {
        m_audioThread = new std::thread(&VideoEncoder::audioThreadMain, this);
    }

    m_recording.store(true);
    emit recordingStarted(m_path);
    return true;
}

void VideoEncoder::writeFrame(const uchar *pixels, int bytes, QSize size)
{
    if (!m_recording.load() || !m_fmt || !m_videoCtx)
        return;
    if (bytes < size.width() * size.height() * 4)
        return;

    QMutexLocker locker(&m_mutex);
    if (!m_recording.load())
        return;

    if (av_frame_make_writable(m_videoFrame) < 0)
        return;

    const uint8_t *srcData[4] = {pixels, nullptr, nullptr, nullptr};
    int srcStride[4] = {size.width() * 4, 0, 0, 0};
    sws_scale(m_sws, srcData, srcStride, 0, size.height(),
              m_videoFrame->data, m_videoFrame->linesize);

    m_videoFrame->pts = m_videoPts++;

    if (avcodec_send_frame(m_videoCtx, m_videoFrame) < 0)
        return;

    AVPacket *pkt = av_packet_alloc();
    while (avcodec_receive_packet(m_videoCtx, pkt) == 0) {
        av_packet_rescale_ts(pkt, m_videoCtx->time_base,
                             m_fmt->streams[m_videoStreamIdx]->time_base);
        pkt->stream_index = m_videoStreamIdx;
        av_interleaved_write_frame(m_fmt, pkt);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}

void VideoEncoder::audioThreadMain()
{
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) return;

    const int frameSize = m_audioCtx ? (m_audioCtx->frame_size > 0 ? m_audioCtx->frame_size : 1024) : 1024;
    const int bytesPerSample = m_inBytesPerSample > 0 ? m_inBytesPerSample : 2;
    const int inChannels = m_inChannels > 0 ? m_inChannels : 2;
    const int inRate = m_inSampleRate > 0 ? m_inSampleRate : 48000;

    while (!m_audioStop.load()) {
        if (av_read_frame(m_audioInput, pkt) < 0)
            break;

        if (m_audioStop.load()) {
            av_packet_unref(pkt);
            break;
        }

        int inSamples = pkt->size / (inChannels * bytesPerSample);

        if (inSamples > 0 && m_swr) {
            int64_t delay = swr_get_delay(m_swr, inRate);
            int maxOutSamples = av_rescale_rnd(delay + inSamples, 48000, inRate, AV_ROUND_UP);
            if (maxOutSamples > 0) {
                uint8_t **outData = nullptr;
                int outLinesize = 0;
                if (av_samples_alloc_array_and_samples(&outData, &outLinesize, 2, maxOutSamples,
                                                       AV_SAMPLE_FMT_FLTP, 0) >= 0) {
                    const uint8_t *inData[1] = {pkt->data};
                    int converted = swr_convert(m_swr, outData, maxOutSamples, inData, inSamples);
                    if (converted > 0) {
                        QMutexLocker locker(&m_mutex);
                        if (!m_audioStop.load() && m_audioFifo && m_audioCtx && m_audioFrame) {
                            if (av_audio_fifo_space(m_audioFifo) < converted) {
                                int ret = av_audio_fifo_realloc(m_audioFifo, av_audio_fifo_size(m_audioFifo) + converted + frameSize * 2);
                                (void)ret;
                            }
                            av_audio_fifo_write(m_audioFifo, reinterpret_cast<void**>(outData), converted);

                            while (av_audio_fifo_size(m_audioFifo) >= frameSize) {
                                if (av_frame_make_writable(m_audioFrame) == 0) {
                                    m_audioFrame->nb_samples = frameSize;
                                    av_audio_fifo_read(m_audioFifo, reinterpret_cast<void**>(m_audioFrame->data), frameSize);
                                    m_audioFrame->pts = m_audioPts;
                                    m_audioPts += frameSize;

                                    if (avcodec_send_frame(m_audioCtx, m_audioFrame) == 0) {
                                        AVPacket *outPkt = av_packet_alloc();
                                        while (avcodec_receive_packet(m_audioCtx, outPkt) == 0) {
                                            av_packet_rescale_ts(outPkt, m_audioCtx->time_base,
                                                                 m_fmt->streams[m_audioStreamIdx]->time_base);
                                            outPkt->stream_index = m_audioStreamIdx;
                                            av_interleaved_write_frame(m_fmt, outPkt);
                                            av_packet_unref(outPkt);
                                        }
                                        av_packet_free(&outPkt);
                                    }
                                } else {
                                    break;
                                }
                            }
                        }
                    }
                    if (outData) {
                        av_freep(&outData[0]);
                        av_freep(&outData);
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}

void VideoEncoder::stop()
{
    if (!m_recording.load() && !m_fmt)
        return;

    m_recording.store(false);
    m_audioStop.store(true);
    if (m_audioThread) {
        m_audioThread->join();
        delete m_audioThread;
        m_audioThread = nullptr;
    }

    QMutexLocker locker(&m_mutex);
    if (m_fmt) {
        // Flush video encoder
        if (m_videoCtx && m_videoStreamIdx >= 0) {
            avcodec_send_frame(m_videoCtx, nullptr);
            AVPacket *pkt = av_packet_alloc();
            while (avcodec_receive_packet(m_videoCtx, pkt) == 0) {
                av_packet_rescale_ts(pkt, m_videoCtx->time_base,
                                     m_fmt->streams[m_videoStreamIdx]->time_base);
                pkt->stream_index = m_videoStreamIdx;
                av_interleaved_write_frame(m_fmt, pkt);
                av_packet_unref(pkt);
            }
            av_packet_free(&pkt);
        }

        // Flush audio encoder
        if (m_audioCtx && m_withAudio && m_audioStreamIdx >= 0) {
            const int frameSize = m_audioCtx->frame_size > 0 ? m_audioCtx->frame_size : 1024;
            const int inRate = m_inSampleRate > 0 ? m_inSampleRate : 48000;

            // Drain remaining samples from resampler into FIFO
            if (m_swr && m_audioFifo) {
                int64_t delay = swr_get_delay(m_swr, inRate);
                int drainSamples = av_rescale_rnd(delay, 48000, inRate, AV_ROUND_UP);
                if (drainSamples > 0) {
                    uint8_t **outData = nullptr;
                    int outLinesize = 0;
                    if (av_samples_alloc_array_and_samples(&outData, &outLinesize, 2, drainSamples, AV_SAMPLE_FMT_FLTP, 0) >= 0) {
                        int converted = swr_convert(m_swr, outData, drainSamples, nullptr, 0);
                        if (converted > 0) {
                            if (av_audio_fifo_space(m_audioFifo) < converted) {
                                int ret = av_audio_fifo_realloc(m_audioFifo, av_audio_fifo_size(m_audioFifo) + converted + frameSize * 2);
                                (void)ret;
                            }
                            av_audio_fifo_write(m_audioFifo, reinterpret_cast<void**>(outData), converted);
                        }
                        av_freep(&outData[0]);
                        av_freep(&outData);
                    }
                }
            }

            // Drain all remaining samples in FIFO to encoder, padding last partial frame with silence
            if (m_audioFifo && m_audioFrame) {
                while (av_audio_fifo_size(m_audioFifo) > 0) {
                    int samplesInFifo = av_audio_fifo_size(m_audioFifo);
                    int samplesToRead = std::min(samplesInFifo, frameSize);
                    if (av_frame_make_writable(m_audioFrame) == 0) {
                        m_audioFrame->nb_samples = frameSize;
                        av_audio_fifo_read(m_audioFifo, reinterpret_cast<void**>(m_audioFrame->data), samplesToRead);
                        if (samplesToRead < frameSize) {
                            for (int ch = 0; ch < 2; ++ch) {
                                float *buf = reinterpret_cast<float*>(m_audioFrame->data[ch]);
                                std::memset(buf + samplesToRead, 0, (frameSize - samplesToRead) * sizeof(float));
                            }
                        }
                        m_audioFrame->pts = m_audioPts;
                        m_audioPts += samplesToRead;

                        if (avcodec_send_frame(m_audioCtx, m_audioFrame) == 0) {
                            AVPacket *outPkt = av_packet_alloc();
                            while (avcodec_receive_packet(m_audioCtx, outPkt) == 0) {
                                av_packet_rescale_ts(outPkt, m_audioCtx->time_base,
                                                     m_fmt->streams[m_audioStreamIdx]->time_base);
                                outPkt->stream_index = m_audioStreamIdx;
                                av_interleaved_write_frame(m_fmt, outPkt);
                                av_packet_unref(outPkt);
                            }
                            av_packet_free(&outPkt);
                        }
                    } else {
                        break;
                    }
                }
            }

            // Flush encoder with NULL frame
            avcodec_send_frame(m_audioCtx, nullptr);
            AVPacket *pkt = av_packet_alloc();
            while (avcodec_receive_packet(m_audioCtx, pkt) == 0) {
                av_packet_rescale_ts(pkt, m_audioCtx->time_base,
                                     m_fmt->streams[m_audioStreamIdx]->time_base);
                pkt->stream_index = m_audioStreamIdx;
                av_interleaved_write_frame(m_fmt, pkt);
                av_packet_unref(pkt);
            }
            av_packet_free(&pkt);
        }

        av_write_trailer(m_fmt);
    }

    cleanup();

    QString finished = m_path;
    m_path.clear();
    if (!finished.isEmpty())
        emit recordingStopped(finished);
}

void VideoEncoder::cleanup()
{
    closeAudioInput();
    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = nullptr;
    }
    if (m_swr) {
        swr_free(&m_swr);
        m_swr = nullptr;
    }
    if (m_videoFrame) {
        av_frame_free(&m_videoFrame);
        m_videoFrame = nullptr;
    }
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
    }
    if (m_videoCtx) {
        avcodec_free_context(&m_videoCtx);
        m_videoCtx = nullptr;
    }
    if (m_audioCtx) {
        avcodec_free_context(&m_audioCtx);
        m_audioCtx = nullptr;
    }
    if (m_sws) {
        sws_freeContext(m_sws);
        m_sws = nullptr;
    }
    if (m_fmt) {
        if (!(m_fmt->oformat->flags & AVFMT_NOFILE) && m_fmt->pb)
            avio_closep(&m_fmt->pb);
        avformat_free_context(m_fmt);
        m_fmt = nullptr;
    }
    m_videoStreamIdx = -1;
    m_audioStreamIdx = -1;
}

} // namespace cmi
