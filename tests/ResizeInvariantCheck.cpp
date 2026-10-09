#include "document/Document.h"
#include "document/PageSettings.h"
#include "gui/Editor/DocumentView.h"

#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextLayout>
#include <QtMath>

#include <cstdio>
#include <algorithm>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    PageSettings settings;
    const QSizeF portraitSize = settings.pageSizeLogical();
    const bool defaultsAreA4Portrait =
        portraitSize.width() < portraitSize.height()
        && qAbs(portraitSize.width() - 793.7) < 1.0
        && qAbs(portraitSize.height() - 1122.5) < 1.0;
    const bool orientationWorks =
        settings.setOrientation(QPageLayout::Landscape)
        && settings.pageSizeLogical().width() > settings.pageSizeLogical().height();
    const bool customSizeWorks =
        settings.setCustomSizeMillimeters(QSizeF(210.0, 297.0))
        && qAbs(settings.pageSizeLogical().width() - 1122.5) < 1.0;
    const bool marginsAreValidated =
        !settings.setMarginsMillimeters(QMarginsF(200.0, 200.0, 200.0, 200.0))
        && settings.setMarginsMillimeters(QMarginsF(12.0, 18.0, 12.0, 18.0));
    Document settingsDocument;
    PageSettings landscapeSettings = settingsDocument.pageSettings();
    bool documentUsesPageSettings =
        landscapeSettings.setOrientation(QPageLayout::Landscape);
    if (documentUsesPageSettings) {
        settingsDocument.setPageSettings(landscapeSettings);
        documentUsesPageSettings =
            settingsDocument.textDocument()->pageSize().width()
            > settingsDocument.textDocument()->pageSize().height();
    }

    Document document;
    document.textDocument()->setPlainText(QString(12000, QLatin1Char('W')));

    DocumentView view(&document);
    view.resize(1100, 760);
    view.show();
    application.processEvents();

    QTextEdit *editor = view.activeEditor();
    const int initialEditorWidth = editor->width();
    const int initialPageWidth =
        qRound(document.textDocument()->pageSize().width());
    const int initialLineCount =
        document.textDocument()->begin().layout()->lineCount();
    const int initialPageCount = document.textDocument()->pageCount();

    view.resize(720, 500);
    application.processEvents();
    view.setZoom(1.1);
    application.processEvents();

    const int finalEditorWidth = editor->width();
    const int finalPageWidth =
        qRound(document.textDocument()->pageSize().width());
    const int finalLineCount =
        document.textDocument()->begin().layout()->lineCount();
    const int finalPageCount = document.textDocument()->pageCount();

    QList<QGraphicsProxyWidget *> pageProxies;
    for (QGraphicsItem *item : view.scene()->items()) {
        if (auto *proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(item))
            pageProxies.append(proxy);
    }
    std::sort(pageProxies.begin(), pageProxies.end(),
              [](const QGraphicsProxyWidget *left,
                 const QGraphicsProxyWidget *right) {
                  return left->pos().y() < right->pos().y();
              });
    bool pageOffsetsMatch = pageProxies.size() == finalPageCount;
    const qreal contentPageHeight = document.textDocument()->pageSize().height();
    const qreal physicalPageHeight =
        document.pageSettings().pageSizeLogical().height();
    for (qsizetype index = 0; pageOffsetsMatch && index < pageProxies.size(); ++index) {
        const auto *pageEditor = qobject_cast<QTextEdit *>(pageProxies[index]->widget());
        const int actualOffset =
            pageEditor ? pageEditor->verticalScrollBar()->value() : -1;
        const int expectedOffset = qRound(index * contentPageHeight);
        pageOffsetsMatch = actualOffset == expectedOffset;
        if (!pageOffsetsMatch)
            std::printf("page %lld scroll offset: %d (expected %d)\n",
                        static_cast<long long>(index), actualOffset, expectedOffset);
    }
    bool pageGeometryMatches = pageProxies.size() == finalPageCount;
    constexpr qreal PageGap = 24.0;
    for (qsizetype index = 1;
         pageGeometryMatches && index < pageProxies.size(); ++index) {
        const qreal actualDistance =
            pageProxies[index]->pos().y() - pageProxies[index - 1]->pos().y();
        pageGeometryMatches =
            qAbs(actualDistance - (physicalPageHeight + PageGap)) < 1.0;
    }

    PageSettings viewLandscape = document.pageSettings();
    bool viewUpdatesPageSettings =
        viewLandscape.setOrientation(QPageLayout::Landscape);
    if (viewUpdatesPageSettings) {
        document.setPageSettings(viewLandscape);
        application.processEvents();
        viewUpdatesPageSettings =
            document.textDocument()->pageSize().width()
                > document.textDocument()->pageSize().height()
            && view.pageCount() == document.textDocument()->pageCount()
            && qAbs(view.scene()->sceneRect().width()
                    - document.pageSettings().pageSizeLogical().width()) < 1.0;
    }

    std::printf("editor width: %d -> %d; page width: %d -> %d; "
                "first-block lines: %d -> %d; pages: %d -> %d; "
                "page offsets/gaps: %s/%s; settings: %s/%s/%s\n",
                initialEditorWidth, finalEditorWidth,
                initialPageWidth, finalPageWidth,
                initialLineCount, finalLineCount,
                initialPageCount, finalPageCount,
                pageOffsetsMatch ? "ok" : "mismatch",
                pageGeometryMatches ? "ok" : "mismatch",
                defaultsAreA4Portrait && orientationWorks
                        && customSizeWorks && marginsAreValidated
                    ? "ok" : "mismatch",
                documentUsesPageSettings ? "applied" : "not applied",
                viewUpdatesPageSettings ? "view updated" : "view stale");

    return initialEditorWidth == finalEditorWidth
                   && initialPageWidth == finalPageWidth
                   && initialLineCount == finalLineCount
                   && initialPageCount == finalPageCount
                   && initialPageCount > 1
                   && pageOffsetsMatch
                   && pageGeometryMatches
                   && defaultsAreA4Portrait
                   && orientationWorks
                   && customSizeWorks
                   && marginsAreValidated
                   && documentUsesPageSettings
                   && viewUpdatesPageSettings
               ? 0
               : 1;
}
