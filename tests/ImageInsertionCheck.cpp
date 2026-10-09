#include "document/Document.h"
#include "document/DocumentController.h"
#include "document/ImageLayerFormat.h"
#include "gui/Editor/DocumentView.h"
#include "io/ImageLoader.h"

#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsObject>
#include <QImage>
#include <QPainter>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QTemporaryDir>
#include <QVariant>
#include <QUrl>
#include <QtMath>

#include <cstdio>

namespace {

bool findImage(QTextDocument *document, QTextImageFormat *foundFormat = nullptr,
               int *foundPosition = nullptr)
{
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid() || !fragment.charFormat().isImageFormat())
                continue;

            if (foundFormat) {
                *foundFormat = fragment.charFormat().toImageFormat();
                if (fragment.charFormat()
                        .property(ImageLayerFormat::IsLayeredImage).toBool()) {
                    foundFormat->setWidth(fragment.charFormat()
                                              .property(ImageLayerFormat::SourceWidth)
                                              .toReal());
                    foundFormat->setHeight(fragment.charFormat()
                                               .property(ImageLayerFormat::SourceHeight)
                                               .toReal());
                }
            }
            if (foundPosition)
                *foundPosition = fragment.position();
            return true;
        }
    }
    return false;
}

bool dragResizeHandle(DocumentView &view, const QPointF &delta)
{
    QGraphicsObject *overlay = nullptr;
    for (QGraphicsItem *item : view.scene()->items()) {
        if (item->zValue() == 3.0) {
            overlay = qgraphicsitem_cast<QGraphicsObject *>(item);
            break;
        }
    }
    if (!overlay || !overlay->isVisible())
        return false;

    const QPointF start = overlay->mapToScene(QPointF(120.0, 80.0));
    QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
    press.setWidget(view.viewport());
    press.setPos(overlay->mapFromScene(start));
    press.setScenePos(start);
    press.setButton(Qt::LeftButton);
    press.setButtons(Qt::LeftButton);
    view.scene()->sendEvent(overlay, &press);
    if (!press.isAccepted())
        return false;

    const QPointF finish = start + delta;
    QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
    move.setWidget(view.viewport());
    move.setPos(overlay->mapFromScene(finish));
    move.setScenePos(finish);
    move.setLastPos(overlay->mapFromScene(start));
    move.setLastScenePos(start);
    move.setButtons(Qt::LeftButton);
    view.scene()->sendEvent(overlay, &move);

    QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
    release.setWidget(view.viewport());
    release.setPos(overlay->mapFromScene(finish));
    release.setScenePos(finish);
    release.setButton(Qt::LeftButton);
    release.setButtons(Qt::NoButton);
    view.scene()->sendEvent(overlay, &release);
    return true;
}

bool dragImageLayer(DocumentView &view, const QPointF &delta)
{
    QGraphicsObject *layer = nullptr;
    for (QGraphicsItem *item : view.scene()->items()) {
        if (item->zValue() == 2.0) {
            layer = qgraphicsitem_cast<QGraphicsObject *>(item);
            break;
        }
    }
    if (!layer)
        return false;

    const QPointF start = layer->mapToScene(layer->boundingRect().center());
    QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
    press.setWidget(view.viewport());
    press.setPos(layer->mapFromScene(start));
    press.setScenePos(start);
    press.setButton(Qt::LeftButton);
    press.setButtons(Qt::LeftButton);
    view.scene()->sendEvent(layer, &press);
    if (!press.isAccepted())
        return false;

    const QPointF finish = start + delta;
    QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
    move.setWidget(view.viewport());
    move.setPos(layer->mapFromScene(finish));
    move.setScenePos(finish);
    move.setLastPos(layer->mapFromScene(start));
    move.setLastScenePos(start);
    move.setButtons(Qt::LeftButton);
    view.scene()->sendEvent(layer, &move);

    QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
    release.setWidget(view.viewport());
    release.setPos(layer->mapFromScene(finish));
    release.setScenePos(finish);
    release.setButton(Qt::LeftButton);
    release.setButtons(Qt::NoButton);
    view.scene()->sendEvent(layer, &release);
    return true;
}

QRect findSolidColorBounds(QWidget *widget, const QColor &color)
{
    QImage image(widget->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget->render(&painter);
    painter.end();

    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) != color)
                continue;
            bounds |= QRect(x, y, 1, 1);
        }
    }
    return bounds;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    Document document;
    DocumentView view(&document);
    DocumentController controller(&document, &view);
    view.resize(900, 700);
    view.show();
    application.processEvents();

    QTextCursor cursor(document.textDocument());
    cursor.insertText(QStringLiteral("before after"));
    cursor.setPosition(7);
    view.activeEditor()->setTextCursor(cursor);

    QImage source(800, 1600, QImage::Format_ARGB32);
    source.fill(QColor(190, 40, 70));
    const bool insertionSucceeded = controller.insertImage(source);
    application.processEvents();

    QTextImageFormat insertedFormat;
    const bool imageExists =
        findImage(document.textDocument(), &insertedFormat);
    const QSizeF maximumSize = document.pageSettings().contentSizeLogical();
    const bool displaySizeFits =
        insertedFormat.width() <= maximumSize.width()
        && insertedFormat.height() <= maximumSize.height();
    const QImage storedImage =
        document.textDocument()
            ->resource(QTextDocument::ImageResource, QUrl(insertedFormat.name()))
            .value<QImage>();
    const bool resourceWasStored =
        storedImage.size() == source.size()
        && storedImage.pixelColor(0, 0) == source.pixelColor(0, 0);

    controller.undo();
    application.processEvents();
    const bool undoRemovedImage = !findImage(document.textDocument());
    controller.redo();
    application.processEvents();
    const bool redoRestoredImage = findImage(document.textDocument());

    QTextImageFormat beforeResizeFormat;
    int imagePosition = -1;
    const bool imagePositionFound =
        findImage(document.textDocument(), &beforeResizeFormat, &imagePosition);
    QTextCursor imageCursor(document.textDocument());
    imageCursor.setPosition(imagePosition + 1);
    view.activeEditor()->setTextCursor(imageCursor);
    const bool resizeSucceeded =
        imagePositionFound && controller.resizeSelectedImage(120.0, 80.0);
    QTextImageFormat resizedFormat;
    const bool resizedToRequestedSize =
        findImage(document.textDocument(), &resizedFormat)
        && qAbs(resizedFormat.width() - 120.0) < 0.1
        && qAbs(resizedFormat.height() - 80.0) < 0.1;
    controller.undo();
    QTextImageFormat undoneResizeFormat;
    const bool undoRestoredOriginalSize =
        findImage(document.textDocument(), &undoneResizeFormat)
        && qAbs(undoneResizeFormat.width() - beforeResizeFormat.width()) < 0.1
        && qAbs(undoneResizeFormat.height() - beforeResizeFormat.height()) < 0.1;
    controller.redo();
    QTextImageFormat redoneResizeFormat;
    const bool redoRestoredResizedSize =
        findImage(document.textDocument(), &redoneResizeFormat)
        && qAbs(redoneResizeFormat.width() - 120.0) < 0.1
        && qAbs(redoneResizeFormat.height() - 80.0) < 0.1;
    controller.beginImageResize(imagePosition);
    const bool firstGroupedResize =
        controller.resizeImageAtPosition(imagePosition, 130.0, 86.0);
    const bool secondGroupedResize =
        controller.resizeImageAtPosition(imagePosition, 140.0, 92.0);
    controller.finishImageResize();
    QTextImageFormat groupedResizeFormat;
    const bool groupedResizeApplied =
        firstGroupedResize && secondGroupedResize
        && findImage(document.textDocument(), &groupedResizeFormat)
        && qAbs(groupedResizeFormat.width() - 140.0) < 0.1
        && qAbs(groupedResizeFormat.height() - 92.0) < 0.1;
    controller.undo();
    QTextImageFormat groupedUndoFormat;
    const bool groupedResizeUndone =
        findImage(document.textDocument(), &groupedUndoFormat)
        && qAbs(groupedUndoFormat.width() - 120.0) < 0.1
        && qAbs(groupedUndoFormat.height() - 80.0) < 0.1;
    QObject::connect(&view, &DocumentView::imageResizeStarted, &application,
                     [&controller](int position) {
                         controller.beginImageResize(position);
                     });
    QObject::connect(&view, &DocumentView::imageResizeChanged, &application,
                     [&controller](int position, const QSizeF &size) {
                         controller.resizeImageAtPosition(
                             position, size.width(), size.height());
                     });
    QObject::connect(&view, &DocumentView::imageResizeFinished, &application,
                     [&controller] {
                         controller.finishImageResize();
                     });
    QObject::connect(&view, &DocumentView::imageMoveStarted, &application,
                     [&controller](int position) {
                         controller.beginImageMove(position);
                     });
    QObject::connect(&view, &DocumentView::imageMoveChanged, &application,
                     [&controller](int position, const QPointF &scenePosition) {
                         controller.moveImageAtPosition(position, scenePosition);
                     });
    QObject::connect(&view, &DocumentView::imageMoveFinished, &application,
                     [&controller] {
                         controller.finishImageMove();
                     });
    view.activeEditor()->setTextCursor(imageCursor);
    application.processEvents();
    QGraphicsObject *overlay = nullptr;
    for (QGraphicsItem *item : view.scene()->items()) {
        if (item->zValue() == 3.0) {
            overlay = qgraphicsitem_cast<QGraphicsObject *>(item);
            break;
        }
    }
    const QRect renderedImageBounds =
        findSolidColorBounds(view.viewport(), source.pixelColor(0, 0));
    const QPoint renderedOverlayTopLeft =
        overlay ? view.mapFromScene(overlay->pos()) : QPoint();
    const QRect expectedImageViewportBounds = overlay
        ? view.mapFromScene(QRectF(overlay->pos(), QSizeF(120.0, 80.0)))
              .boundingRect()
        : QRect();
    const bool selectionOutlineAligned =
        overlay && overlay->isVisible() && !renderedImageBounds.isEmpty()
        && (renderedOverlayTopLeft - renderedImageBounds.topLeft()).manhattanLength()
               <= 3;
    const bool imagePaintStaysWithinBounds =
        !renderedImageBounds.isEmpty() && !expectedImageViewportBounds.isEmpty()
        && renderedImageBounds.left() >= expectedImageViewportBounds.left() - 1
        && renderedImageBounds.top() >= expectedImageViewportBounds.top() - 1
        && renderedImageBounds.right() <= expectedImageViewportBounds.right() + 1
        && renderedImageBounds.bottom() <= expectedImageViewportBounds.bottom() + 1
        && renderedImageBounds.width() >= expectedImageViewportBounds.width() - 2
        && renderedImageBounds.height() >= expectedImageViewportBounds.height() - 2;
    const bool handleDragSucceeded =
        dragResizeHandle(view, QPointF(30.0, 20.0));
    QTextImageFormat draggedFormat;
    const bool handleResizedWithAspectRatio =
        handleDragSucceeded
        && findImage(document.textDocument(), &draggedFormat)
        && qAbs(draggedFormat.width() - 150.0) < 0.1
        && qAbs(draggedFormat.height() - 100.0) < 0.1;
    controller.undo();
    QTextImageFormat dragUndoFormat;
    const bool dragUndoRestoredSize =
        findImage(document.textDocument(), &dragUndoFormat)
        && qAbs(dragUndoFormat.width() - 120.0) < 0.1
        && qAbs(dragUndoFormat.height() - 80.0) < 0.1;
    QGraphicsObject *layer = nullptr;
    for (QGraphicsItem *item : view.scene()->items()) {
        if (item->zValue() == 2.0) {
            layer = qgraphicsitem_cast<QGraphicsObject *>(item);
            break;
        }
    }
    const QPointF layerStart = layer ? layer->pos() : QPointF();
    const bool imageMoveSucceeded =
        layer && dragImageLayer(view, QPointF(40.0, 25.0));
    const QPointF layerEnd = layer ? layer->pos() : QPointF();
    const QSizeF documentPageSize = document.pageSettings().pageSizeLogical();
    const bool imageMovedWithinPage =
        imageMoveSucceeded && layerEnd.x() > layerStart.x()
        && layerEnd.y() > layerStart.y()
        && layerEnd.x() + layer->boundingRect().width() <= documentPageSize.width()
        && layerEnd.y() + layer->boundingRect().height()
               <= view.scene()->sceneRect().height();
    controller.undo();
    const bool imageMoveUndoRestoredPosition =
        layer && (layer->pos() - layerStart).manhattanLength() < 0.1;
    const bool rightBoundaryDragSucceeded =
        layer && dragImageLayer(
                     view, QPointF(documentPageSize.width() * 2.0, 0.0));
    const bool imageStayedInsideRightEdge =
        rightBoundaryDragSucceeded
        && qAbs(layer->pos().x() + layer->boundingRect().width()
                - documentPageSize.width()) < 0.1;
    controller.undo();
    const bool boundaryDragUndoRestoredPosition =
        layer && (layer->pos() - layerStart).manhattanLength() < 0.1;

    QTemporaryDir temporaryDirectory;
    QImage largeSource(3000, 4000, QImage::Format_RGB32);
    largeSource.fill(QColor(40, 90, 170));
    const QString imagePath = temporaryDirectory.filePath(QStringLiteral("large.png"));
    const bool imageSaved = largeSource.save(imagePath);
    QString loadError;
    const QSizeF contentSize = document.pageSettings().contentSizeLogical();
    const QImage loadedImage = imageSaved
        ? ImageLoader::load(imagePath, contentSize, &loadError)
        : QImage();
    const QSize maximumDecodeSize(qRound(contentSize.width() * 3.0),
                                  qRound(contentSize.height() * 3.0));
    const bool fileLoadWasBounded =
        !loadedImage.isNull()
        && loadedImage.width() <= maximumDecodeSize.width()
        && loadedImage.height() <= maximumDecodeSize.height();

    std::printf("inserted: %s; image format/resource: %s/%s; "
                "display bounds: %s; undo/redo: %s/%s; resize: %s/%s/%s/%s; "
                "drag group: %s/%s/%s/%s; outline/image bounds: %s/%s; "
                "handle drag/undo: %s/%s/%s; image move/bounds/undo: %s/%s/%s; "
                "right edge/undo: %s/%s; "
                "file load: %s\n",
                insertionSucceeded ? "yes" : "no",
                imageExists ? "yes" : "no",
                resourceWasStored ? "ok" : "missing",
                displaySizeFits ? "ok" : "out of bounds",
                undoRemovedImage ? "ok" : "failed",
                redoRestoredImage ? "ok" : "failed",
                resizeSucceeded ? "ok" : "failed",
                resizedToRequestedSize ? "ok" : "failed",
                undoRestoredOriginalSize ? "ok" : "failed",
                redoRestoredResizedSize ? "ok" : "failed",
                firstGroupedResize ? "ok" : "failed",
                secondGroupedResize ? "ok" : "failed",
                groupedResizeApplied ? "ok" : "failed",
                groupedResizeUndone ? "ok" : "failed",
                selectionOutlineAligned ? "ok" : "failed",
                imagePaintStaysWithinBounds ? "ok" : "failed",
                handleDragSucceeded ? "ok" : "failed",
                handleResizedWithAspectRatio ? "ok" : "failed",
                dragUndoRestoredSize ? "ok" : "failed",
                imageMoveSucceeded ? "ok" : "failed",
                imageMovedWithinPage ? "ok" : "failed",
                imageMoveUndoRestoredPosition ? "ok" : "failed",
                rightBoundaryDragSucceeded ? "ok" : "failed",
                imageStayedInsideRightEdge
                    && boundaryDragUndoRestoredPosition ? "ok" : "failed",
                fileLoadWasBounded ? "ok" : "failed");

    return insertionSucceeded && imageExists && displaySizeFits
                   && resourceWasStored && undoRemovedImage && redoRestoredImage
                   && resizeSucceeded && resizedToRequestedSize
                   && undoRestoredOriginalSize && redoRestoredResizedSize
                   && firstGroupedResize && secondGroupedResize
                   && groupedResizeApplied && groupedResizeUndone
                   && selectionOutlineAligned
                   && imagePaintStaysWithinBounds
                   && handleDragSucceeded && handleResizedWithAspectRatio
                   && dragUndoRestoredSize
                   && imageMoveSucceeded && imageMovedWithinPage
                   && imageMoveUndoRestoredPosition
                   && rightBoundaryDragSucceeded && imageStayedInsideRightEdge
                   && boundaryDragUndoRestoredPosition
                   && fileLoadWasBounded
               ? 0
               : 1;
}
