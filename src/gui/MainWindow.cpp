#include "MainWindow.h"

#include "Editor/DocumentView.h"
#include "Ribbon.h"
#include "document/Document.h"
#include "document/DocumentController.h"
#include "io/DocumentFile.h"
#include "io/ImageLoader.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextImageFormat>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_document(new Document(this)),
      m_controller(nullptr),
      m_ribbon(new Ribbon(this)),
      m_documentView(new DocumentView(m_document, this))
{
    setObjectName(QStringLiteral("mainWindow"));
    updateWindowTitle();
    resize(1100, 800);
    setMinimumSize(720, 520);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_ribbon);
    layout->addWidget(m_documentView, 1);
    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("Ready"));

    m_controller = new DocumentController(m_document, m_documentView);

    const auto createAction = [this](const QString &id, const QString &text,
                                     bool checkable = false) {
        auto *action = new QAction(text, this);
        action->setCheckable(checkable);
        m_actions.insert(id, action);
        m_ribbon->bindAction(id, action);
        return action;
    };

    QAction *pasteAction = createAction(QStringLiteral("paste"), QStringLiteral("Paste"));
    QAction *cutAction = createAction(QStringLiteral("cut"), QStringLiteral("Cut"));
    QAction *copyAction = createAction(QStringLiteral("copy"), QStringLiteral("Copy"));
    QAction *boldAction = createAction(QStringLiteral("bold"), QStringLiteral("Bold"), true);
    QAction *italicAction = createAction(QStringLiteral("italic"), QStringLiteral("Italic"), true);
    QAction *underlineAction =
        createAction(QStringLiteral("underline"), QStringLiteral("Underline"), true);
    QAction *alignLeftAction =
        createAction(QStringLiteral("alignLeft"), QStringLiteral("Align Left"), true);
    QAction *alignCenterAction =
        createAction(QStringLiteral("alignCenter"), QStringLiteral("Align Center"), true);
    QAction *alignRightAction =
        createAction(QStringLiteral("alignRight"), QStringLiteral("Align Right"), true);
    QAction *alignJustifyAction =
        createAction(QStringLiteral("alignJustify"), QStringLiteral("Justify"), true);
    QAction *insertImageAction =
        createAction(QStringLiteral("insertImage"), QStringLiteral("Picture"));
    insertImageAction->setToolTip(QStringLiteral("Insert a picture from a file"));
    QAction *newAction = createAction(QStringLiteral("new"), QStringLiteral("New"));
    QAction *openAction = createAction(QStringLiteral("open"), QStringLiteral("Open"));
    QAction *saveAction = createAction(QStringLiteral("save"), QStringLiteral("Save"));
    QAction *saveAsAction =
        createAction(QStringLiteral("saveAs"), QStringLiteral("Save As"));

    connect(newAction, &QAction::triggered, this, &MainWindow::newDocument);
    connect(openAction, &QAction::triggered, this, &MainWindow::openDocument);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveDocumentAs);
    connect(pasteAction, &QAction::triggered, this, [this] { m_controller->paste(); });
    connect(cutAction, &QAction::triggered, this, [this] { m_controller->cut(); });
    connect(copyAction, &QAction::triggered, this, [this] { m_controller->copy(); });
    connect(boldAction, &QAction::triggered, this, [this] { m_controller->toggleBold(); });
    connect(italicAction, &QAction::triggered, this, [this] { m_controller->toggleItalic(); });
    connect(underlineAction, &QAction::triggered, this,
            [this] { m_controller->toggleUnderline(); });
    connect(alignLeftAction, &QAction::triggered, this, [this] {
        m_controller->setAlignment(Qt::AlignLeft);
    });
    connect(alignCenterAction, &QAction::triggered, this, [this] {
        m_controller->setAlignment(Qt::AlignHCenter);
    });
    connect(alignRightAction, &QAction::triggered, this, [this] {
        m_controller->setAlignment(Qt::AlignRight);
    });
    connect(alignJustifyAction, &QAction::triggered, this, [this] {
        m_controller->setAlignment(Qt::AlignJustify);
    });
    connect(insertImageAction, &QAction::triggered,
            this, &MainWindow::insertImage);
    connect(m_ribbon->applyImageSizeButton(), &QPushButton::clicked,
            this, [this] {
                constexpr qreal LogicalUnitsPerMillimeter = 96.0 / 25.4;
                m_controller->resizeSelectedImage(
                    m_ribbon->imageWidthSpinBox()->value()
                        * LogicalUnitsPerMillimeter,
                    m_ribbon->imageHeightSpinBox()->value()
                        * LogicalUnitsPerMillimeter);
            });
    connect(m_documentView, &DocumentView::imageResizeStarted,
            this, [this](int position) {
                m_controller->beginImageResize(position);
            });
    connect(m_documentView, &DocumentView::imageResizeChanged,
            this, [this](int position, const QSizeF &size) {
                m_controller->resizeImageAtPosition(
                    position, size.width(), size.height());
            });
    connect(m_documentView, &DocumentView::imageResizeFinished,
            this, [this] {
                m_controller->finishImageResize();
            });
    connect(m_documentView, &DocumentView::imageMoveStarted,
            this, [this](int position) {
                m_controller->beginImageMove(position);
            });
    connect(m_documentView, &DocumentView::imageMoveChanged,
            this, [this](int position, const QPointF &positionInScene) {
                m_controller->moveImageAtPosition(position, positionInScene);
            });
    connect(m_documentView, &DocumentView::imageMoveFinished,
            this, [this] {
                m_controller->finishImageMove();
            });
    connect(m_documentView, &DocumentView::imageLayerSelected,
            this, [this] {
                updateFormatControls();
            });

    connect(m_ribbon->fontFamilyCombo(), &QFontComboBox::currentFontChanged,
            this, [this](const QFont &font) {
                m_controller->setFontFamily(font.family());
            });
    connect(m_ribbon->fontSizeCombo(), &QComboBox::currentTextChanged,
            this, [this](const QString &text) {
                bool ok = false;
                const double size = text.toDouble(&ok);
                if (ok)
                    m_controller->setFontSize(size);
            });

    connect(m_ribbon->zoomInButton(), &QPushButton::clicked,
            m_documentView, &DocumentView::zoomIn);
    connect(m_ribbon->zoomOutButton(), &QPushButton::clicked,
            m_documentView, &DocumentView::zoomOut);
    connect(m_ribbon->fitPageButton(), &QPushButton::clicked,
            m_documentView, &DocumentView::fitPage);
    connect(m_ribbon->fitWidthButton(), &QPushButton::clicked,
            m_documentView, &DocumentView::fitWidth);
    connect(m_ribbon->zoomCombo(),
            qOverload<int>(&QComboBox::activated),
            this, [this](int) {
                const QString text = m_ribbon->zoomCombo()->currentText();
                QString percentage = text;
                percentage.remove(QLatin1Char('%'));
                bool ok = false;
                const qreal zoom = percentage.toDouble(&ok) / 100.0;
                if (ok)
                    m_documentView->setZoom(zoom);
            });
    connect(m_documentView, &DocumentView::zoomChanged,
            this, [this](int percentage) {
                const QSignalBlocker blocker(m_ribbon->zoomCombo());
                m_ribbon->zoomCombo()->setCurrentText(
                    QStringLiteral("%1%").arg(percentage));
                statusBar()->showMessage(
                    QStringLiteral("Pages: %1    Zoom: %2%")
                        .arg(m_documentView->pageCount())
                        .arg(percentage));
            });
    connect(m_documentView, &DocumentView::pageCountChanged,
            this, [this](int count) {
                statusBar()->showMessage(
                    QStringLiteral("Pages: %1    Zoom: %2%")
                        .arg(count)
                        .arg(m_documentView->zoomPercent()));
            });
    connect(m_documentView, &DocumentView::activeEditorStateChanged,
            this, &MainWindow::updateFormatControls);
    connect(m_document->textDocument(), &QTextDocument::modificationChanged,
            this, [this](bool modified) {
                Q_UNUSED(modified);
                updateWindowTitle();
            });
    statusBar()->showMessage(
        QStringLiteral("Pages: %1    Zoom: %2%")
            .arg(m_documentView->pageCount())
            .arg(m_documentView->zoomPercent()));
    updateFormatControls();
}

void MainWindow::newDocument()
{
    if (!confirmDiscardChanges())
        return;

    m_document->textDocument()->clear();
    m_document->textDocument()->clearUndoRedoStacks();
    m_document->setPageSettings(PageSettings());
    m_document->setModified(false);
    m_fileName.clear();
    updateWindowTitle();
}

void MainWindow::openDocument()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open Document"), m_fileName,
        DocumentFile::fileDialogFilter());
    if (fileName.isEmpty() || !confirmDiscardChanges())
        return;

    QString errorMessage;
    if (!DocumentFile::load(*m_document, fileName, &errorMessage)) {
        QMessageBox::warning(
            this, QStringLiteral("Cannot open document"),
            QStringLiteral("The selected file could not be opened:\n%1")
                .arg(errorMessage));
        return;
    }

    m_fileName = fileName;
    updateWindowTitle();
}

void MainWindow::saveDocument()
{
    if (m_fileName.isEmpty()) {
        saveDocumentAs();
        return;
    }
    saveDocumentTo(m_fileName);
}

void MainWindow::saveDocumentAs()
{
    QString suggestedName = m_fileName;
    if (suggestedName.isEmpty())
        suggestedName = QStringLiteral("Untitled.docu");
    const QString fileName = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save Document"), suggestedName,
        DocumentFile::fileDialogFilter());
    if (!fileName.isEmpty())
        saveDocumentTo(fileName);
}

bool MainWindow::saveDocumentTo(const QString &fileName)
{
    QString errorMessage;
    if (!DocumentFile::save(*m_document, fileName, &errorMessage)) {
        QMessageBox::warning(
            this, QStringLiteral("Cannot save document"),
            QStringLiteral("The document could not be saved:\n%1")
                .arg(errorMessage));
        return false;
    }

    m_fileName = fileName;
    m_document->setModified(false);
    updateWindowTitle();
    return true;
}

bool MainWindow::confirmDiscardChanges()
{
    if (!m_document->isModified())
        return true;

    QMessageBox messageBox(QMessageBox::Warning,
                           QStringLiteral("Unsaved changes"),
                           QStringLiteral("Save changes before continuing?"),
                           QMessageBox::Save | QMessageBox::Discard
                               | QMessageBox::Cancel,
                           this);
    messageBox.setDefaultButton(QMessageBox::Save);
    const int response = messageBox.exec();
    if (response == QMessageBox::Save) {
        saveDocument();
        return !m_document->isModified();
    }
    return response == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardChanges())
        event->accept();
    else
        event->ignore();
}

void MainWindow::updateWindowTitle()
{
    const QString documentName = m_fileName.isEmpty()
        ? QStringLiteral("Untitled")
        : QFileInfo(m_fileName).fileName();
    setWindowTitle(QStringLiteral("%1%2 - Docu")
                       .arg(documentName,
                            m_document->isModified() ? QStringLiteral("*")
                                                     : QString()));
}

void MainWindow::insertImage()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this, QStringLiteral("Insert Picture"), QString(),
        ImageLoader::fileDialogFilter());
    if (fileName.isEmpty())
        return;

    QString errorMessage;
    const QImage image = ImageLoader::load(
        fileName, m_document->pageSettings().contentSizeLogical(), &errorMessage);
    if (image.isNull()) {
        QMessageBox::warning(
            this, QStringLiteral("Cannot insert picture"),
            QStringLiteral("The selected file could not be read as an image:\n%1")
                .arg(errorMessage));
        return;
    }

    if (!m_controller->insertImage(image)) {
        QMessageBox::warning(
            this, QStringLiteral("Cannot insert picture"),
            QStringLiteral("The image could not be inserted into the document."));
    }
}

void MainWindow::updateFormatControls()
{
    QTextEdit *editor = m_documentView->activeEditor();
    if (!editor)
        return;

    const QTextImageFormat imageFormat = m_controller->selectedImageFormat();
    const bool imageSelected = imageFormat.isImageFormat();
    constexpr qreal MillimetersPerLogicalUnit = 25.4 / 96.0;
    const QSizeF maximumImageSize =
        m_document->pageSettings().contentSizeLogical();
    m_ribbon->imageWidthSpinBox()->setMaximum(
        maximumImageSize.width() * MillimetersPerLogicalUnit);
    m_ribbon->imageHeightSpinBox()->setMaximum(
        maximumImageSize.height() * MillimetersPerLogicalUnit);
    m_ribbon->imageWidthSpinBox()->setEnabled(imageSelected);
    m_ribbon->imageHeightSpinBox()->setEnabled(imageSelected);
    m_ribbon->applyImageSizeButton()->setEnabled(imageSelected);
    if (imageSelected) {
        const QSignalBlocker widthBlocker(m_ribbon->imageWidthSpinBox());
        const QSignalBlocker heightBlocker(m_ribbon->imageHeightSpinBox());
        m_ribbon->imageWidthSpinBox()->setValue(
            imageFormat.width() * MillimetersPerLogicalUnit);
        m_ribbon->imageHeightSpinBox()->setValue(
            imageFormat.height() * MillimetersPerLogicalUnit);
    }

    const QTextCharFormat charFormat = editor->currentCharFormat();
    const Qt::Alignment alignment =
        editor->textCursor().blockFormat().alignment();

    const QSignalBlocker boldBlocker(m_actions.value(QStringLiteral("bold")));
    const QSignalBlocker italicBlocker(m_actions.value(QStringLiteral("italic")));
    const QSignalBlocker underlineBlocker(m_actions.value(QStringLiteral("underline")));
    const QSignalBlocker leftBlocker(m_actions.value(QStringLiteral("alignLeft")));
    const QSignalBlocker centerBlocker(m_actions.value(QStringLiteral("alignCenter")));
    const QSignalBlocker rightBlocker(m_actions.value(QStringLiteral("alignRight")));
    const QSignalBlocker justifyBlocker(m_actions.value(QStringLiteral("alignJustify")));
    const QSignalBlocker familyBlocker(m_ribbon->fontFamilyCombo());
    const QSignalBlocker sizeBlocker(m_ribbon->fontSizeCombo());

    m_actions.value(QStringLiteral("bold"))->setChecked(charFormat.fontWeight() >= QFont::DemiBold);
    m_actions.value(QStringLiteral("italic"))->setChecked(charFormat.fontItalic());
    m_actions.value(QStringLiteral("underline"))->setChecked(charFormat.fontUnderline());
    m_actions.value(QStringLiteral("alignLeft"))->setChecked(alignment == Qt::AlignLeft);
    m_actions.value(QStringLiteral("alignCenter"))->setChecked(alignment == Qt::AlignHCenter);
    m_actions.value(QStringLiteral("alignRight"))->setChecked(alignment == Qt::AlignRight);
    m_actions.value(QStringLiteral("alignJustify"))->setChecked(alignment == Qt::AlignJustify);

    m_ribbon->fontFamilyCombo()->setCurrentFont(charFormat.font());
    if (charFormat.fontPointSize() > 0.0)
        m_ribbon->fontSizeCombo()->setCurrentText(
            QString::number(charFormat.fontPointSize(), 'g', 4));
}
