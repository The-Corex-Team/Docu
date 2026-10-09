#include "document/Document.h"
#include "document/ImageLayerFormat.h"
#include "io/DocumentFile.h"

#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QUrl>

#include <cstdio>

namespace {

bool findImage(Document &document, QTextImageFormat *imageFormat,
               QTextCharFormat *characterFormat = nullptr)
{
    QTextDocument *textDocument = document.textDocument();
    for (QTextBlock block = textDocument->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid() || !fragment.charFormat().isImageFormat())
                continue;

            *imageFormat = fragment.charFormat().toImageFormat();
            if (characterFormat)
                *characterFormat = fragment.charFormat();
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid())
        return 1;

    Document original;
    PageSettings pageSettings;
    const bool customPageSizeSet =
        pageSettings.setCustomSizeMillimeters(QSizeF(210.0, 297.0));
    const bool landscapeSet =
        customPageSizeSet && pageSettings.setOrientation(QPageLayout::Landscape);
    const bool marginsSet =
        landscapeSet
        && pageSettings.setMarginsMillimeters(QMarginsF(12.0, 14.0, 16.0, 18.0));
    original.setPageSettings(pageSettings);

    QTextCursor cursor(original.textDocument());
    cursor.insertText(QStringLiteral("Native Docu round trip"));
    const QString resourceName = QStringLiteral("docu-image:test-resource");
    QImage source(48, 32, QImage::Format_ARGB32);
    source.fill(QColor(22, 144, 201));
    original.textDocument()->addResource(
        QTextDocument::ImageResource, QUrl(resourceName), source);

    QTextImageFormat image;
    image.setName(resourceName);
    image.setWidth(1.0);
    image.setHeight(1.0);
    cursor.insertImage(image);
    const int imagePosition = cursor.position() - 1;
    QTextCursor imageCursor(original.textDocument());
    imageCursor.setPosition(imagePosition);
    imageCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QTextCharFormat imageProperties = imageCursor.charFormat();
    imageProperties.setProperty(ImageLayerFormat::IsLayeredImage, true);
    imageProperties.setProperty(ImageLayerFormat::SourceWidth, 123.0);
    imageProperties.setProperty(ImageLayerFormat::SourceHeight, 82.0);
    imageProperties.setProperty(ImageLayerFormat::PositionX, 45.0);
    imageProperties.setProperty(ImageLayerFormat::PositionY, 67.0);
    imageCursor.mergeCharFormat(imageProperties);

    const QString fileName =
        temporaryDirectory.filePath(QStringLiteral("round-trip.docu"));
    QString errorMessage;
    const bool saved = DocumentFile::save(original, fileName, &errorMessage);

    Document loaded;
    const bool loadedSuccessfully =
        saved && DocumentFile::load(loaded, fileName, &errorMessage);
    QTextImageFormat loadedImage;
    QTextCharFormat loadedProperties;
    const bool imagePreserved =
        loadedSuccessfully && findImage(loaded, &loadedImage, &loadedProperties);
    const QImage restoredImage = imagePreserved
        ? loaded.textDocument()
              ->resource(QTextDocument::ImageResource, QUrl(loadedImage.name()))
              .value<QImage>()
        : QImage();
    const bool embeddedImagePreserved =
        restoredImage.size() == source.size()
        && restoredImage.pixelColor(0, 0) == source.pixelColor(0, 0);
    const bool imageLayerPreserved =
        imagePreserved
        && loadedProperties.property(ImageLayerFormat::IsLayeredImage).toBool()
        && qAbs(loadedProperties.property(ImageLayerFormat::SourceWidth).toReal()
                - 123.0) < 0.1
        && qAbs(loadedProperties.property(ImageLayerFormat::SourceHeight).toReal()
                - 82.0) < 0.1
        && qAbs(loadedProperties.property(ImageLayerFormat::PositionX).toReal()
                - 45.0) < 0.1
        && qAbs(loadedProperties.property(ImageLayerFormat::PositionY).toReal()
                - 67.0) < 0.1;
    const QSizeF originalPage = original.pageSettings().pageSizeLogical();
    const QSizeF loadedPage = loaded.pageSettings().pageSizeLogical();
    const QMarginsF originalMargins = original.pageSettings().marginsMillimeters();
    const QMarginsF loadedMargins = loaded.pageSettings().marginsMillimeters();
    const bool pageSettingsPreserved =
        loadedSuccessfully
        && loaded.pageSettings().orientation() == QPageLayout::Landscape
        && qAbs(originalPage.width() - loadedPage.width()) < 1.0
        && qAbs(originalPage.height() - loadedPage.height()) < 1.0
        && qAbs(originalMargins.left() - loadedMargins.left()) < 0.01
        && qAbs(originalMargins.bottom() - loadedMargins.bottom()) < 0.01;
    const bool contentPreserved =
        loadedSuccessfully
        && loaded.textDocument()->toPlainText()
               == original.textDocument()->toPlainText();
    const bool loadedAsUnmodified = loadedSuccessfully && !loaded.isModified();

    const QString invalidPath =
        temporaryDirectory.filePath(QStringLiteral("invalid.docu"));
    QFile invalidFile(invalidPath);
    const bool invalidFileWritten =
        invalidFile.open(QIODevice::WriteOnly)
        && invalidFile.write("{not-json") == 9;
    invalidFile.close();
    const QString contentBeforeInvalidLoad =
        loaded.textDocument()->toPlainText();
    const bool invalidFileRejected =
        invalidFileWritten
        && !DocumentFile::load(loaded, invalidPath, &errorMessage)
        && loaded.textDocument()->toPlainText() == contentBeforeInvalidLoad;

    std::printf("save/load: %s/%s; text/image/layer/page: %s/%s/%s/%s; "
                "clean state/invalid rejection: %s/%s\n",
                saved ? "ok" : "failed",
                loadedSuccessfully ? "ok" : "failed",
                contentPreserved ? "ok" : "failed",
                embeddedImagePreserved ? "ok" : "failed",
                imageLayerPreserved ? "ok" : "failed",
                pageSettingsPreserved ? "ok" : "failed",
                loadedAsUnmodified ? "ok" : "failed",
                invalidFileRejected ? "ok" : "failed");

    return saved && loadedSuccessfully && contentPreserved
                   && embeddedImagePreserved && imageLayerPreserved
                   && pageSettingsPreserved && loadedAsUnmodified
                   && invalidFileRejected
               ? 0 : 1;
}
