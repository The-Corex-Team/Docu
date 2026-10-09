#pragma once

#include <QHash>
#include <QMainWindow>
#include <QString>

class QAction;
class Document;
class DocumentController;
class DocumentView;
class Ribbon;
class QCloseEvent;

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void newDocument();
    void openDocument();
    void saveDocument();
    void saveDocumentAs();
    bool saveDocumentTo(const QString &fileName);
    bool confirmDiscardChanges();
    void updateWindowTitle();
    void insertImage();
    void updateFormatControls();

    Document *m_document;
    DocumentController *m_controller;
    Ribbon *m_ribbon;
    DocumentView *m_documentView;
    QHash<QString, QAction *> m_actions;
    QString m_fileName;
};
