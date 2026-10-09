#pragma once

#include <QGraphicsView>
#include <QHash>
#include <QSizeF>
#include <QTextCursor>
#include <QVector>

class Document;
class QGraphicsProxyWidget;
class QGraphicsScene;
class QPaintEvent;
class QGraphicsObject;
class QTextEdit;
class QImage;

class DocumentView final : public QGraphicsView
{
    Q_OBJECT

public:
    explicit DocumentView(Document *document, QWidget *parent = nullptr);

    QTextEdit *activeEditor() const;
    int pageCount() const;
    int zoomPercent() const;

    void setZoom(qreal zoom);
    void zoomIn();
    void zoomOut();
    void fitPage();
    void fitWidth();
    QPointF scenePositionForCursor(const QTextCursor &cursor) const;

signals:
    void activeEditorChanged(QTextEdit *editor);
    void activeEditorStateChanged();
    void pageCountChanged(int count);
    void zoomChanged(int percentage);
    void imageResizeStarted(int documentPosition);
    void imageResizeChanged(int documentPosition, const QSizeF &size);
    void imageResizeFinished();
    void imageMoveStarted(int documentPosition);
    void imageMoveChanged(int documentPosition, const QPointF &position);
    void imageMoveFinished();
    void imageLayerSelected(int documentPosition);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QTextEdit *createPageEditor(int pageIndex);
    void refreshPages();
    void rebuildPages();
    void layoutPages();
    void setPageScrollPositions();
    void synchronizeCursors(QTextEdit *source);
    void setActiveEditor(QTextEdit *editor);
    bool selectImageAtPosition(QTextEdit *editor, int position);
    void clearImageSelection();
    void updateImageSelectionGeometry();
    void refreshImageLayers();
    QRectF pageRect(int pageIndex) const;

    Document *m_document;
    QGraphicsScene *m_scene;
    QVector<QTextEdit *> m_pageEditors;
    QVector<QGraphicsProxyWidget *> m_pageProxies;
    QTextEdit *m_activeEditor = nullptr;
    QGraphicsObject *m_imageSelectionOverlay = nullptr;
    QHash<int, QGraphicsObject *> m_imageLayers;
    QTextCursor m_selectedImageCursor;
    QSizeF m_imageResizeStartSize;
    qreal m_zoom = 0.60;
    int m_pageCount = 0;
    bool m_synchronizingCursors = false;
};
