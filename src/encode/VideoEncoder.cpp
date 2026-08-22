#include "VideoEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
}

#include <cstring>
#include <thread>

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
    if (codec->name && std::strcmp(codec->name, "libx264") == 0)
        av_opt_set(m_videoCtx->priv_data, "preset", "veryfast", 0);

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

    m_audioFrame = av_frame_alloc();
    m_audioFrame->format = m_audioCtx->sample_fmt;
    m_audioFrame->sample_rate = m_audioCtx->sample_rate;
    av_channel_layout_copy(&m_audioFrame->ch_layout, &m_audioCtx->ch_layout);
    m_audioFrame->nb_samples = m_audioCtx->frame_size;
    if (av_frame_get_buffer(m_audioFrame, 0) < 0) {
        emit errorOccurred(QStringLiteral("Cannot allocate audio frame"));
        return false;
    }
    return true;
}

bool VideoEncoder::openAudioInput()
{
    avdevice_register_all();
    const AVInputFormat *inFmt = av_find_input_format("pulse");
    if (!inFmt) {
        emit errorOccurred(QStringLiteral("FFmpeg pulse input not available"));
        return false;
    }
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "sample_rate", "48000", 0);
    av_dict_set(&opts, "channels", "2", 0);
    int r = avformat_open_input(&m_audioInput, "default", inFmt, &opts);
    av_dict_free(&opts);
    if (r < 0) {
        emit errorOccurred(QStringLiteral("Cannot open default audio input (Pulse/PipeWire)"));
        return false;
    }
    if (avformat_find_stream_info(m_audioInput, nullptr) < 0) {
        emit errorOccurred(QStringLiteral("Cannot read audio input stream info"));
        avformat_close_input(&m_audioInput);
        return false;
    }

    // Resampler: input (whatever pulse gives) -> FLTP 48k stereo.
    AVStream *inSt = m_audioInput->streams[0];
    AVCodecParameters *par = inSt->codecpar;
    swr_alloc_set_opts2(&m_swr,
                        &m_audioCtx->ch_layout, AV_SAMPLE_FMT_FLTP, m_audioCtx->sample_rate,
                        &par->ch_layout, (AVSampleFormat)par->format, par->sample_rate,
                        0, nullptr);
    if (!m_swr || swr_init(m_swr) < 0) {
        emit errorOccurred(QStringLiteral("Audio resampler init failed"));
        avformat_close_input(&m_audioInput);
        return false;
    }
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
        // Audio is best-effort: continue video-only if capture init fails.
        audioOk = initAudioStream();
        if (audioOk && !openAudioInput())
            audioOk = false;
    }

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

    m_audioStop.store(false);
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
    AVFrame *inFrame = av_frame_alloc();

    while (!m_audioStop.load()) {
        if (av_read_frame(m_audioInput, pkt) < 0)
            break;

        // Decode not needed: pulse input delivers raw PCM.
        // Copy raw samples into a resampled frame.
        AVStream *inSt = m_audioInput->streams[pkt->stream_index];
        AVCodecParameters *par = inSt->codecpar;
        int inSamples = pkt->size / (par->ch_layout.nb_channels *
                                     av_get_bytes_per_sample((AVSampleFormat)par->format));
        if (inSamples <= 0) {
            av_packet_unref(pkt);
            continue;
        }

        QMutexLocker locker(&m_mutex);
        if (m_audioStop.load()) {
            av_packet_unref(pkt);
            break;
        }

        if (av_frame_make_writable(m_audioFrame) == 0) {
            const uint8_t *inData[1] = {pkt->data};
            int outSamples = swr_convert(m_swr,
                                         m_audioFrame->data, m_audioFrame->nb_samples,
                                         inData, inSamples);
            if (outSamples > 0) {
                m_audioFrame->nb_samples = outSamples;
                m_audioFrame->pts = m_audioPts;
                m_audioPts += outSamples;

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
            }
        }
        locker.unlock();
        av_packet_unref(pkt);
    }

    av_frame_free(&inFrame);
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
        // Flush encoders.
        if (m_videoCtx) {
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
        if (m_audioCtx && m_withAudio) {
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
