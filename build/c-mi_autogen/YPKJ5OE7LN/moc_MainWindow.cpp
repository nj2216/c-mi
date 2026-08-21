/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/MainWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_cmi__MainWindow_t {
    uint offsetsAndSizes[32];
    char stringdata0[16];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[17];
    char stringdata4[6];
    char stringdata5[13];
    char stringdata6[13];
    char stringdata7[5];
    char stringdata8[6];
    char stringdata9[5];
    char stringdata10[9];
    char stringdata11[7];
    char stringdata12[8];
    char stringdata13[16];
    char stringdata14[15];
    char stringdata15[8];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_cmi__MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_cmi__MainWindow_t qt_meta_stringdata_cmi__MainWindow = {
    {
        QT_MOC_LITERAL(0, 15),  // "cmi::MainWindow"
        QT_MOC_LITERAL(16, 16),  // "onDevicesChanged"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 16),  // "onDeviceSelected"
        QT_MOC_LITERAL(51, 5),  // "index"
        QT_MOC_LITERAL(57, 12),  // "onFrameReady"
        QT_MOC_LITERAL(70, 12),  // "const uchar*"
        QT_MOC_LITERAL(83, 4),  // "data"
        QT_MOC_LITERAL(88, 5),  // "bytes"
        QT_MOC_LITERAL(94, 4),  // "size"
        QT_MOC_LITERAL(99, 8),  // "uint32_t"
        QT_MOC_LITERAL(108, 6),  // "pixFmt"
        QT_MOC_LITERAL(115, 7),  // "onPhoto"
        QT_MOC_LITERAL(123, 15),  // "onRecordToggled"
        QT_MOC_LITERAL(139, 14),  // "onCaptureError"
        QT_MOC_LITERAL(154, 7)   // "message"
    },
    "cmi::MainWindow",
    "onDevicesChanged",
    "",
    "onDeviceSelected",
    "index",
    "onFrameReady",
    "const uchar*",
    "data",
    "bytes",
    "size",
    "uint32_t",
    "pixFmt",
    "onPhoto",
    "onRecordToggled",
    "onCaptureError",
    "message"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_cmi__MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x08,    1 /* Private */,
       3,    1,   51,    2, 0x08,    2 /* Private */,
       5,    4,   54,    2, 0x08,    4 /* Private */,
      12,    0,   63,    2, 0x08,    9 /* Private */,
      13,    0,   64,    2, 0x08,   10 /* Private */,
      14,    1,   65,    2, 0x08,   11 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Int, QMetaType::QSize, 0x80000000 | 10,    7,    8,    9,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   15,

       0        // eod
};

Q_CONSTINIT const QMetaObject cmi::MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_cmi__MainWindow.offsetsAndSizes,
    qt_meta_data_cmi__MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_cmi__MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'onDevicesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeviceSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onFrameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const uchar *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSize, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'onPhoto'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRecordToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCaptureError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void cmi::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onDevicesChanged(); break;
        case 1: _t->onDeviceSelected((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->onFrameReady((*reinterpret_cast< std::add_pointer_t<const uchar*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QSize>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[4]))); break;
        case 3: _t->onPhoto(); break;
        case 4: _t->onRecordToggled(); break;
        case 5: _t->onCaptureError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *cmi::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *cmi::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_cmi__MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int cmi::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
