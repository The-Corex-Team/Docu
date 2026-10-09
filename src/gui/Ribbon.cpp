#include "Ribbon.h"

#include "Style.h"

#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QWidget *makeSection(const QString &title, QWidget *contents)
{
    auto *section = new QWidget;
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(3);
    layout->addWidget(contents, 1);

    auto *caption = new QLabel(title, section);
    caption->setAlignment(Qt::AlignCenter);
    layout->addWidget(caption);
    section->setObjectName(QStringLiteral("ribbonSection"));
    section->setStyleSheet(DocuStyle::ribbonSectionStyleSheet());
    return section;
}

} // namespace

Ribbon::Ribbon(QWidget *parent)
    : QTabWidget(parent)
{
    setObjectName(QStringLiteral("ribbon"));
    setDocumentMode(false);
    setMovable(false);
    addTab(createFileTab(), QStringLiteral("File"));
    addTab(createHomeTab(), QStringLiteral("Home"));
    addTab(createViewTab(), QStringLiteral("View"));
    setCurrentIndex(1);
}

void Ribbon::bindAction(const QString &id, QAction *action)
{
    if (QToolButton *button = m_actionButtons.value(id))
        button->setDefaultAction(action);
}

QFontComboBox *Ribbon::fontFamilyCombo() const
{
    return m_fontFamilyCombo;
}

QComboBox *Ribbon::fontSizeCombo() const
{
    return m_fontSizeCombo;
}

QComboBox *Ribbon::zoomCombo() const
{
    return m_zoomCombo;
}

QPushButton *Ribbon::zoomInButton() const
{
    return m_zoomInButton;
}

QPushButton *Ribbon::zoomOutButton() const
{
    return m_zoomOutButton;
}

QPushButton *Ribbon::fitPageButton() const
{
    return m_fitPageButton;
}

QPushButton *Ribbon::fitWidthButton() const
{
    return m_fitWidthButton;
}

QDoubleSpinBox *Ribbon::imageWidthSpinBox() const
{
    return m_imageWidthSpinBox;
}

QDoubleSpinBox *Ribbon::imageHeightSpinBox() const
{
    return m_imageHeightSpinBox;
}

QPushButton *Ribbon::applyImageSizeButton() const
{
    return m_applyImageSizeButton;
}

QWidget *Ribbon::createFileTab()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setAlignment(Qt::AlignLeft);
    const QList<QPair<QString, QString>> fileActions{
        {QStringLiteral("new"), QStringLiteral("New")},
        {QStringLiteral("open"), QStringLiteral("Open")},
        {QStringLiteral("save"), QStringLiteral("Save")},
        {QStringLiteral("saveAs"), QStringLiteral("Save As")}
    };
    for (const auto &action : fileActions) {
        auto *button = new QToolButton(page);
        button->setText(action.second);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        m_actionButtons.insert(action.first, button);
        layout->addWidget(button);
    }
    layout->addStretch();
    return page;
}

QWidget *Ribbon::createHomeTab()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(0);

    const auto addSectionActions = [this, layout](const QString &title,
                                                  const QList<QPair<QString, QString>> &items) {
        auto *contents = new QWidget;
        auto *row = new QHBoxLayout(contents);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);
        for (const auto &item : items) {
            auto *button = new QToolButton(contents);
            button->setText(item.second);
            button->setToolButtonStyle(Qt::ToolButtonTextOnly);
            button->setMinimumHeight(30);
            button->setCheckable(item.first == QStringLiteral("bold")
                                 || item.first == QStringLiteral("italic")
                                 || item.first == QStringLiteral("underline")
                                 || item.first.startsWith(QStringLiteral("align")));
            m_actionButtons.insert(item.first, button);
            row->addWidget(button);
        }
        layout->addWidget(makeSection(title, contents));
    };

    addSectionActions(QStringLiteral("Clipboard"),
                      {{QStringLiteral("paste"), QStringLiteral("Paste")},
                       {QStringLiteral("cut"), QStringLiteral("Cut")},
                       {QStringLiteral("copy"), QStringLiteral("Copy")}});

    auto *fontContents = new QWidget;
    auto *fontLayout = new QVBoxLayout(fontContents);
    fontLayout->setContentsMargins(0, 0, 0, 0);
    fontLayout->setSpacing(3);
    auto *fontRow = new QHBoxLayout;
    fontRow->setContentsMargins(0, 0, 0, 0);
    m_fontFamilyCombo = new QFontComboBox(fontContents);
    m_fontFamilyCombo->setMinimumWidth(150);
    m_fontSizeCombo = new QComboBox(fontContents);
    m_fontSizeCombo->setEditable(true);
    m_fontSizeCombo->addItems({QStringLiteral("8"), QStringLiteral("9"),
                               QStringLiteral("10"), QStringLiteral("11"),
                               QStringLiteral("12"), QStringLiteral("14"),
                               QStringLiteral("16"), QStringLiteral("18"),
                               QStringLiteral("20"), QStringLiteral("24"),
                               QStringLiteral("28"), QStringLiteral("36"),
                               QStringLiteral("48"), QStringLiteral("72")});
    m_fontSizeCombo->setCurrentText(QStringLiteral("11"));
    fontRow->addWidget(m_fontFamilyCombo, 1);
    fontRow->addWidget(m_fontSizeCombo);
    fontLayout->addLayout(fontRow);

    auto *formatRow = new QHBoxLayout;
    formatRow->setContentsMargins(0, 0, 0, 0);
    for (const auto &item : QList<QPair<QString, QString>>{
             {QStringLiteral("bold"), QStringLiteral("B")},
             {QStringLiteral("italic"), QStringLiteral("I")},
             {QStringLiteral("underline"), QStringLiteral("U")}}) {
        auto *button = new QToolButton(fontContents);
        button->setText(item.second);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setCheckable(true);
        m_actionButtons.insert(item.first, button);
        formatRow->addWidget(button);
    }
    formatRow->addStretch();
    fontLayout->addLayout(formatRow);
    layout->addWidget(makeSection(QStringLiteral("Font"), fontContents));

    addSectionActions(QStringLiteral("Paragraph"),
                      {{QStringLiteral("alignLeft"), QStringLiteral("Left")},
                       {QStringLiteral("alignCenter"), QStringLiteral("Center")},
                       {QStringLiteral("alignRight"), QStringLiteral("Right")},
                       {QStringLiteral("alignJustify"), QStringLiteral("Justify")}});

    auto *insertContents = new QWidget;
    auto *insertLayout = new QHBoxLayout(insertContents);
    insertLayout->setContentsMargins(0, 0, 0, 0);
    auto *pictureButton = new QToolButton(insertContents);
    pictureButton->setText(QStringLiteral("Picture"));
    pictureButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    pictureButton->setMinimumHeight(30);
    m_actionButtons.insert(QStringLiteral("insertImage"), pictureButton);
    insertLayout->addWidget(pictureButton);
    auto *imageSizeLayout = new QVBoxLayout;
    imageSizeLayout->setContentsMargins(6, 0, 0, 0);
    imageSizeLayout->setSpacing(2);
    const auto createImageSizeControl = [insertContents, &imageSizeLayout](
                                            const QString &labelText) {
        auto *row = new QWidget(insertContents);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(4);
        auto *label = new QLabel(labelText, row);
        auto *spinBox = new QDoubleSpinBox(row);
        spinBox->setRange(0.1, 1000.0);
        spinBox->setDecimals(1);
        spinBox->setSuffix(QStringLiteral(" mm"));
        spinBox->setEnabled(false);
        rowLayout->addWidget(label);
        rowLayout->addWidget(spinBox);
        imageSizeLayout->addWidget(row);
        return spinBox;
    };
    m_imageWidthSpinBox = createImageSizeControl(QStringLiteral("Width"));
    m_imageHeightSpinBox = createImageSizeControl(QStringLiteral("Height"));
    m_applyImageSizeButton = new QPushButton(QStringLiteral("Apply size"), insertContents);
    m_applyImageSizeButton->setEnabled(false);
    imageSizeLayout->addWidget(m_applyImageSizeButton);
    insertLayout->addLayout(imageSizeLayout);
    layout->addWidget(makeSection(QStringLiteral("Insert"), insertContents));
    layout->addStretch();
    return page;
}

QWidget *Ribbon::createViewTab()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setAlignment(Qt::AlignLeft);
    m_zoomOutButton = new QPushButton(QStringLiteral("Zoom Out"), page);
    m_zoomInButton = new QPushButton(QStringLiteral("Zoom In"), page);
    m_fitPageButton = new QPushButton(QStringLiteral("Fit Page"), page);
    m_fitWidthButton = new QPushButton(QStringLiteral("Fit Width"), page);
    m_zoomCombo = new QComboBox(page);
    m_zoomCombo->setEditable(true);
    m_zoomCombo->addItems({QStringLiteral("25%"), QStringLiteral("50%"),
                           QStringLiteral("60%"), QStringLiteral("75%"),
                           QStringLiteral("100%"), QStringLiteral("125%"),
                           QStringLiteral("150%"), QStringLiteral("200%")});
    m_zoomCombo->setCurrentText(QStringLiteral("60%"));
    layout->addWidget(m_zoomOutButton);
    layout->addWidget(m_zoomInButton);
    layout->addWidget(m_fitPageButton);
    layout->addWidget(m_fitWidthButton);
    layout->addWidget(m_zoomCombo);
    layout->addStretch();
    return page;
}
