#include "DocumentFile.h"

#include "document/Document.h"
#include "document/ImageLayerFormat.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPageSize>
#include <QSaveFile>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QUrl>

namespace {
constexpr int FormatVersion = 1;
constexpr qint64 MaximumFileSize = 128 * 1024 * 1024;

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage)
        *errorMessage = message;
}

bool imageData(const QImage &image, QByteArray *data)
{
    if (image.isNull())
        return false;

    QBuffer buffer(data);
    if (!buffer.open(QIODevice::WriteOnly))
        return false;
    return image.save(&buffer, "PNG");
}

bool loadPageSettings(const QJsonObject &object, PageSettings *settings)
{
    const QJsonObject page = object.value(QStringLiteral("page")).toObject();
    const QJsonArray size = page.value(QStringLiteral("sizeMm")).toArray();
    const QJsonArray margins = page.value(QStringLiteral("marginsMm")).toArray();
    const int orientationValue = page.value(QStringLiteral("orientation")).toInt(-1);
    if (size.size() != 2 || margins.size() != 4
        || orientationValue < QPageLayout::Portrait
        || orientationValue > QPageLayout::Landscape) {
        return false;
    }

    const QSizeF sizeMm(size[0].toDouble(-1.0), size[1].toDouble(-1.0));
    if (!settings->setCustomSizeMillimeters(sizeMm))
        return false;

    const auto orientation =
        static_cast<QPageLayout::Orientation>(orientationValue);
    if (!settings->setOrientation(orientation))
        return false;

    return settings->setMarginsMillimeters(
        QMarginsF(margins[0].toDouble(-1.0), margins[1].toDouble(-1.0),
                  margins[2].toDouble(-1.0), margins[3].toDouble(-1.0)));
}

QJsonObject savePageSettings(const PageSettings &settings)
{
    QSizeF size = settings.paperSize().size(QPageSize::Millimeter);
    if (settings.orientation() == QPageLayout::Landscape
        && size.width() < size.height()) {
        size.transpose();
    } else if (settings.orientation() == QPageLayout::Portrait
               && size.width() > size.height()) {
        size.transpose();
    }

    const QMarginsF margins = settings.marginsMillimeters();
    QJsonObject page;
    page.insert(QStringLiteral("sizeMm"),
                QJsonArray{size.width(), size.height()});
    page.insert(QStringLiteral("orientation"),
                static_cast<int>(settings.orientation()));
    page.insert(QStringLiteral("marginsMm"),
                QJsonArray{margins.left(), margins.top(),
                           margins.right(), margins.bottom()});
    return page;
}

bool prepareDocument(const QJsonObject &root, QTextDocument *preparedDocument,
                     PageSettings *pageSettings, QString *errorMessage)
{
    if (root.value(QStringLiteral("formatVersion")).toInt(-1) != FormatVersion) {
        setError(errorMessage, QStringLiteral("Unsupported Docu file version."));
        return false;
    }

    const QString html = root.value(QStringLiteral("html")).toString();
    if (html.isEmpty()) {
        setError(errorMessage, QStringLiteral("The file does not contain document content."));
        return false;
    }

    if (!loadPageSettings(root, pageSettings)) {
        setError(errorMessage, QStringLiteral("The file contains invalid page settings."));
        return false;
    }

    const QJsonObject images = root.value(QStringLiteral("images")).toObject();
    QString resolvedHtml = html;
    for (auto it = images.constBegin(); it != images.constEnd(); ++it) {
        const QByteArray encoded = it.value().toString().toLatin1();
        const QByteArray bytes = QByteArray::fromBase64(
            encoded, QByteArray::AbortOnBase64DecodingErrors);
        QImage image;
        if (bytes.isEmpty() || !image.loadFromData(bytes)) {
            setError(errorMessage, QStringLiteral("The file contains invalid image data."));
            return false;
        }

        const QString dataUrl = QStringLiteral("data:image/png;base64,%1")
                                    .arg(QString::fromLatin1(encoded));
        const QString originalSource = it.key();
        resolvedHtml.replace(QStringLiteral("src=\"%1\"").arg(originalSource),
                             QStringLiteral("src=\"%1\"").arg(dataUrl));
        resolvedHtml.replace(QStringLiteral("src='%1'").arg(originalSource),
                             QStringLiteral("src='%1'").arg(dataUrl));
    }

    preparedDocument->setHtml(resolvedHtml);
    const QJsonArray layers = root.value(QStringLiteral("imageLayers")).toArray();
    for (const QJsonValue &value : layers) {
        if (!value.isObject()) {
            setError(errorMessage, QStringLiteral("The file contains invalid image metadata."));
            return false;
        }

        const QJsonObject layer = value.toObject();
        const int position = layer.value(QStringLiteral("position")).toInt(-1);
        const double width = layer.value(QStringLiteral("width")).toDouble(-1.0);
        const double height = layer.value(QStringLiteral("height")).toDouble(-1.0);
        const double x = layer.value(QStringLiteral("x")).toDouble(-1.0);
        const double y = layer.value(QStringLiteral("y")).toDouble(-1.0);
        if (position < 0 || position >= preparedDocument->characterCount() - 1
            || width <= 0.0 || height <= 0.0 || x < 0.0 || y < 0.0) {
            setError(errorMessage, QStringLiteral("The file contains invalid image metadata."));
            return false;
        }

        QTextCursor cursor(preparedDocument);
        cursor.setPosition(position);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        if (!cursor.charFormat().isImageFormat()) {
            setError(errorMessage, QStringLiteral("Image metadata does not match document content."));
            return false;
        }

        QTextCharFormat format = cursor.charFormat();
        format.setProperty(ImageLayerFormat::IsLayeredImage, true);
        format.setProperty(ImageLayerFormat::SourceWidth, width);
        format.setProperty(ImageLayerFormat::SourceHeight, height);
        format.setProperty(ImageLayerFormat::PositionX, x);
        format.setProperty(ImageLayerFormat::PositionY, y);
        cursor.mergeCharFormat(format);
    }

    return true;
}
}

QString DocumentFile::fileDialogFilter()
{
    return QStringLiteral("Docu documents (*.docu)");
}

bool DocumentFile::save(const Document &document, const QString &fileName,
                        QString *errorMessage)
{
    QJsonObject root;
    root.insert(QStringLiteral("formatVersion"), FormatVersion);
    root.insert(QStringLiteral("page"), savePageSettings(document.pageSettings()));

    QString html = document.textDocument()->toHtml();
    QJsonObject images;
    QJsonArray imageLayers;
    QTextDocument *textDocument = document.textDocument();
    for (QTextBlock block = textDocument->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid() || !fragment.charFormat().isImageFormat())
                continue;

            const QTextCharFormat charFormat = fragment.charFormat();
            const QTextImageFormat imageFormat = charFormat.toImageFormat();
            const QUrl imageUrl(imageFormat.name());
            const QImage image = textDocument
                                     ->resource(QTextDocument::ImageResource, imageUrl)
                                     .value<QImage>();
            QByteArray png;
            if (image.isNull() || !imageData(image, &png)) {
                setError(errorMessage, QStringLiteral("An image in the document could not be saved."));
                return false;
            }

            const QString encoded = QString::fromLatin1(png.toBase64());
            images.insert(imageFormat.name(), encoded);
            const QString dataUrl = QStringLiteral("data:image/png;base64,%1").arg(encoded);
            html.replace(QStringLiteral("src=\"%1\"").arg(imageFormat.name()),
                         QStringLiteral("src=\"%1\"").arg(dataUrl));
            html.replace(QStringLiteral("src='%1'").arg(imageFormat.name()),
                         QStringLiteral("src='%1'").arg(dataUrl));

            if (charFormat.property(ImageLayerFormat::IsLayeredImage).toBool()) {
                imageLayers.append(QJsonObject{
                    {QStringLiteral("position"), fragment.position()},
                    {QStringLiteral("width"),
                     charFormat.property(ImageLayerFormat::SourceWidth).toDouble()},
                    {QStringLiteral("height"),
                     charFormat.property(ImageLayerFormat::SourceHeight).toDouble()},
                    {QStringLiteral("x"),
                     charFormat.property(ImageLayerFormat::PositionX).toDouble()},
                    {QStringLiteral("y"),
                     charFormat.property(ImageLayerFormat::PositionY).toDouble()}
                });
            }
        }
    }
    root.insert(QStringLiteral("html"), html);
    root.insert(QStringLiteral("images"), images);
    root.insert(QStringLiteral("imageLayers"), imageLayers);

    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (data.size() > MaximumFileSize) {
        setError(errorMessage, QStringLiteral("The document is too large to save."));
        return false;
    }

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(errorMessage, file.errorString());
        return false;
    }
    if (file.write(data) != data.size()) {
        setError(errorMessage, file.errorString());
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        setError(errorMessage, file.errorString());
        return false;
    }

    if (errorMessage)
        errorMessage->clear();
    return true;
}

bool DocumentFile::load(Document &document, const QString &fileName,
                        QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, file.errorString());
        return false;
    }
    if (file.size() > MaximumFileSize) {
        setError(errorMessage, QStringLiteral("The file is too large to open."));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        setError(errorMessage, QStringLiteral("The file is not a valid Docu document."));
        return false;
    }

    QTextDocument preparedDocument;
    PageSettings settings;
    if (!prepareDocument(json.object(), &preparedDocument, &settings, errorMessage))
        return false;

    document.setPageSettings(settings);
    document.textDocument()->setHtml(preparedDocument.toHtml());
    for (QTextBlock block = document.textDocument()->begin();
         block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid() || !fragment.charFormat().isImageFormat())
                continue;
            QTextCursor cursor(document.textDocument());
            cursor.setPosition(fragment.position());
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(fragment.charFormat());
        }
    }
    document.textDocument()->clearUndoRedoStacks();
    document.setModified(false);
    if (errorMessage)
        errorMessage->clear();
    return true;
}
