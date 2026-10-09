#pragma once

#include <QHash>
#include <QTabWidget>

class QAction;
class QComboBox;
class QDoubleSpinBox;
class QFontComboBox;
class QPushButton;
class QToolButton;

class Ribbon final : public QTabWidget
{
public:
    explicit Ribbon(QWidget *parent = nullptr);

    void bindAction(const QString &id, QAction *action);
    QFontComboBox *fontFamilyCombo() const;
    QComboBox *fontSizeCombo() const;
    QComboBox *zoomCombo() const;
    QPushButton *zoomInButton() const;
    QPushButton *zoomOutButton() const;
    QPushButton *fitPageButton() const;
    QPushButton *fitWidthButton() const;
    QDoubleSpinBox *imageWidthSpinBox() const;
    QDoubleSpinBox *imageHeightSpinBox() const;
    QPushButton *applyImageSizeButton() const;

private:
    QWidget *createFileTab();
    QWidget *createHomeTab();
    QWidget *createViewTab();

    QHash<QString, QToolButton *> m_actionButtons;
    QFontComboBox *m_fontFamilyCombo = nullptr;
    QComboBox *m_fontSizeCombo = nullptr;
    QComboBox *m_zoomCombo = nullptr;
    QPushButton *m_zoomInButton = nullptr;
    QPushButton *m_zoomOutButton = nullptr;
    QPushButton *m_fitPageButton = nullptr;
    QPushButton *m_fitWidthButton = nullptr;
    QDoubleSpinBox *m_imageWidthSpinBox = nullptr;
    QDoubleSpinBox *m_imageHeightSpinBox = nullptr;
    QPushButton *m_applyImageSizeButton = nullptr;
};
