#pragma once
#include <QRect>
#include <QPointF>

inline QPointF relativeCursorTarget(const QRect &bounds, QPointF position)
{
    if (bounds.isEmpty()) { return position; }
    const double marginX = qMin(50.0, bounds.width() / 4.0);
    const double marginY = qMin(50.0, bounds.height() / 4.0);
    if (position.x() < bounds.x() + marginX || position.x() > bounds.x() + bounds.width() - marginX
        || position.y() < bounds.y() + marginY || position.y() > bounds.y() + bounds.height() - marginY) {
        return QPointF(bounds.x() + bounds.width() / 2.0, bounds.y() + bounds.height() / 2.0);
    }
    return position;
}
