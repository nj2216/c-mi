#include "PreviewWidget.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <QOpenGLBuffer>
#include <linux/videodev2.h>
#include <cstring>

namespace cmi {

namespace {
const char *kVertSrc = R"(#version 330 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex;
out vec2 vTex;
void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    vTex = tex;
})";

const char *kFragSrc = R"(#version 330 core
in vec2 vTex;
out vec4 fragColor;
uniform sampler2D frameTex;
void main() {
    fragColor = texture(frameTex, vTex);
})";

// Fullscreen quad: position.xy, texcoord.xy
const float kQuad[] = {
    -1.f, -1.f,  0.f, 1.f,
     1.f, -1.f,  1.f, 1.f,
     1.f,  1.f,  1.f, 0.f,
    -1.f, -1.f,  0.f, 1.f,
     1.f,  1.f,  1.f, 0.f,
    -1.f,  1.f,  0.f, 0.f,
};
} // namespace

PreviewWidget::PreviewWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    if (codec) {
        m_mjpegCtx = avcodec_alloc_context3(codec);
        if (m_mjpegCtx && avcodec_open2(m_mjpegCtx, codec, nullptr) < 0) {
            avcodec_free_context(&m_mjpegCtx);
        }
    }
    m_mjpegFrame = av_frame_alloc();
    m_mjpegPkt = av_packet_alloc();
}

PreviewWidget::~PreviewWidget()
{
    makeCurrent();
    delete m_texture;
    delete m_program;
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    doneCurrent();

    if (m_mjpegSws) sws_freeContext(m_mjpegSws);
    if (m_mjpegFrame) av_frame_free(&m_mjpegFrame);
    if (m_mjpegPkt) av_packet_free(&m_mjpegPkt);
    if (m_mjpegCtx) avcodec_free_context(&m_mjpegCtx);
}

void PreviewWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertSrc);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc);
    m_program->link();

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void PreviewWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void PreviewWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    QImage frame;
    {
        QMutexLocker locker(&m_frameMutex);
        if (!m_pending.isNull()) {
            frame = m_pending;
            m_pending = QImage();
        }
    }

    if (!frame.isNull()) {
        m_lastFrame = frame;
        m_frameSize = frame.size();
        m_hasFrame = true;
    }

    if (!m_hasFrame || m_lastFrame.isNull())
        return;

    // Upload as GL texture, recreating when the size changes or a new
    // frame was staged this paint cycle.
    if (m_texture && m_texture->width() == m_frameSize.width() &&
        m_texture->height() == m_frameSize.height() && frame.isNull()) {
        // reuse existing texture, nothing new to upload
    } else {
        delete m_texture;
        QImage tex = m_lastFrame.convertToFormat(QImage::Format_RGBA8888);
        m_texture = new QOpenGLTexture(tex.mirrored(),
                                       QOpenGLTexture::DontGenerateMipMaps);
        m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    m_program->bind();
    glBindVertexArray(m_vao);
    m_texture->bind(0);
    m_program->setUniformValue("frameTex", 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_texture->release();
    glBindVertexArray(0);
    m_program->release();
}

void PreviewWidget::clearFrame()
{
    QMutexLocker locker(&m_frameMutex);
    m_pending = QImage();
    m_lastFrame = QImage();
    m_hasFrame = false;
    update();
}

QImage PreviewWidget::lastFrame() const
{
    QMutexLocker locker(&const_cast<QMutex &>(m_frameMutex));
    return m_lastFrame;
}

bool PreviewWidget::decodeMjpeg(const uchar *data, int bytes, QImage &out)
{
    if (!m_mjpegCtx)
        return false;

    av_packet_unref(m_mjpegPkt);
    if (av_new_packet(m_mjpegPkt, bytes) < 0)
        return false;
    std::memcpy(m_mjpegPkt->data, data, bytes);

    if (avcodec_send_packet(m_mjpegCtx, m_mjpegPkt) < 0)
        return false;
    if (avcodec_receive_frame(m_mjpegCtx, m_mjpegFrame) < 0)
        return false;

    const int w = m_mjpegFrame->width;
    const int h = m_mjpegFrame->height;
    if (w <= 0 || h <= 0)
        return false;

    m_mjpegSws = sws_getCachedContext(m_mjpegSws,
                                      w, h, (AVPixelFormat)m_mjpegFrame->format,
                                      w, h, AV_PIX_FMT_RGBA,
                                      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_mjpegSws)
        return false;

    out = QImage(w, h, QImage::Format_RGBA8888);
    uint8_t *dstData[4] = {out.bits(), nullptr, nullptr, nullptr};
    int dstStride[4] = {static_cast<int>(out.bytesPerLine()), 0, 0, 0};
    sws_scale(m_mjpegSws, m_mjpegFrame->data, m_mjpegFrame->linesize,
              0, h, dstData, dstStride);
    return true;
}

QImage PreviewWidget::yuyvToRgba(const uchar *data, QSize size)
{
    const int w = size.width();
    const int h = size.height();
    QImage out(w, h, QImage::Format_RGBA8888);

    for (int y = 0; y < h; ++y) {
        const uchar *src = data + y * w * 2;
        uchar *dst = out.scanLine(y);
        for (int x = 0; x < w; x += 2) {
            int y0 = src[0], u = src[1], y1 = src[2], v = src[3];
            src += 4;
            auto writePx = [&](uchar *d, int yy) {
                int c = yy - 16, dU = u - 128, dV = v - 128;
                int r = (298 * c + 409 * dV + 128) >> 8;
                int g = (298 * c - 100 * dU - 208 * dV + 128) >> 8;
                int b = (298 * c + 516 * dU + 128) >> 8;
                d[0] = static_cast<uchar>(qBound(0, r, 255));
                d[1] = static_cast<uchar>(qBound(0, g, 255));
                d[2] = static_cast<uchar>(qBound(0, b, 255));
                d[3] = 255;
            };
            writePx(dst, y0);
            dst += 4;
            if (x + 1 < w) {
                writePx(dst, y1);
                dst += 4;
            }
        }
    }
    return out;
}

void PreviewWidget::presentFrame(const uchar *data, int bytes,
                                 QSize size, uint32_t pixFmt)
{
    if (!data || bytes <= 0)
        return;

    QImage decoded;
    bool ok = false;

    if (pixFmt == V4L2_PIX_FMT_MJPEG) {
        ok = decodeMjpeg(data, bytes, decoded);
    } else if (pixFmt == V4L2_PIX_FMT_YUYV) {
        if (bytes >= size.width() * size.height() * 2) {
            decoded = yuyvToRgba(data, size);
            ok = !decoded.isNull();
        }
    }
    // Unsupported formats (e.g. raw H264 in preview) are dropped silently.

    if (!ok)
        return;

    {
        QMutexLocker locker(&m_frameMutex);
        m_pending = decoded;
    }
    update();
}

} // namespace cmi
