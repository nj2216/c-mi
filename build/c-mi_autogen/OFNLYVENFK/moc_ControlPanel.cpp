/****************************************************************************
** Meta object code from reading C++ file 'ControlPanel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/v4l2/ControlPanel.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ControlPanel.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_cmi__ControlPanel_t {
    uint offsetsAndSizes[14];
    char stringdata0[18];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[9];
    char stringdata4[3];
    char stringdata5[8];
    char stringdata6[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_cmi__ControlPanel_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_cmi__ControlPanel_t qt_meta_stringdata_cmi__ControlPanel = {
    {
        QT_MOC_LITERAL(0, 17),  // "cmi::ControlPanel"
        QT_MOC_LITERAL(18, 14),  // "controlChanged"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 8),  // "uint32_t"
        QT_MOC_LITERAL(43, 2),  // "id"
        QT_MOC_LITERAL(46, 7),  // "int32_t"
        QT_MOC_LITERAL(54, 5)   // "value"
    },
    "cmi::ControlPanel",
    "controlChanged",
    "",
    "uint32_t",
    "id",
    "int32_t",
    "value"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_cmi__ControlPanel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject cmi::ControlPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_cmi__ControlPanel.offsetsAndSizes,
    qt_meta_data_cmi__ControlPanel,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_cmi__ControlPanel_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ControlPanel, std::true_type>,
        // method 'controlChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>
    >,
    nullptr
} };

void cmi::ControlPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ControlPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->controlChanged((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ControlPanel::*)(uint32_t , int32_t );
            if (_t _q_method = &ControlPanel::controlChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *cmi::ControlPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *cmi::ControlPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_cmi__ControlPanel.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int cmi::ControlPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void cmi::ControlPanel::controlChanged(uint32_t _t1, int32_t _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
