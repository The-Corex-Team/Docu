#pragma once

#include <QColor>
#include <QString>

namespace DocuStyle {

inline QColor documentCanvasColor()
{
    return QColor(QStringLiteral("#d5d8dc"));
}

inline QColor pageColor()
{
    return Qt::white;
}

inline QColor pageBorderColor()
{
    return QColor(QStringLiteral("#a7a7a7"));
}

inline QColor pageShadowColor()
{
    return QColor(0, 0, 0, 45);
}

inline QColor imageSelectionColor()
{
    return QColor(QStringLiteral("#1769aa"));
}

inline QString applicationStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow#mainWindow {
            background: #eceff1;
        }

        QTabWidget#ribbon::pane {
            border: 0;
            border-bottom: 1px solid #b8bec4;
            background: #f4f5f6;
        }
        QTabWidget#ribbon QTabBar::tab {
            padding: 6px 13px;
            border: 1px solid transparent;
            border-bottom: 0;
            background: transparent;
        }
        QTabWidget#ribbon QTabBar::tab:selected {
            border-color: #b8bec4;
            background: #f4f5f6;
        }
        QTabWidget#ribbon QTabBar::tab:hover:!selected {
            background: #e7eaed;
        }

        QPushButton, QToolButton {
            padding: 5px 9px;
            border: 1px solid #b8bec4;
            border-radius: 3px;
            background: #f8f9fa;
        }
        QPushButton:hover, QToolButton:hover {
            border-color: #8795a1;
            background: #e8eef3;
        }
        QPushButton:pressed, QToolButton:pressed,
        QToolButton:checked {
            background: #d5e3f0;
        }
        QPushButton:disabled, QToolButton:disabled {
            color: #92999f;
            background: #eceff1;
        }

        QComboBox, QDoubleSpinBox {
            min-height: 21px;
            padding: 2px 5px;
            border: 1px solid #b8bec4;
            border-radius: 2px;
            background: white;
        }
        QComboBox:focus, QDoubleSpinBox:focus {
            border-color: #5689b5;
        }
        QStatusBar {
            border-top: 1px solid #c5cbd0;
            background: #f1f3f4;
        }
    )");
}

inline QString ribbonSectionStyleSheet()
{
    return QStringLiteral(
        "QWidget#ribbonSection { border-right: 1px solid #c8c8c8; }"
        "QWidget#ribbonSection QLabel { color: #40484f; }");
}

inline QString pageEditorStyleSheet()
{
    return QStringLiteral(
        "QTextEdit { background: white; border: none; "
        "selection-background-color: #8ab4e8; }");
}

} // namespace DocuStyle
