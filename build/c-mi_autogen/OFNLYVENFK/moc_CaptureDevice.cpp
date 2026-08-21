/****************************************************************************
** Meta object code from reading C++ file 'CaptureDevice.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/v4l2/CaptureDevice.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CaptureDevice.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_cmi__CaptureDevice_t {
    uint offsetsAndSizes[36];
    char stringdata0[19];
    char stringdata1[11];
    char stringdata2[1];
    char stringdata3[13];
    char stringdata4[5];
    char stringdata5[6];
    char stringdata6[5];
    char stringdata7[9];
    char stringdata8[7];
    char stringdata9[14];
    char stringdata10[8];
    char stringdata11[5];
    char stringdata12[5];
    char stringdata13[6];
    char stringdata14[6];
    char stringdata15[11];
    char stringdata16[5];
    char stringdata17[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_cmi__CaptureDevice_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_cmi__CaptureDevice_t qt_meta_stringdata_cmi__CaptureDevice = {
    {
        QT_MOC_LITERAL(0, 18),  // "cmi::CaptureDevice"
        QT_MOC_LITERAL(19, 10),  // "frameReady"
        QT_MOC_LITERAL(30, 0),  // ""
        QT_MOC_LITERAL(31, 12),  // "const uchar*"
        QT_MOC_LITERAL(44, 4),  // "data"
        QT_MOC_LITERAL(49, 5),  // "bytes"
        QT_MOC_LITERAL(55, 4),  // "size"
        QT_MOC_LITERAL(60, 8),  // "uint32_t"
        QT_MOC_LITERAL(69, 6),  // "pixFmt"
        QT_MOC_LITERAL(76, 13),  // "errorOccurred"
        QT_MOC_LITERAL(90, 7),  // "message"
        QT_MOC_LITERAL(98, 4),  // "open"
        QT_MOC_LITERAL(103, 4),  // "node"
        QT_MOC_LITERAL(108, 5),  // "close"
        QT_MOC_LITERAL(114, 5),  // "start"
        QT_MOC_LITERAL(120, 10),  // "resolution"
        QT_MOC_LITERAL(131, 4),  // "stop"
        QT_MOC_LITERAL(136, 9)   // "grabFrame"
    },
    "cmi::CaptureDevice",
    "frameReady",
    "",
    "const uchar*",
    "data",
    "bytes",
    "size",
    "uint32_t",
    "pixFmt",
    "errorOccurred",
    "message",
    "open",
    "node",
    "close",
    "start",
    "resolution",
    "stop",
    "grabFrame"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_cmi__CaptureDevice[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    4,   62,    2, 0x06,    1 /* Public */,
       9,    1,   71,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      11,    1,   74,    2, 0x0a,    8 /* Public */,
      13,    0,   77,    2, 0x0a,   10 /* Public */,
      14,    2,   78,    2, 0x0a,   11 /* Public */,
      14,    1,   83,    2, 0x2a,   14 /* Public | MethodCloned */,
      16,    0,   86,    2, 0x0a,   16 /* Public */,
      17,    0,   87,    2, 0x0a,   17 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int, QMetaType::QSize, 0x80000000 | 7,    4,    5,    6,    8,
    QMetaType::Void, QMetaType::QString,   10,

 // slots: parameters
    QMetaType::Bool, QMetaType::QString,   12,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QSize, 0x80000000 | 7,   15,    8,
    QMetaType::Bool, QMetaType::QSize,   15,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject cmi::CaptureDevice::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_cmi__CaptureDevice.offsetsAndSizes,
    qt_meta_data_cmi__CaptureDevice,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_cmi__CaptureDevice_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CaptureDevice, std::true_type>,
        // method 'frameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const uchar *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSize, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'open'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'close'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'start'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QSize &, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'start'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QSize &, std::false_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'grabFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void cmi::CaptureDevice::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CaptureDevice *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->frameReady((*reinterpret_cast< std::add_pointer_t<const uchar*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QSize>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[4]))); break;
        case 1: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: { bool _r = _t->open((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->close(); break;
        case 4: { bool _r = _t->start((*reinterpret_cast< std::add_pointer_t<QSize>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 5: { bool _r = _t->start((*reinterpret_cast< std::add_pointer_t<QSize>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->stop(); break;
        case 7: _t->grabFrame(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CaptureDevice::*)(const uchar * , int , QSize , uint32_t );
            if (_t _q_method = &CaptureDevice::frameReady; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CaptureDevice::*)(const QString & );
            if (_t _q_method = &CaptureDevice::errorOccurred; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *cmi::CaptureDevice::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *cmi::CaptureDevice::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_cmi__CaptureDevice.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int cmi::CaptureDevice::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void cmi::CaptureDevice::frameReady(const uchar * _t1, int _t2, QSize _t3, uint32_t _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void cmi::CaptureDevice::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
