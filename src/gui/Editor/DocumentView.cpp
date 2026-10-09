#include "DocumentView.h"

#include "Style.h"
#include "document/Document.h"
#include "document/ImageLayerFormat.h"
#include "document/PageSettings.h"

#include <QAbstractTextDocumentLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QCursor>
#include <QEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsObject>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QOpenGLWidget>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QTextEdit>
#include <QTimer>
#include <QtMath>
#include <functional>
#include <utility>

namespace {
constexpr qreal PageGap = 24.0;
constexpr qreal ZoomStep = 1.15;
constexpr qreal MinimumZoom = 0.25;
constexpr qreal MaximumZoom = 4.0;
constexpr qreal FitPadding = 24.0;
constexpr qreal ImageResizeHandleSize = 12.0;
constexpr qreal MinimumImageSize = 12.0;

class ImageLayerItem final : public QGraphicsObject
{
public:
    using MoveCallback = std::function<void(int, const QPointF &)>;
    using DragStateCallback = std::function<void(int, bool)>;

    ImageLayerItem(int documentPosition, const QImage &image)
        : m_documentPosition(documentPosition),
          m_image(image)
    {
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptHoverEvents(true);
        setZValue(2.0);
    }

    QRectF boundingRect() const override
    {
        return QRectF(QPointF(0.0, 0.0), m_size);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        if (!m_image.isNull() && !m_size.isEmpty())
            painter->drawImage(boundingRect(), m_image);
    }

    void setLayer(int documentPosition, const QImage &image,
                  const QSizeF &size, const QPointF &position,
                  const QSizeF &documentSize)
    {
        prepareGeometryChange();
        m_documentPosition = documentPosition;
        if (m_image.cacheKey() != image.cacheKey())
            m_image = image;
        m_size = size;
        m_documentSize = documentSize;
        setPos(constrainedPosition(position));
        update();
    }

    void setMoveCallback(MoveCallback callback)
    {
        m_moveCallback = std::move(callback);
    }

    void setDragStateCallback(DragStateCallback callback)
    {
        m_dragStateCallback = std::move(callback);
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override
    {
        setCursor(Qt::OpenHandCursor);
        QGraphicsObject::hoverEnterEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override
    {
        unsetCursor();
        QGraphicsObject::hoverLeaveEvent(event);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            event->ignore();
            return;
        }

        m_dragging = true;
        m_dragOffset = event->scenePos() - pos();
        setCursor(Qt::ClosedHandCursor);
        if (m_dragStateCallback)
            m_dragStateCallback(m_documentPosition, true);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!m_dragging) {
            event->ignore();
            return;
        }

        const QPointF newPosition =
            constrainedPosition(event->scenePos() - m_dragOffset);
        if (newPosition != pos()) {
            setPos(newPosition);
            if (m_moveCallback)
                m_moveCallback(m_documentPosition, newPosition);
        }
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!m_dragging) {
            event->ignore();
            return;
        }

        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        if (m_dragStateCallback)
            m_dragStateCallback(m_documentPosition, false);
        event->accept();
    }

private:
    QPointF constrainedPosition(const QPointF &position) const
    {
        return QPointF(
            qBound(0.0, position.x(),
                   qMax(0.0, m_documentSize.width() - m_size.width())),
            qBound(0.0, position.y(),
                   qMax(0.0, m_documentSize.height() - m_size.height())));
    }

    int m_documentPosition;
    QImage m_image;
    QSizeF m_size;
    QSizeF m_documentSize;
    QPointF m_dragOffset;
    MoveCallback m_moveCallback;
    DragStateCallback m_dragStateCallback;
    bool m_dragging = false;
};

class ImageSelectionOverlay final : public QGraphicsObject
{
public:
    using ResizeCallback =
        std::function<void(const QPointF &, const QPointF &)>;
    using DragStateCallback = std::function<void(bool)>;

    explicit ImageSelectionOverlay(QGraphicsItem *parent = nullptr)
        : QGraphicsObject(parent)
    {
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptHoverEvents(true);
        setZValue(3.0);
        hide();
    }

    QRectF boundingRect() const override
    {
        const qreal halfHandle = m_handleSize / 2.0;
        return QRectF(-halfHandle - 1.0, -halfHandle - 1.0,
                      m_size.width() + m_handleSize + 2.0,
                      m_size.height() + m_handleSize + 2.0);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setPen(QPen(DocuStyle::imageSelectionColor(), 1.0, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QRectF(QPointF(0.0, 0.0), m_size));
        painter->setPen(QPen(Qt::white, 1.0));
        painter->setBrush(DocuStyle::imageSelectionColor());
        for (const QPointF &corner : corners())
            painter->drawRect(handleRect(corner));
    }

    void setImageRect(const QRectF &rect)
    {
        prepareGeometryChange();
        setPos(rect.topLeft());
        m_size = rect.size();
        update();
        show();
    }

    void setZoom(qreal zoom)
    {
        prepareGeometryChange();
        m_handleSize = ImageResizeHandleSize / qMax(zoom, 0.01);
        update();
    }

    void setResizeCallback(ResizeCallback callback)
    {
        m_resizeCallback = std::move(callback);
    }

    void setDragStateCallback(DragStateCallback callback)
    {
        m_dragStateCallback = std::move(callback);
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override
    {
        const int handle = handleAt(event->pos());
        if (handle >= 0)
            setCursor(handle == 1 || handle == 3
                          ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor);
        else
            unsetCursor();
        QGraphicsObject::hoverMoveEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override
    {
        unsetCursor();
        QGraphicsObject::hoverLeaveEvent(event);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        const int handle = handleAt(event->pos());
        if (handle < 0) {
            event->ignore();
            return;
        }

        m_dragging = true;
        m_resizeDirection = directions()[handle];
        m_startScenePosition = event->scenePos();
        if (m_dragStateCallback)
            m_dragStateCallback(true);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!m_dragging) {
            event->ignore();
            return;
        }

        if (m_resizeCallback)
            m_resizeCallback(event->scenePos() - m_startScenePosition,
                             m_resizeDirection);
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!m_dragging) {
            event->ignore();
            return;
        }

        m_dragging = false;
        if (m_dragStateCallback)
            m_dragStateCallback(false);
        event->accept();
    }

private:
    QVector<QPointF> corners() const
    {
        return {QPointF(0.0, 0.0), QPointF(m_size.width(), 0.0),
                QPointF(m_size.width(), m_size.height()),
                QPointF(0.0, m_size.height())};
    }

    QVector<QPointF> directions() const
    {
        return {QPointF(-1.0, -1.0), QPointF(1.0, -1.0),
                QPointF(1.0, 1.0), QPointF(-1.0, 1.0)};
    }

    QRectF handleRect(const QPointF &corner) const
    {
        return QRectF(corner.x() - m_handleSize / 2.0,
                      corner.y() - m_handleSize / 2.0,
                      m_handleSize, m_handleSize);
    }

    int handleAt(const QPointF &position) const
    {
        const QVector<QPointF> imageCorners = corners();
        for (int index = 0; index < imageCorners.size(); ++index) {
            if (handleRect(imageCorners[index]).contains(position))
                return index;
        }
        return -1;
    }

    QSizeF m_size;
    qreal m_handleSize = ImageResizeHandleSize;
    QPointF m_startScenePosition;
    QPointF m_resizeDirection;
    ResizeCallback m_resizeCallback;
    DragStateCallback m_dragStateCallback;
    bool m_dragging = false;
};
}

DocumentView::DocumentView(Document *document, QWidget *parent)
    : QGraphicsView(parent),
      m_document(document),
      m_scene(new QGraphicsScene(this))
{
    setObjectName(QStringLiteral("documentView"));
    setScene(m_scene);
    const bool useOpenGL =
        QGuiApplication::platformName() != QStringLiteral("offscreen")
        && qEnvironmentVariableIsEmpty("DOCU_SOFTWARE_RENDERING");
    if (useOpenGL)
        setViewport(new QOpenGLWidget);
    setBackgroundBrush(DocuStyle::documentCanvasColor());
    setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setFrameShape(QFrame::NoFrame);

    connect(m_document, &Document::pageSettingsChanged,
            this, &DocumentView::rebuildPages);
    connect(m_document->textDocument(), &QTextDocument::contentsChanged,
            this, &DocumentView::refreshPages);
    connect(m_document->textDocument()->documentLayout(),
            &QAbstractTextDocumentLayout::pageCountChanged,
            this, &DocumentView::refreshPages);

    auto *selectionOverlay = new ImageSelectionOverlay;
    m_imageSelectionOverlay = selectionOverlay;
    selectionOverlay->setZoom(m_zoom);
    selectionOverlay->setResizeCallback(
        [this](const QPointF &delta, const QPointF &direction) {
            if (!m_selectedImageCursor.hasSelection()
                || !m_selectedImageCursor.charFormat().isImageFormat()) {
                return;
            }

            const qreal width = m_imageResizeStartSize.width();
            const qreal height = m_imageResizeStartSize.height();
            if (width <= 0.0 || height <= 0.0)
                return;

            const qreal scaleDelta =
                (delta.x() * direction.x() * width
                 + delta.y() * direction.y() * height)
                / (width * width + height * height);
            const QSizeF maximumSize =
                m_document->pageSettings().contentSizeLogical();
            const qreal minimumScale =
                qMax(MinimumImageSize / width, MinimumImageSize / height);
            const qreal maximumScale =
                qMin(maximumSize.width() / width, maximumSize.height() / height);
            const qreal scale =
                qBound(minimumScale, 1.0 + scaleDelta, maximumScale);
            emit imageResizeChanged(
                m_selectedImageCursor.selectionStart(),
                QSizeF(width * scale, height * scale));
        });
    selectionOverlay->setDragStateCallback([this](bool dragging) {
        if (dragging && m_selectedImageCursor.hasSelection()) {
            const QTextCharFormat format = m_selectedImageCursor.charFormat();
            m_imageResizeStartSize = QSizeF(
                format.property(ImageLayerFormat::SourceWidth).toReal(),
                format.property(ImageLayerFormat::SourceHeight).toReal());
            emit imageResizeStarted(m_selectedImageCursor.selectionStart());
        } else if (!dragging) {
            emit imageResizeFinished();
        }
    });
    m_scene->addItem(selectionOverlay);

    rebuildPages();
    setZoom(m_zoom);

    connect(this, &DocumentView::imageLayerSelected, this,
            [this](int position) {
                if (!m_activeEditor)
                    return;
                QTextCursor cursor(m_document->textDocument());
                cursor.setPosition(position + 1);
                m_activeEditor->setTextCursor(cursor);
                m_activeEditor->setFocus();
            });
}

QTextEdit *DocumentView::activeEditor() const
{
    return m_activeEditor;
}

int DocumentView::pageCount() const
{
    return m_pageCount;
}

int DocumentView::zoomPercent() const
{
    return qRound(m_zoom * 100.0);
}

QPointF DocumentView::scenePositionForCursor(const QTextCursor &cursor) const
{
    if (!m_activeEditor)
        return {};
    const int pageIndex = m_pageEditors.indexOf(m_activeEditor);
    if (pageIndex < 0 || pageIndex >= m_pageProxies.size())
        return {};

    QTextCursor startCursor = cursor;
    startCursor.clearSelection();
    const QPoint cursorPosition = m_activeEditor->cursorRect(startCursor).topLeft();
    const QPoint editorPosition =
        m_activeEditor->viewport()->mapTo(m_activeEditor, cursorPosition);
    return m_pageProxies[pageIndex]->mapToScene(QPointF(editorPosition));
}

void DocumentView::setZoom(qreal zoom)
{
    const qreal boundedZoom = qBound(MinimumZoom, zoom, MaximumZoom);
    if (qFuzzyCompare(m_zoom, boundedZoom))
        return;

    m_zoom = boundedZoom;
    setTransform(QTransform::fromScale(m_zoom, m_zoom));
    if (auto *overlay =
            static_cast<ImageSelectionOverlay *>(m_imageSelectionOverlay)) {
        overlay->setZoom(m_zoom);
    }
    updateImageSelectionGeometry();
    emit zoomChanged(zoomPercent());
}

void DocumentView::zoomIn()
{
    setZoom(m_zoom * ZoomStep);
}

void DocumentView::zoomOut()
{
    setZoom(m_zoom / ZoomStep);
}

void DocumentView::fitPage()
{
    const QSizeF pageSize = m_document->pageSettings().pageSizeLogical();
    const QSize available = viewport()->size();
    if (pageSize.width() <= 0.0 || pageSize.height() <= 0.0
        || available.width() <= 0 || available.height() <= 0) {
        return;
    }

    setZoom(qMin((available.width() - FitPadding) / pageSize.width(),
                 (available.height() - FitPadding) / pageSize.height()));
}

void DocumentView::fitWidth()
{
    const qreal pageWidth = m_document->pageSettings().pageSizeLogical().width();
    if (pageWidth <= 0.0 || viewport()->width() <= 0)
        return;

    setZoom((viewport()->width() - FitPadding) / pageWidth);
}

void DocumentView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor(QStringLiteral("#d5d8dc")));
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    for (int index = 0; index < m_pageCount; ++index) {
        const QRectF paper = pageRect(index);
        if (!paper.intersects(rect))
            continue;

        painter->fillRect(paper.translated(3.0, 4.0), DocuStyle::pageShadowColor());
        painter->fillRect(paper, DocuStyle::pageColor());
        painter->setPen(QPen(DocuStyle::pageBorderColor(), 1.0));
        painter->drawRect(paper);
    }
    painter->restore();
}

bool DocumentView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        if (auto *editor = qobject_cast<QTextEdit *>(watched))
            setActiveEditor(editor);
    }
    return QGraphicsView::eventFilter(watched, event);
}

QTextEdit *DocumentView::createPageEditor(int pageIndex)
{
    auto *editor = new QTextEdit;
    const QSizeF contentSize = m_document->pageSettings().contentSizeLogical();
    editor->setFixedSize(qMax(1, qRound(contentSize.width())),
                         qMax(1, qRound(contentSize.height())));
    editor->setDocument(m_document->textDocument());
    editor->setFrameShape(QFrame::NoFrame);
    editor->setDocumentTitle(QStringLiteral("Docu"));
    editor->setAcceptRichText(true);
    editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setStyleSheet(DocuStyle::pageEditorStyleSheet());
    editor->installEventFilter(this);

    connect(editor, &QTextEdit::cursorPositionChanged, this, [this, editor] {
        if (!m_synchronizingCursors)
            synchronizeCursors(editor);
        if (editor == m_activeEditor) {
            updateImageSelectionGeometry();
            emit activeEditorStateChanged();
        }
    });
    connect(editor, &QTextEdit::selectionChanged, this, [this, editor] {
        if (!m_synchronizingCursors)
            synchronizeCursors(editor);
        if (editor == m_activeEditor) {
            updateImageSelectionGeometry();
            emit activeEditorStateChanged();
        }
    });
    connect(editor, &QTextEdit::currentCharFormatChanged, this,
            [this, editor] {
                if (editor == m_activeEditor)
                    emit activeEditorStateChanged();
            });
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this, editor] {
                if (editor == m_activeEditor)
                    updateImageSelectionGeometry();
            });

    auto *proxy = m_scene->addWidget(editor);
    proxy->setZValue(1.0);
    m_pageEditors.append(editor);
    m_pageProxies.append(proxy);

    if (pageIndex == 0) {
        setActiveEditor(editor);
    } else if (m_activeEditor) {
        m_synchronizingCursors = true;
        editor->setTextCursor(m_activeEditor->textCursor());
        m_synchronizingCursors = false;
    }

    return editor;
}

void DocumentView::refreshPages()
{
    const int newCount = qMax(1, m_document->textDocument()->pageCount());
    while (m_pageEditors.size() < newCount)
        createPageEditor(m_pageEditors.size());

    bool activeEditorRemoved = false;
    while (m_pageEditors.size() > newCount) {
        QTextEdit *removedEditor = m_pageEditors.takeLast();
        QGraphicsProxyWidget *removedProxy = m_pageProxies.takeLast();
        if (removedEditor == m_activeEditor) {
            m_activeEditor = nullptr;
            activeEditorRemoved = true;
        }
        if (removedEditor) {
            removedEditor->removeEventFilter(this);
            removedEditor->deleteLater();
        }
        if (removedProxy) {
            m_scene->removeItem(removedProxy);
            delete removedProxy;
        }
    }

    if (activeEditorRemoved && !m_pageEditors.isEmpty()) {
        m_activeEditor = m_pageEditors.constLast();
        emit activeEditorChanged(m_activeEditor);
        emit activeEditorStateChanged();
    }

    const bool countChanged = newCount != m_pageCount;
    m_pageCount = newCount;
    layoutPages();
    refreshImageLayers();
    if (countChanged)
        emit pageCountChanged(m_pageCount);
}

void DocumentView::rebuildPages()
{
    for (QGraphicsProxyWidget *proxy : std::as_const(m_pageProxies)) {
        m_scene->removeItem(proxy);
        delete proxy;
    }
    m_pageEditors.clear();
    m_pageProxies.clear();
    m_activeEditor = nullptr;
    m_pageCount = 0;
    refreshPages();
}

void DocumentView::layoutPages()
{
    const PageSettings &settings = m_document->pageSettings();
    const QSizeF pageSize = settings.pageSizeLogical();
    const QMarginsF margins = settings.marginsLogical();
    const QSizeF contentSize = settings.contentSizeLogical();
    const QSizeF logicalContentSize(
        qMax(1, qRound(contentSize.width())),
        qMax(1, qRound(contentSize.height())));
    m_document->textDocument()->setPageSize(logicalContentSize);
    const qreal contentPageHeight =
        logicalContentSize.height();
    const qreal documentHeight =
        m_document->textDocument()->documentLayout()->documentSize().height();

    for (int index = 0; index < m_pageProxies.size(); ++index) {
        const qreal pageTop = index * (pageSize.height() + PageGap);
        const qreal remainingDocumentHeight =
            documentHeight - index * contentPageHeight;
        const int editorHeight = qMax(
            1, qRound(qMin(contentPageHeight, remainingDocumentHeight)));
        m_pageEditors[index]->setFixedSize(logicalContentSize.width(),
                                           editorHeight);
        m_pageProxies[index]->setPos(margins.left(),
                                     pageTop + margins.top());
    }

    const qreal sceneHeight = m_pageCount * pageSize.height()
        + qMax(0, m_pageCount - 1) * PageGap;
    m_scene->setSceneRect(0.0, 0.0, pageSize.width(), sceneHeight);
    m_scene->update();
    QTimer::singleShot(0, this, &DocumentView::setPageScrollPositions);
    updateImageSelectionGeometry();
}

void DocumentView::setPageScrollPositions()
{
    const qreal contentPageHeight =
        m_document->textDocument()->pageSize().height();
    for (int index = 0; index < m_pageEditors.size(); ++index) {
        m_pageEditors[index]->verticalScrollBar()->setValue(
            qRound(index * contentPageHeight));
    }
    updateImageSelectionGeometry();
}

void DocumentView::synchronizeCursors(QTextEdit *source)
{
    if (m_synchronizingCursors || !source)
        return;

    m_synchronizingCursors = true;
    const QTextCursor cursor = source->textCursor();
    QVector<int> scrollPositions;
    scrollPositions.reserve(m_pageEditors.size());
    for (QTextEdit *editor : std::as_const(m_pageEditors))
        scrollPositions.append(editor->verticalScrollBar()->value());

    for (QTextEdit *editor : std::as_const(m_pageEditors)) {
        if (editor != source)
            editor->setTextCursor(cursor);
    }

    for (int index = 0; index < m_pageEditors.size(); ++index)
        m_pageEditors[index]->verticalScrollBar()->setValue(scrollPositions[index]);
    m_synchronizingCursors = false;
}

void DocumentView::setActiveEditor(QTextEdit *editor)
{
    if (!editor || editor == m_activeEditor)
        return;

    m_activeEditor = editor;
    updateImageSelectionGeometry();
    emit activeEditorChanged(editor);
    emit activeEditorStateChanged();
}

QRectF DocumentView::pageRect(int pageIndex) const
{
    const QSizeF size = m_document->pageSettings().pageSizeLogical();
    const qreal top = pageIndex * (size.height() + PageGap);
    return QRectF(0.0, top, size.width(), size.height());
}

bool DocumentView::selectImageAtPosition(QTextEdit *editor, int position)
{
    if (!editor)
        return false;

    QTextDocument *document = m_document->textDocument();
    const int lastPosition = document->characterCount() - 2;
    for (const int candidate : {position, position - 1}) {
        if (candidate < 0 || candidate > lastPosition)
            continue;

        QTextCursor cursor(document);
        cursor.setPosition(candidate);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor);
        if (cursor.charFormat().isImageFormat()) {
            m_selectedImageCursor = cursor;
            return true;
        }
    }
    return false;
}

void DocumentView::clearImageSelection()
{
    m_selectedImageCursor = QTextCursor();
    if (m_imageSelectionOverlay)
        m_imageSelectionOverlay->hide();
}

void DocumentView::updateImageSelectionGeometry()
{
    if (!m_activeEditor || !m_imageSelectionOverlay)
        return;

    const QTextCursor editorCursor = m_activeEditor->textCursor();
    if (!selectImageAtPosition(m_activeEditor, editorCursor.position())) {
        clearImageSelection();
        return;
    }

    const int imagePosition = m_selectedImageCursor.selectionStart();
    auto *layer = static_cast<ImageLayerItem *>(
        m_imageLayers.value(imagePosition, nullptr));
    if (!layer
        || !m_selectedImageCursor.charFormat()
                .property(ImageLayerFormat::IsLayeredImage).toBool()) {
        clearImageSelection();
        return;
    }

    auto *overlay = static_cast<ImageSelectionOverlay *>(m_imageSelectionOverlay);
    overlay->setImageRect(layer->sceneBoundingRect());
}

void DocumentView::refreshImageLayers()
{
    QHash<int, QGraphicsObject *> updatedLayers;
    QTextDocument *textDocument = m_document->textDocument();
    const QSizeF documentSize(m_document->pageSettings().pageSizeLogical().width(),
                              m_scene->sceneRect().height());
    for (QTextBlock block = textDocument->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid() || !fragment.charFormat().isImageFormat())
                continue;

            const QTextCharFormat charFormat = fragment.charFormat();
            if (!charFormat.property(ImageLayerFormat::IsLayeredImage).toBool())
                continue;

            const int position = fragment.position();
            const QUrl imageUrl(charFormat.toImageFormat().name());
            const QImage image = textDocument
                                     ->resource(QTextDocument::ImageResource, imageUrl)
                                     .value<QImage>();
            if (image.isNull())
                continue;

            auto *layer = static_cast<ImageLayerItem *>(m_imageLayers.take(position));
            if (!layer) {
                layer = new ImageLayerItem(position, image);
                layer->setMoveCallback(
                    [this](int imagePosition, const QPointF &imagePositionInScene) {
                        emit imageMoveChanged(imagePosition, imagePositionInScene);
                        updateImageSelectionGeometry();
                    });
                layer->setDragStateCallback([this](int imagePosition, bool dragging) {
                    emit imageLayerSelected(imagePosition);
                    if (dragging)
                        emit imageMoveStarted(imagePosition);
                    else
                        emit imageMoveFinished();
                });
                m_scene->addItem(layer);
            }

            const QSizeF size(
                charFormat.property(ImageLayerFormat::SourceWidth).toReal(),
                charFormat.property(ImageLayerFormat::SourceHeight).toReal());
            const QPointF positionInScene(
                charFormat.property(ImageLayerFormat::PositionX).toReal(),
                charFormat.property(ImageLayerFormat::PositionY).toReal());
            layer->setLayer(position, image, size, positionInScene, documentSize);
            updatedLayers.insert(position, layer);
        }
    }

    for (QGraphicsObject *removedLayer : std::as_const(m_imageLayers)) {
        m_scene->removeItem(removedLayer);
        delete removedLayer;
    }
    m_imageLayers = std::move(updatedLayers);
    updateImageSelectionGeometry();
}
