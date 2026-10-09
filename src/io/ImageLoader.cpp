#include "ImageLoader.h"

#include <QCoreApplication>
#include <QImageReader>
#include <QStringList>
#include <QtMath>

namespace {
constexpr qint64 MaximumImagePixels = 50'000'000;
constexpr qreal DecodeScale = 3.0;

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage)
        *errorMessage = message;
}

bool exceedsPixelLimit(const QSize &size)
{
    return size.isValid()
        && qint64(size.width()) * qint64(size.height()) > MaximumImagePixels;
}
}

QString ImageLoader::fileDialogFilter()
{
    QStringList patterns;
    for (const QByteArray &format : QImageReader::supportedImageFormats())
        patterns.append(QStringLiteral("*.%1").arg(QString::fromLatin1(format)));

    if (patterns.isEmpty())
        return QStringLiteral("All Files (*)");

    return QCoreApplication::translate("ImageLoader", "Images (%1);;All Files (*)")
        .arg(patterns.join(QLatin1Char(' ')));
}

QImage ImageLoader::load(const QString &fileName,
                         const QSizeF &maximumLogicalSize,
                         QString *errorMessage)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);

    const QSize sourceSize = reader.size();
    if (exceedsPixelLimit(sourceSize)) {
        setError(errorMessage,
                 QCoreApplication::translate(
                     "ImageLoader", "This image is too large to open safely."));
        return {};
    }

    if (sourceSize.isValid()
        && maximumLogicalSize.width() > 0.0
        && maximumLogicalSize.height() > 0.0) {
        const QSize maximumDecodeSize(
            qMax(1, qRound(maximumLogicalSize.width() * DecodeScale)),
            qMax(1, qRound(maximumLogicalSize.height() * DecodeScale)));
        if (sourceSize.width() > maximumDecodeSize.width()
            || sourceSize.height() > maximumDecodeSize.height()) {
            reader.setScaledSize(
                sourceSize.scaled(maximumDecodeSize, Qt::KeepAspectRatio));
        }
    }

    QImage image = reader.read();
    if (image.isNull()) {
        setError(errorMessage, reader.errorString());
        return {};
    }

    if (exceedsPixelLimit(image.size())) {
        setError(errorMessage,
                 QCoreApplication::translate(
                     "ImageLoader", "This image is too large to insert."));
        return {};
    }

    if (errorMessage)
        errorMessage->clear();
    return image;
}
