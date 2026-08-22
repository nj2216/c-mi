#ifndef CMI_UI_PREVIEWWIDGET_H
#define CMI_UI_PREVIEWWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QImage>
#include <QMutex>
#include <QSize>
#include <QTimer>

#include <stdint.h>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace cmi {

enum class ColorFilter {
    None = 0,
    Grayscale = 1,
    Sepia = 2,
    Cool = 3,
    Warm = 4,
    Cyber = 5,
    Noir = 6,
    Vintage = 7,
    Invert = 8
};

enum class AspectRatioMode {
    Fit = 0,
    Ratio16_9 = 1,
    Ratio4_3 = 2,
    Ratio1_1 = 3
};

// Renders V4L2 frames as GL textures. MJPEG frames are decoded via
// libavcodec; YUYV is converted to RGBA on CPU. Decoded frames are staged
// under a mutex and uploaded once in paintGL to avoid extra copies.
class PreviewWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget() override;

    void setFilter(ColorFilter filter);
    ColorFilter filter() const { return m_filter; }

    void setAspectRatioMode(AspectRatioMode mode);
    AspectRatioMode aspectRatioMode() const { return m_arMode; }

    void setMirrored(bool mirrored);
    bool isMirrored() const { return m_mirrored; }

    void setShowGrid(bool show);
    bool showGrid() const { return m_showGrid; }

    void triggerFlash();

    void setQualityResolution(const QSize &targetRes);
    QSize qualityResolution() const { return m_targetQuality; }

    QImage processedLastFrame() const;
    QImage lastRawFrame() const;
    bool hasFrame() const;

    static QImage applyEffectsToImage(const QImage &src, ColorFilter filter, bool mirror, AspectRatioMode ar, const QSize &targetRes = QSize());

public slots:
    void presentFrame(const uchar *data, int bytes, QSize size, uint32_t pixFmt);
    void clearFrame();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    bool decodeMjpeg(const uchar *data, int bytes, QImage &out);
    static QImage yuyvToRgba(const uchar *data, QSize size);

    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLTexture *m_texture = nullptr;
    GLuint m_vbo = 0;
    GLuint m_vao = 0;

    QMutex m_frameMutex;
    QImage m_pending;     // staged RGBA frame awaiting texture upload
    QImage m_lastFrame;   // latest uploaded frame (photo/record source)
    QSize m_frameSize;
    bool m_hasFrame = false;

    ColorFilter m_filter = ColorFilter::None;
    AspectRatioMode m_arMode = AspectRatioMode::Fit;
    QSize m_targetQuality = QSize(1280, 720);
    bool m_mirrored = true;
    bool m_showGrid = false;
    float m_flashIntensity = 0.0f;
    QTimer *m_flashTimer = nullptr;

    ::AVCodecContext *m_mjpegCtx = nullptr;
    ::AVFrame *m_mjpegFrame = nullptr;
    ::AVPacket *m_mjpegPkt = nullptr;
    ::SwsContext *m_mjpegSws = nullptr;
};

} // namespace cmi

#endif // CMI_UI_PREVIEWWIDGET_H
