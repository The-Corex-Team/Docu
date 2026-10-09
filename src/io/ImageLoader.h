#pragma once

#include <QImage>
#include <QString>
#include <QSizeF>

class ImageLoader final
{
public:
    static QString fileDialogFilter();
    static QImage load(const QString &fileName,
                       const QSizeF &maximumLogicalSize,
                       QString *errorMessage = nullptr);
};
