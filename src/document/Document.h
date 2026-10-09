#pragma once

#include <QObject>

#include "PageSettings.h"

class QTextDocument;

class Document final : public QObject
{
    Q_OBJECT

public:
    explicit Document(QObject *parent = nullptr);

    QTextDocument *textDocument() const;
    const PageSettings &pageSettings() const;
    void setPageSettings(const PageSettings &settings);
    bool isModified() const;
    void setModified(bool modified);

signals:
    void pageSettingsChanged();

private:
    void applyPageSettings();

    QTextDocument *m_textDocument;
    PageSettings m_pageSettings;
};
