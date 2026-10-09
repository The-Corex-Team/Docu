#include "DocumentController.h"

#include "Document.h"
#include "ImageLayerFormat.h"
#include "gui/Editor/DocumentView.h"

#include <QFont>
#include <QGraphicsScene>
#include <QImage>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextImageFormat>
#include <QUrl>
#include <QUuid>
#include <QtMath>

DocumentController::DocumentController(Document *document, DocumentView *view)
    : m_document(document),
      m_view(view)
{
}

void DocumentController::cut()
{
    m_view->activeEditor()->cut();
}

void DocumentController::copy()
{
    m_view->activeEditor()->copy();
}

void DocumentController::paste()
{
    m_view->activeEditor()->paste();
}

bool DocumentController::insertImage(const QImage &image)
{
    if (image.isNull() || !m_view->activeEditor())
        return false;

    const QSizeF maximumSize = m_document->pageSettings().contentSizeLogical();
    QSizeF displaySize(image.size());
    if (displaySize.width() > maximumSize.width()
        || displaySize.height() > maximumSize.height()) {
        displaySize.scale(maximumSize, Qt::KeepAspectRatio);
    }

    const QString resourceName =
        QStringLiteral("docu-image:%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QUrl resourceUrl(resourceName);
    m_document->textDocument()->addResource(
        QTextDocument::ImageResource, resourceUrl, image);

    QTextImageFormat imageFormat;
    imageFormat.setName(resourceName);
    imageFormat.setWidth(1.0);
    imageFormat.setHeight(1.0);

    QTextEdit *editor = m_view->activeEditor();
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.insertImage(imageFormat);
    const int imagePosition = cursor.position() - 1;
    QTextCursor imageCursor = imageCursorAtPosition(imagePosition);
    if (!imageCursor.hasSelection()) {
        cursor.endEditBlock();
        return false;
    }

    QTextCursor imageStartCursor = imageCursor;
    imageStartCursor.setPosition(imagePosition);
    const QPointF imagePositionInScene =
        m_view->scenePositionForCursor(imageStartCursor);
    imageFormat.setProperty(ImageLayerFormat::IsLayeredImage, true);
    imageFormat.setProperty(ImageLayerFormat::SourceWidth, displaySize.width());
    imageFormat.setProperty(ImageLayerFormat::SourceHeight, displaySize.height());
    imageFormat.setProperty(ImageLayerFormat::PositionX, imagePositionInScene.x());
    imageFormat.setProperty(ImageLayerFormat::PositionY, imagePositionInScene.y());
    imageCursor.mergeCharFormat(imageFormat);
    cursor.endEditBlock();
    editor->setTextCursor(cursor);
    return true;
}

QTextImageFormat DocumentController::selectedImageFormat() const
{
    const QTextCursor cursor = selectedImageCursor();
    if (!cursor.hasSelection() || !cursor.charFormat().isImageFormat())
        return {};

    QTextImageFormat format = cursor.charFormat().toImageFormat();
    format.setWidth(cursor.charFormat()
                        .property(ImageLayerFormat::SourceWidth).toReal());
    format.setHeight(cursor.charFormat()
                         .property(ImageLayerFormat::SourceHeight).toReal());
    return format;
}

bool DocumentController::resizeSelectedImage(qreal width, qreal height)
{
    const QTextCursor cursor = selectedImageCursor();
    if (!cursor.hasSelection())
        return false;

    return resizeImageAtPosition(cursor.selectionStart(), width, height);
}

void DocumentController::beginImageResize(int position)
{
    finishImageResize();
    m_imageResizeCursor = imageCursorAtPosition(position);
    if (m_imageResizeCursor.hasSelection())
        m_imageResizeCursor.beginEditBlock();
}

bool DocumentController::resizeImageAtPosition(int position,
                                               qreal width, qreal height)
{
    if (!m_view->activeEditor())
        return false;

    const QSizeF maximumSize = m_document->pageSettings().contentSizeLogical();
    if (!qIsFinite(width) || !qIsFinite(height)
        || width <= 0.0 || height <= 0.0
        || width > maximumSize.width() || height > maximumSize.height()) {
        return false;
    }

    QTextCursor cursor = imageCursorAtPosition(position);
    if (!cursor.hasSelection() || !cursor.charFormat().isImageFormat())
        return false;

    QTextImageFormat format = cursor.charFormat().toImageFormat();
    format.setProperty(ImageLayerFormat::SourceWidth, width);
    format.setProperty(ImageLayerFormat::SourceHeight, height);
    cursor.mergeCharFormat(format);
    m_view->activeEditor()->setTextCursor(cursor);
    return true;
}

void DocumentController::finishImageResize()
{
    if (m_imageResizeCursor.hasSelection())
        m_imageResizeCursor.endEditBlock();
    m_imageResizeCursor = QTextCursor();
}

void DocumentController::beginImageMove(int position)
{
    finishImageMove();
    m_imageMoveCursor = imageCursorAtPosition(position);
    if (m_imageMoveCursor.hasSelection())
        m_imageMoveCursor.beginEditBlock();
}

bool DocumentController::moveImageAtPosition(
    int position, const QPointF &scenePosition)
{
    if (!m_view->activeEditor()
        || !qIsFinite(scenePosition.x()) || !qIsFinite(scenePosition.y())) {
        return false;
    }

    QTextCursor cursor = imageCursorAtPosition(position);
    if (!cursor.hasSelection()
        || !cursor.charFormat().property(ImageLayerFormat::IsLayeredImage).toBool()) {
        return false;
    }

    const QSizeF imageSize(
        cursor.charFormat().property(ImageLayerFormat::SourceWidth).toReal(),
        cursor.charFormat().property(ImageLayerFormat::SourceHeight).toReal());
    const QSizeF documentSize = m_document->pageSettings().pageSizeLogical();
    const qreal documentHeight = m_view->scene()->sceneRect().height();
    if (imageSize.isEmpty() || imageSize.width() > documentSize.width()
        || imageSize.height() > documentHeight
        || scenePosition.x() < 0.0
        || scenePosition.x() + imageSize.width() > documentSize.width()
        || scenePosition.y() < 0.0
        || scenePosition.y() + imageSize.height() > documentHeight) {
        return false;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setProperty(ImageLayerFormat::PositionX, scenePosition.x());
    format.setProperty(ImageLayerFormat::PositionY, scenePosition.y());
    cursor.mergeCharFormat(format);
    return true;
}

void DocumentController::finishImageMove()
{
    if (m_imageMoveCursor.hasSelection())
        m_imageMoveCursor.endEditBlock();
    m_imageMoveCursor = QTextCursor();
}

void DocumentController::undo()
{
    m_document->textDocument()->undo();
}

void DocumentController::redo()
{
    m_document->textDocument()->redo();
}

void DocumentController::toggleBold()
{
    QTextCharFormat format;
    format.setFontWeight(m_view->activeEditor()->currentCharFormat().fontWeight() >= QFont::DemiBold
                             ? QFont::Normal
                             : QFont::Bold);
    mergeFormat(format);
}

void DocumentController::toggleItalic()
{
    QTextCharFormat format;
    format.setFontItalic(!m_view->activeEditor()->currentCharFormat().fontItalic());
    mergeFormat(format);
}

void DocumentController::toggleUnderline()
{
    QTextCharFormat format;
    format.setFontUnderline(!m_view->activeEditor()->currentCharFormat().fontUnderline());
    mergeFormat(format);
}

void DocumentController::setFontFamily(const QString &family)
{
    QTextCharFormat format;
    format.setFontFamilies({family});
    mergeFormat(format);
}

void DocumentController::setFontSize(double pointSize)
{
    if (pointSize <= 0.0)
        return;

    QTextCharFormat format;
    format.setFontPointSize(pointSize);
    mergeFormat(format);
}

void DocumentController::setAlignment(Qt::Alignment alignment)
{
    QTextCursor cursor = currentCursor();
    QTextBlockFormat format;
    format.setAlignment(alignment);
    cursor.mergeBlockFormat(format);
    m_view->activeEditor()->setTextCursor(cursor);
}

QTextCursor DocumentController::currentCursor() const
{
    return m_view->activeEditor()->textCursor();
}

QTextCursor DocumentController::selectedImageCursor() const
{
    if (!m_view->activeEditor())
        return {};

    const QTextCursor cursor = currentCursor();
    QTextDocument *document = m_document->textDocument();
    const int position = cursor.position();
    const int lastPosition = document->characterCount() - 2;
    for (const int candidate : {position, position - 1}) {
        if (candidate < 0 || candidate > lastPosition)
            continue;

        QTextCursor imageCursor(document);
        imageCursor.setPosition(candidate);
        imageCursor.movePosition(QTextCursor::NextCharacter,
                                 QTextCursor::KeepAnchor);
        if (imageCursor.charFormat().isImageFormat())
            return imageCursor;
    }

    return {};
}

QTextCursor DocumentController::imageCursorAtPosition(int position) const
{
    QTextDocument *document = m_document->textDocument();
    if (position < 0 || position >= document->characterCount() - 1)
        return {};

    QTextCursor cursor(document);
    cursor.setPosition(position);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    return cursor.charFormat().isImageFormat() ? cursor : QTextCursor();
}

void DocumentController::mergeFormat(const QTextCharFormat &format)
{
    QTextCursor cursor = currentCursor();
    if (cursor.hasSelection()) {
        cursor.mergeCharFormat(format);
        m_view->activeEditor()->setTextCursor(cursor);
    } else {
        m_view->activeEditor()->mergeCurrentCharFormat(format);
    }
}
