/****************************************************************************
** Meta object code from reading C++ file 'VideoEncoder.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/encode/VideoEncoder.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'VideoEncoder.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_cmi__VideoEncoder_t {
    uint offsetsAndSizes[32];
    char stringdata0[18];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[8];
    char stringdata4[17];
    char stringdata5[5];
    char stringdata6[17];
    char stringdata7[6];
    char stringdata8[5];
    char stringdata9[4];
    char stringdata10[10];
    char stringdata11[5];
    char stringdata12[11];
    char stringdata13[13];
    char stringdata14[7];
    char stringdata15[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_cmi__VideoEncoder_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_cmi__VideoEncoder_t qt_meta_stringdata_cmi__VideoEncoder = {
    {
        QT_MOC_LITERAL(0, 17),  // "cmi::VideoEncoder"
        QT_MOC_LITERAL(18, 13),  // "errorOccurred"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 7),  // "message"
        QT_MOC_LITERAL(41, 16),  // "recordingStarted"
        QT_MOC_LITERAL(58, 4),  // "path"
        QT_MOC_LITERAL(63, 16),  // "recordingStopped"
        QT_MOC_LITERAL(80, 5),  // "start"
        QT_MOC_LITERAL(86, 4),  // "size"
        QT_MOC_LITERAL(91, 3),  // "fps"
        QT_MOC_LITERAL(95, 9),  // "withAudio"
        QT_MOC_LITERAL(105, 4),  // "stop"
        QT_MOC_LITERAL(110, 10),  // "writeFrame"
        QT_MOC_LITERAL(121, 12),  // "const uchar*"
        QT_MOC_LITERAL(134, 6),  // "pixels"
        QT_MOC_LITERAL(141, 5)   // "bytes"
    },
    "cmi::VideoEncoder",
    "errorOccurred",
    "",
    "message",
    "recordingStarted",
    "path",
    "recordingStopped",
    "start",
    "size",
    "fps",
    "withAudio",
    "stop",
    "writeFrame",
    "const uchar*",
    "pixels",
    "bytes"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_cmi__VideoEncoder[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       4,    1,   53,    2, 0x06,    3 /* Public */,
       6,    1,   56,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    4,   59,    2, 0x0a,    7 /* Public */,
      11,    0,   68,    2, 0x0a,   12 /* Public */,
      12,    3,   69,    2, 0x0a,   13 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Bool, QMetaType::QString, QMetaType::QSize, QMetaType::Int, QMetaType::Bool,    5,    8,    9,   10,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 13, QMetaType::Int, QMetaType::QSize,   14,   15,    8,

       0        // eod
};

Q_CONSTINIT const QMetaObject cmi::VideoEncoder::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_cmi__VideoEncoder.offsetsAndSizes,
    qt_meta_data_cmi__VideoEncoder,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_cmi__VideoEncoder_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<VideoEncoder, std::true_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'recordingStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'recordingStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'start'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSize, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'writeFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const uchar *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSize, std::false_type>
    >,
    nullptr
} };

void cmi::VideoEncoder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VideoEncoder *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->recordingStarted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->recordingStopped((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: { bool _r = _t->start((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QSize>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->stop(); break;
        case 5: _t->writeFrame((*reinterpret_cast< std::add_pointer_t<const uchar*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QSize>>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VideoEncoder::*)(const QString & );
            if (_t _q_method = &VideoEncoder::errorOccurred; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (VideoEncoder::*)(const QString & );
            if (_t _q_method = &VideoEncoder::recordingStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (VideoEncoder::*)(const QString & );
            if (_t _q_method = &VideoEncoder::recordingStopped; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *cmi::VideoEncoder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *cmi::VideoEncoder::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_cmi__VideoEncoder.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int cmi::VideoEncoder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void cmi::VideoEncoder::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void cmi::VideoEncoder::recordingStarted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void cmi::VideoEncoder::recordingStopped(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
