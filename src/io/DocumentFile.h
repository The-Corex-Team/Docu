#pragma once

#include <QString>

class Document;

class DocumentFile final
{
public:
    static QString fileDialogFilter();
    static bool save(const Document &document, const QString &fileName,
                     QString *errorMessage = nullptr);
    static bool load(Document &document, const QString &fileName,
                     QString *errorMessage = nullptr);
};
