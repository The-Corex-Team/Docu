#pragma once

#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QSizeF>

class PageSettings
{
public:
    PageSettings();

    QPageSize paperSize() const;
    bool setPaperSize(const QPageSize &size);
    bool setCustomSizeMillimeters(const QSizeF &size);

    QPageLayout::Orientation orientation() const;
    bool setOrientation(QPageLayout::Orientation orientation);

    QMarginsF marginsMillimeters() const;
    bool setMarginsMillimeters(const QMarginsF &margins);

    QSizeF pageSizePoints() const;
    QSizeF pageSizeLogical() const;
    QMarginsF marginsLogical() const;
    QSizeF contentSizeLogical() const;

private:
    bool marginsFitPaper() const;

    QPageSize m_paperSize;
    QPageLayout::Orientation m_orientation;
    QMarginsF m_marginsMillimeters;
};
