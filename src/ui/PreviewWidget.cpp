#include "PreviewWidget.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <QOpenGLBuffer>
#include <linux/videodev2.h>
#include <cstring>
#include <cmath>
#include <algorithm>

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
uniform vec2 uImageSize;
uniform vec2 uViewSize;
uniform int uAspectRatioMode; // 0=Fit, 1=16:9, 2=4:3, 3=1:1
uniform bool uMirror;
uniform int uFilter; // 0=None, 1=Mono, 2=Sepia, 3=Cool, 4=Warm, 5=Cyber, 6=Noir, 7=Vintage, 8=Invert
uniform bool uShowGrid;
uniform float uFlashIntensity;

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 adjustHueSat(vec3 col, float hueShiftDeg, float satScale) {
    vec3 hsv = rgb2hsv(col);
    hsv.x = fract(hsv.x + hueShiftDeg / 360.0);
    hsv.y = clamp(hsv.y * satScale, 0.0, 1.0);
    return hsv2rgb(hsv);
}

vec3 applyFilter(vec3 col, int fx) {
    if (fx == 1) { // Grayscale / Mono
        float g = dot(col, vec3(0.299, 0.587, 0.114));
        col = vec3(g);
        col = (col - 0.5) * 1.08 + 0.5;
    } else if (fx == 2) { // Sepia
        vec3 sep;
        sep.r = dot(col, vec3(0.393, 0.769, 0.189));
        sep.g = dot(col, vec3(0.349, 0.686, 0.168));
        sep.b = dot(col, vec3(0.272, 0.534, 0.131));
        col = mix(col, sep, 0.65);
        col = adjustHueSat(col, 0.0, 1.2);
        col = (col - 0.5) * 1.05 + 0.5;
    } else if (fx == 3) { // Cool
        col = adjustHueSat(col, -12.0, 1.15);
        col *= 1.02;
    } else if (fx == 4) { // Warm
        col = adjustHueSat(col, 10.0, 1.2);
        col *= 1.04;
    } else if (fx == 5) { // Cyber
        col = (col - 0.5) * 1.3 + 0.5;
        col = adjustHueSat(col, 145.0, 1.8);
    } else if (fx == 6) { // Noir
        float g = dot(col, vec3(0.299, 0.587, 0.114));
        col = vec3(g);
        col = (col - 0.5) * 1.7 + 0.5;
        col *= 0.9;
    } else if (fx == 7) { // Vintage
        vec3 sep;
        sep.r = dot(col, vec3(0.393, 0.769, 0.189));
        sep.g = dot(col, vec3(0.349, 0.686, 0.168));
        sep.b = dot(col, vec3(0.272, 0.534, 0.131));
        col = mix(col, sep, 0.35);
        col = (col - 0.5) * 1.15 + 0.5;
        col = adjustHueSat(col, 0.0, 1.3);
        col *= 0.95;
    } else if (fx == 8) { // Invert
        col = vec3(1.0) - col;
        col = adjustHueSat(col, 180.0, 1.0);
    }
    return clamp(col, 0.0, 1.0);
}

void main() {
    float srcAspect = uImageSize.x / max(uImageSize.y, 1.0);
    float targetAspect = srcAspect;
    if (uAspectRatioMode == 1) {
        targetAspect = 16.0 / 9.0;
    } else if (uAspectRatioMode == 2) {
        targetAspect = 4.0 / 3.0;
    } else if (uAspectRatioMode == 3) {
        targetAspect = 1.0;
    }

    float viewAspect = uViewSize.x / max(uViewSize.y, 1.0);

    float scaleX = 1.0;
    float scaleY = 1.0;
    if (targetAspect > viewAspect) {
        scaleY = viewAspect / targetAspect;
    } else {
        scaleX = targetAspect / viewAspect;
    }

    vec2 targetUv = (vTex - vec2(0.5)) / vec2(scaleX, scaleY) + vec2(0.5);

    if (targetUv.x < 0.0 || targetUv.x > 1.0 || targetUv.y < 0.0 || targetUv.y > 1.0) {
        fragColor = vec4(0.05, 0.05, 0.06, 1.0);
        return;
    }

    float cropScaleX = 1.0;
    float cropScaleY = 1.0;
    if (targetAspect > srcAspect) {
        cropScaleY = srcAspect / targetAspect;
    } else {
        cropScaleX = targetAspect / srcAspect;
    }

    vec2 sampleUv = (targetUv - vec2(0.5)) * vec2(cropScaleX, cropScaleY) + vec2(0.5);

    if (uMirror) {
        sampleUv.x = 1.0 - sampleUv.x;
    }

    vec4 col = texture(frameTex, clamp(sampleUv, 0.0, 1.0));
    col.rgb = applyFilter(col.rgb, uFilter);

    if (uShowGrid) {
        float lineW = 1.5 / uViewSize.y;
        if (abs(targetUv.x - 0.3333) < lineW || abs(targetUv.x - 0.6666) < lineW ||
            abs(targetUv.y - 0.3333) < lineW || abs(targetUv.y - 0.6666) < lineW) {
            col.rgb = mix(col.rgb, vec3(1.0), 0.45);
        }
    }

    if (uFlashIntensity > 0.001) {
        col.rgb = mix(col.rgb, vec3(1.0), clamp(uFlashIntensity, 0.0, 1.0));
    }

    fragColor = col;
}
)";

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

    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(25);
    connect(m_flashTimer, &QTimer::timeout, this, [this] {
        m_flashIntensity -= 0.1f;
        if (m_flashIntensity <= 0.0f) {
            m_flashIntensity = 0.0f;
            m_flashTimer->stop();
        }
        update();
    });
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

void PreviewWidget::setFilter(ColorFilter filter)
{
    if (m_filter != filter) {
        m_filter = filter;
        update();
    }
}

void PreviewWidget::setAspectRatioMode(AspectRatioMode mode)
{
    if (m_arMode != mode) {
        m_arMode = mode;
        update();
    }
}

void PreviewWidget::setMirrored(bool mirrored)
{
    if (m_mirrored != mirrored) {
        m_mirrored = mirrored;
        update();
    }
}

void PreviewWidget::setShowGrid(bool show)
{
    if (m_showGrid != show) {
        m_showGrid = show;
        update();
    }
}

void PreviewWidget::triggerFlash()
{
    m_flashIntensity = 1.0f;
    if (m_flashTimer)
        m_flashTimer->start();
    update();
}

void PreviewWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.06f, 1.0f);

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
        m_texture = new QOpenGLTexture(tex,
                                       QOpenGLTexture::DontGenerateMipMaps);
        m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    m_program->bind();
    glBindVertexArray(m_vao);
    m_texture->bind(0);
    m_program->setUniformValue("frameTex", 0);
    m_program->setUniformValue("uImageSize", QVector2D(static_cast<float>(m_lastFrame.width()), static_cast<float>(m_lastFrame.height())));
    m_program->setUniformValue("uViewSize", QVector2D(static_cast<float>(width()), static_cast<float>(height())));
    m_program->setUniformValue("uAspectRatioMode", static_cast<int>(m_arMode));
    m_program->setUniformValue("uMirror", m_mirrored);
    m_program->setUniformValue("uFilter", static_cast<int>(m_filter));
    m_program->setUniformValue("uShowGrid", m_showGrid);
    m_program->setUniformValue("uFlashIntensity", m_flashIntensity);

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

bool PreviewWidget::hasFrame() const
{
    QMutexLocker locker(&const_cast<QMutex &>(m_frameMutex));
    return m_hasFrame && !m_lastFrame.isNull();
}

QImage PreviewWidget::lastRawFrame() const
{
    QMutexLocker locker(&const_cast<QMutex &>(m_frameMutex));
    return m_lastFrame;
}

QImage PreviewWidget::processedLastFrame() const
{
    QImage raw = lastRawFrame();
    if (raw.isNull())
        return QImage();
    return applyEffectsToImage(raw, m_filter, m_mirrored, m_arMode);
}

QImage PreviewWidget::applyEffectsToImage(const QImage &src, ColorFilter filter, bool mirror, AspectRatioMode ar)
{
    if (src.isNull())
        return QImage();

    QImage img = src.convertToFormat(QImage::Format_RGBA8888);

    // 1. Aspect Ratio crop
    if (ar != AspectRatioMode::Fit) {
        double targetAspect = 1.0;
        if (ar == AspectRatioMode::Ratio16_9) targetAspect = 16.0 / 9.0;
        else if (ar == AspectRatioMode::Ratio4_3) targetAspect = 4.0 / 3.0;
        else if (ar == AspectRatioMode::Ratio1_1) targetAspect = 1.0;

        double srcAspect = static_cast<double>(img.width()) / std::max(1, img.height());
        int cropW = img.width();
        int cropH = img.height();

        if (targetAspect > srcAspect) {
            cropH = static_cast<int>(cropW / targetAspect);
        } else {
            cropW = static_cast<int>(cropH * targetAspect);
        }
        int cropX = (img.width() - cropW) / 2;
        int cropY = (img.height() - cropH) / 2;
        img = img.copy(cropX, cropY, cropW, cropH);
    }

    // 2. Mirror
    if (mirror) {
        img = img.mirrored(true, false);
    }

    // 3. Filter
    if (filter != ColorFilter::None) {
        const int w = img.width();
        const int h = img.height();
        for (int y = 0; y < h; ++y) {
            uchar *scan = img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int r = scan[x * 4 + 0];
                int g = scan[x * 4 + 1];
                int b = scan[x * 4 + 2];

                if (filter == ColorFilter::Grayscale) {
                    int gray = (r * 299 + g * 587 + b * 114) / 1000;
                    gray = std::clamp(static_cast<int>((gray - 128) * 1.08 + 128), 0, 255);
                    r = g = b = gray;
                } else if (filter == ColorFilter::Sepia) {
                    int sr = (r * 393 + g * 769 + b * 189) / 1000;
                    int sg = (r * 349 + g * 686 + b * 168) / 1000;
                    int sb = (r * 272 + g * 534 + b * 131) / 1000;
                    r = std::clamp((r * 35 + sr * 65) / 100, 0, 255);
                    g = std::clamp((g * 35 + sg * 65) / 100, 0, 255);
                    b = std::clamp((b * 35 + sb * 65) / 100, 0, 255);
                } else if (filter == ColorFilter::Cool) {
                    r = std::clamp(static_cast<int>(r * 0.9 + 5), 0, 255);
                    g = std::clamp(static_cast<int>(g * 1.02), 0, 255);
                    b = std::clamp(static_cast<int>(b * 1.18 + 15), 0, 255);
                } else if (filter == ColorFilter::Warm) {
                    r = std::clamp(static_cast<int>(r * 1.15 + 10), 0, 255);
                    g = std::clamp(static_cast<int>(g * 1.04), 0, 255);
                    b = std::clamp(static_cast<int>(b * 0.88), 0, 255);
                } else if (filter == ColorFilter::Cyber) {
                    r = std::clamp(static_cast<int>((r - 128) * 1.3 + 128 + 30), 0, 255);
                    g = std::clamp(static_cast<int>((g - 128) * 1.2 + 128 - 15), 0, 255);
                    b = std::clamp(static_cast<int>((b - 128) * 1.4 + 128 + 40), 0, 255);
                } else if (filter == ColorFilter::Noir) {
                    int gray = (r * 299 + g * 587 + b * 114) / 1000;
                    gray = std::clamp(static_cast<int>(((gray - 128) * 1.7 + 128) * 0.9), 0, 255);
                    r = g = b = gray;
                } else if (filter == ColorFilter::Vintage) {
                    int sr = (r * 393 + g * 769 + b * 189) / 1000;
                    int sg = (r * 349 + g * 686 + b * 168) / 1000;
                    int sb = (r * 272 + g * 534 + b * 131) / 1000;
                    r = std::clamp((r * 65 + sr * 35) / 100, 0, 255);
                    g = std::clamp((g * 65 + sg * 35) / 100, 0, 255);
                    b = std::clamp(static_cast<int>((b * 65 + sb * 35) / 100 * 0.92), 0, 255);
                } else if (filter == ColorFilter::Invert) {
                    r = 255 - r;
                    g = 255 - g;
                    b = 255 - b;
                }

                scan[x * 4 + 0] = static_cast<uchar>(r);
                scan[x * 4 + 1] = static_cast<uchar>(g);
                scan[x * 4 + 2] = static_cast<uchar>(b);
            }
        }
    }

    return img;
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

    // Many webcams emit full-range "J" JPEG pixel formats; swscale treats
    // these as deprecated and warns on every frame. Normalize to the
    // non-J format and tell swscale the source is full-range explicitly.
    AVPixelFormat srcFmt = static_cast<AVPixelFormat>(m_mjpegFrame->format);
    bool fullRange = false;
    switch (srcFmt) {
    case AV_PIX_FMT_YUVJ420P: srcFmt = AV_PIX_FMT_YUV420P; fullRange = true; break;
    case AV_PIX_FMT_YUVJ422P: srcFmt = AV_PIX_FMT_YUV422P; fullRange = true; break;
    case AV_PIX_FMT_YUVJ444P: srcFmt = AV_PIX_FMT_YUV444P; fullRange = true; break;
    case AV_PIX_FMT_YUVJ440P: srcFmt = AV_PIX_FMT_YUV440P; fullRange = true; break;
    default: break;
    }

    m_mjpegSws = sws_getCachedContext(m_mjpegSws,
                                      w, h, srcFmt,
                                      w, h, AV_PIX_FMT_RGBA,
                                      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_mjpegSws)
        return false;

    if (fullRange) {
        int srcRange, dstRange, brightness, contrast, saturation;
        const int *invTable, *table;
        sws_getColorspaceDetails(m_mjpegSws, const_cast<int **>(&invTable), &srcRange,
                                 const_cast<int **>(&table), &dstRange,
                                 &brightness, &contrast, &saturation);
        sws_setColorspaceDetails(m_mjpegSws, invTable, 1 /*full range*/,
                                 table, dstRange, brightness, contrast, saturation);
    }

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
                d[0] = static_cast<uchar>(std::clamp(r, 0, 255));
                d[1] = static_cast<uchar>(std::clamp(g, 0, 255));
                d[2] = static_cast<uchar>(std::clamp(b, 0, 255));
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
    // Unsupported formats are dropped silently.

    if (!ok)
        return;

    {
        QMutexLocker locker(&m_frameMutex);
        m_pending = decoded;
    }
    update();
}

} // namespace cmi
