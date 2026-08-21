#include "PhotoEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}

#include <QFile>

namespace cmi {

bool PhotoEncoder::save(const QString &path, const uchar *pixels,
                        QSize size, Format format, QString *errorOut)
{
    const AVCodecID codecId = (format == Format::PNG) ? AV_CODEC_ID_PNG
                                                      : AV_CODEC_ID_MJPEG;
    const AVCodec *codec = avcodec_find_encoder(codecId);
    if (!codec) {
        if (errorOut) *errorOut = QStringLiteral("Encoder not found");
        return false;
    }

    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        if (errorOut) *errorOut = QStringLiteral("Cannot allocate codec context");
        return false;
    }

    ctx->width = size.width();
    ctx->height = size.height();
    ctx->time_base = {1, 25};
    ctx->pix_fmt = (format == Format::PNG) ? AV_PIX_FMT_RGB24
                                           : AV_PIX_FMT_YUVJ420P;

    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        if (errorOut) *errorOut = QStringLiteral("Cannot open codec");
        avcodec_free_context(&ctx);
        return false;
    }

    // Convert RGBA -> target pixel format.
    SwsContext *sws = sws_getContext(size.width(), size.height(), AV_PIX_FMT_RGBA,
                                     size.width(), size.height(), ctx->pix_fmt,
                                     SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!sws) {
        if (errorOut) *errorOut = QStringLiteral("sws_getContext failed");
        avcodec_free_context(&ctx);
        return false;
    }

    AVFrame *frame = av_frame_alloc();
    frame->format = ctx->pix_fmt;
    frame->width = ctx->width;
    frame->height = ctx->height;
    av_frame_get_buffer(frame, 32);

    const uint8_t *srcData[4] = {pixels, nullptr, nullptr, nullptr};
    int srcStride[4] = {size.width() * 4, 0, 0, 0};
    sws_scale(sws, srcData, srcStride, 0, size.height(),
              frame->data, frame->linesize);

    AVPacket *pkt = av_packet_alloc();
    bool ok = false;
    if (avcodec_send_frame(ctx, frame) == 0 &&
        avcodec_receive_packet(ctx, pkt) == 0) {
        QFile out(path);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(reinterpret_cast<const char *>(pkt->data), pkt->size);
            ok = true;
        } else if (errorOut) {
            *errorOut = QStringLiteral("Cannot write %1").arg(path);
        }
    } else if (errorOut) {
        *errorOut = QStringLiteral("Encode failed");
    }

    av_packet_free(&pkt);
    av_frame_free(&frame);
    sws_freeContext(sws);
    avcodec_free_context(&ctx);
    return ok;
}

} // namespace cmi
