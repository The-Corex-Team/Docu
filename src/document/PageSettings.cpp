#include "PageSettings.h"

#include <QtMath>

namespace {
constexpr qreal PointsPerInch = 72.0;
constexpr qreal LogicalDpi = 96.0;
constexpr qreal MillimetersPerInch = 25.4;
}

PageSettings::PageSettings()
    : m_paperSize(QPageSize::A4),
      m_orientation(QPageLayout::Portrait),
      m_marginsMillimeters(15.0, 15.0, 15.0, 15.0)
{
}

QPageSize PageSettings::paperSize() const
{
    return m_paperSize;
}

bool PageSettings::setPaperSize(const QPageSize &size)
{
    if (!size.isValid())
        return false;

    const QPageSize previousSize = m_paperSize;
    m_paperSize = size;
    if (!marginsFitPaper()) {
        m_paperSize = previousSize;
        return false;
    }
    return true;
}

bool PageSettings::setCustomSizeMillimeters(const QSizeF &size)
{
    if (size.width() <= 0.0 || size.height() <= 0.0)
        return false;

    const QPageSize customSize(size, QPageSize::Millimeter,
                               QStringLiteral("Custom"),
                               QPageSize::ExactMatch);
    if (!customSize.isValid())
        return false;

    const QPageSize previousSize = m_paperSize;
    m_paperSize = customSize;
    if (!marginsFitPaper()) {
        m_paperSize = previousSize;
        return false;
    }
    return true;
}

QPageLayout::Orientation PageSettings::orientation() const
{
    return m_orientation;
}

bool PageSettings::setOrientation(QPageLayout::Orientation orientation)
{
    const QPageLayout::Orientation previousOrientation = m_orientation;
    m_orientation = orientation;
    if (!marginsFitPaper()) {
        m_orientation = previousOrientation;
        return false;
    }
    return true;
}

QMarginsF PageSettings::marginsMillimeters() const
{
    return m_marginsMillimeters;
}

bool PageSettings::setMarginsMillimeters(const QMarginsF &margins)
{
    if (margins.left() < 0.0 || margins.top() < 0.0
        || margins.right() < 0.0 || margins.bottom() < 0.0) {
        return false;
    }

    const QMarginsF previousMargins = m_marginsMillimeters;
    m_marginsMillimeters = margins;
    if (!marginsFitPaper()) {
        m_marginsMillimeters = previousMargins;
        return false;
    }
    return true;
}

QSizeF PageSettings::pageSizePoints() const
{
    QSizeF size = m_paperSize.size(QPageSize::Point);
    if (m_orientation == QPageLayout::Landscape && size.width() < size.height())
        size.transpose();
    else if (m_orientation == QPageLayout::Portrait && size.width() > size.height())
        size.transpose();
    return size;
}

QSizeF PageSettings::pageSizeLogical() const
{
    const qreal scale = LogicalDpi / PointsPerInch;
    const QSizeF points = pageSizePoints();
    return QSizeF(points.width() * scale, points.height() * scale);
}

QMarginsF PageSettings::marginsLogical() const
{
    const qreal scale = LogicalDpi / MillimetersPerInch;
    return QMarginsF(m_marginsMillimeters.left() * scale,
                     m_marginsMillimeters.top() * scale,
                     m_marginsMillimeters.right() * scale,
                     m_marginsMillimeters.bottom() * scale);
}

QSizeF PageSettings::contentSizeLogical() const
{
    const QSizeF page = pageSizeLogical();
    const QMarginsF margins = marginsLogical();
    return QSizeF(qMax(1.0, page.width() - margins.left() - margins.right()),
                  qMax(1.0, page.height() - margins.top() - margins.bottom()));
}

bool PageSettings::marginsFitPaper() const
{
    QSizeF size = m_paperSize.size(QPageSize::Millimeter);
    if (m_orientation == QPageLayout::Landscape && size.width() < size.height())
        size.transpose();
    else if (m_orientation == QPageLayout::Portrait && size.width() > size.height())
        size.transpose();

    return size.width() > m_marginsMillimeters.left() + m_marginsMillimeters.right()
        && size.height() > m_marginsMillimeters.top() + m_marginsMillimeters.bottom();
}
