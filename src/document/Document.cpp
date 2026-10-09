#include "Document.h"

#include <QFont>
#include <QTextDocument>

Document::Document(QObject *parent)
    : QObject(parent),
      m_textDocument(new QTextDocument(this))
{
    m_textDocument->setDefaultFont(QFont(QStringLiteral("Arial"), 11));
    m_textDocument->setUndoRedoEnabled(true);
    applyPageSettings();
    m_textDocument->setModified(false);
}

QTextDocument *Document::textDocument() const
{
    return m_textDocument;
}

const PageSettings &Document::pageSettings() const
{
    return m_pageSettings;
}

void Document::setPageSettings(const PageSettings &settings)
{
    m_pageSettings = settings;
    applyPageSettings();
    emit pageSettingsChanged();
}

bool Document::isModified() const
{
    return m_textDocument->isModified();
}

void Document::setModified(bool modified)
{
    m_textDocument->setModified(modified);
}

void Document::applyPageSettings()
{
    m_textDocument->setDocumentMargin(0.0);
    m_textDocument->setPageSize(m_pageSettings.contentSizeLogical());
}
