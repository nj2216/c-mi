#ifndef CMI_ENCODE_PHOTOENCODER_H
#define CMI_ENCODE_PHOTOENCODER_H

#include <QString>
#include <QSize>
#include <stdint.h>

namespace cmi {

// Exports a single RGBA frame as JPEG or PNG via libavcodec.
class PhotoEncoder {
public:
    enum class Format { JPEG, PNG };

    // pixels: tightly packed RGBA8, size = width*height*4.
    static bool save(const QString &path, const uchar *pixels,
                     QSize size, Format format, QString *errorOut = nullptr);
};

} // namespace cmi

#endif // CMI_ENCODE_PHOTOENCODER_H
