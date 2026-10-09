#pragma once

#include <QString>
#include <Qt>
#include <QTextCursor>

class Document;
class DocumentView;
class QImage;
class QTextBlockFormat;
class QTextCharFormat;
class QTextCursor;
class QTextImageFormat;

class DocumentController final
{
public:
    DocumentController(Document *document, DocumentView *view);

    void cut();
    void copy();
    void paste();
    bool insertImage(const QImage &image);
    QTextImageFormat selectedImageFormat() const;
    bool resizeSelectedImage(qreal width, qreal height);
    void beginImageResize(int position);
    bool resizeImageAtPosition(int position, qreal width, qreal height);
    void finishImageResize();
    void beginImageMove(int position);
    bool moveImageAtPosition(int position, const QPointF &scenePosition);
    void finishImageMove();
    void undo();
    void redo();

    void toggleBold();
    void toggleItalic();
    void toggleUnderline();
    void setFontFamily(const QString &family);
    void setFontSize(double pointSize);
    void setAlignment(Qt::Alignment alignment);

private:
    QTextCursor currentCursor() const;
    QTextCursor selectedImageCursor() const;
    QTextCursor imageCursorAtPosition(int position) const;
    void mergeFormat(const QTextCharFormat &format);

    Document *m_document;
    DocumentView *m_view;
    QTextCursor m_imageResizeCursor;
    QTextCursor m_imageMoveCursor;
};
