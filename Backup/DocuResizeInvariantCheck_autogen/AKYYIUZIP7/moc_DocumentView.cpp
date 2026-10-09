/****************************************************************************
** Meta object code from reading C++ file 'DocumentView.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/gui/Editor/DocumentView.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DocumentView.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN12DocumentViewE_t {};
} // unnamed namespace

template <> constexpr inline auto DocumentView::qt_create_metaobjectdata<qt_meta_tag_ZN12DocumentViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DocumentView",
        "activeEditorChanged",
        "",
        "QTextEdit*",
        "editor",
        "activeEditorStateChanged",
        "pageCountChanged",
        "count",
        "zoomChanged",
        "percentage",
        "imageResizeStarted",
        "documentPosition",
        "imageResizeChanged",
        "QSizeF",
        "size",
        "imageResizeFinished"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'activeEditorChanged'
        QtMocHelpers::SignalData<void(QTextEdit *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'activeEditorStateChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pageCountChanged'
        QtMocHelpers::SignalData<void(int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 },
        }}),
        // Signal 'zoomChanged'
        QtMocHelpers::SignalData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 },
        }}),
        // Signal 'imageResizeStarted'
        QtMocHelpers::SignalData<void(int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 },
        }}),
        // Signal 'imageResizeChanged'
        QtMocHelpers::SignalData<void(int, const QSizeF &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 }, { 0x80000000 | 13, 14 },
        }}),
        // Signal 'imageResizeFinished'
        QtMocHelpers::SignalData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DocumentView, qt_meta_tag_ZN12DocumentViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DocumentView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12DocumentViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12DocumentViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12DocumentViewE_t>.metaTypes,
    nullptr
} };

void DocumentView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DocumentView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->activeEditorChanged((*reinterpret_cast<std::add_pointer_t<QTextEdit*>>(_a[1]))); break;
        case 1: _t->activeEditorStateChanged(); break;
        case 2: _t->pageCountChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->zoomChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->imageResizeStarted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->imageResizeChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QSizeF>>(_a[2]))); break;
        case 6: _t->imageResizeFinished(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DocumentView::*)(QTextEdit * )>(_a, &DocumentView::activeEditorChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentView::*)()>(_a, &DocumentView::activeEditorStateChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentView::*)(int )>(_a, &DocumentView::pageCountChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentView::*)(int )>(_a, &DocumentView::zoomChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentView::*)(int )>(_a, &DocumentView::imageResizeStarted, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentView::*)(int , const QSizeF & )>(_a, &DocumentView::imageResizeChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentView::*)()>(_a, &DocumentView::imageResizeFinished, 6))
            return;
    }
}

const QMetaObject *DocumentView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DocumentView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12DocumentViewE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int DocumentView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void DocumentView::activeEditorChanged(QTextEdit * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void DocumentView::activeEditorStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void DocumentView::pageCountChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void DocumentView::zoomChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void DocumentView::imageResizeStarted(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void DocumentView::imageResizeChanged(int _t1, const QSizeF & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void DocumentView::imageResizeFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
