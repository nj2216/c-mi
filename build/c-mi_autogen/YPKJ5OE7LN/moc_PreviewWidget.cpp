/****************************************************************************
** Meta object code from reading C++ file 'PreviewWidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/PreviewWidget.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PreviewWidget.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_cmi__PreviewWidget_t {
    uint offsetsAndSizes[24];
    char stringdata0[19];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[6];
    char stringdata4[13];
    char stringdata5[13];
    char stringdata6[5];
    char stringdata7[6];
    char stringdata8[5];
    char stringdata9[9];
    char stringdata10[7];
    char stringdata11[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_cmi__PreviewWidget_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_cmi__PreviewWidget_t qt_meta_stringdata_cmi__PreviewWidget = {
    {
        QT_MOC_LITERAL(0, 18),  // "cmi::PreviewWidget"
        QT_MOC_LITERAL(19, 13),  // "frameRendered"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 5),  // "frame"
        QT_MOC_LITERAL(40, 12),  // "presentFrame"
        QT_MOC_LITERAL(53, 12),  // "const uchar*"
        QT_MOC_LITERAL(66, 4),  // "data"
        QT_MOC_LITERAL(71, 5),  // "bytes"
        QT_MOC_LITERAL(77, 4),  // "size"
        QT_MOC_LITERAL(82, 8),  // "uint32_t"
        QT_MOC_LITERAL(91, 6),  // "pixFmt"
        QT_MOC_LITERAL(98, 10)   // "clearFrame"
    },
    "cmi::PreviewWidget",
    "frameRendered",
    "",
    "frame",
    "presentFrame",
    "const uchar*",
    "data",
    "bytes",
    "size",
    "uint32_t",
    "pixFmt",
    "clearFrame"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_cmi__PreviewWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   32,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    4,   35,    2, 0x0a,    3 /* Public */,
      11,    0,   44,    2, 0x0a,    8 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QImage,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5, QMetaType::Int, QMetaType::QSize, 0x80000000 | 9,    6,    7,    8,   10,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject cmi::PreviewWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_meta_stringdata_cmi__PreviewWidget.offsetsAndSizes,
    qt_meta_data_cmi__PreviewWidget,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_cmi__PreviewWidget_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PreviewWidget, std::true_type>,
        // method 'frameRendered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'presentFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const uchar *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSize, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'clearFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void cmi::PreviewWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PreviewWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->frameRendered((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 1: _t->presentFrame((*reinterpret_cast< std::add_pointer_t<const uchar*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QSize>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[4]))); break;
        case 2: _t->clearFrame(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PreviewWidget::*)(const QImage & );
            if (_t _q_method = &PreviewWidget::frameRendered; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *cmi::PreviewWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *cmi::PreviewWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_cmi__PreviewWidget.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QOpenGLExtraFunctions"))
        return static_cast< QOpenGLExtraFunctions*>(this);
    return QOpenGLWidget::qt_metacast(_clname);
}

int cmi::PreviewWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QOpenGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void cmi::PreviewWidget::frameRendered(const QImage & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
