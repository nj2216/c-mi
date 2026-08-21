#ifndef CMI_UI_PREVIEWWIDGET_H
#define CMI_UI_PREVIEWWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QImage>
#include <QMutex>
#include <QSize>

#include <stdint.h>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace cmi {

// Renders V4L2 frames as GL textures. MJPEG frames are decoded via
// libavcodec; YUYV is converted to RGBA on CPU. Decoded frames are staged
// under a mutex and uploaded once in paintGL to avoid extra copies.
class PreviewWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget() override;

public slots:
    // data is only valid for the duration of the call; the frame is
    // decoded/copied into the staging buffer synchronously if accepted.
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

    ::AVCodecContext *m_mjpegCtx = nullptr;
    ::AVFrame *m_mjpegFrame = nullptr;
    ::AVPacket *m_mjpegPkt = nullptr;
    ::SwsContext *m_mjpegSws = nullptr;

public:
    QImage lastFrame() const;
};

} // namespace cmi

#endif // CMI_UI_PREVIEWWIDGET_H
