#include "icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace Icons {

namespace {

QPainterPath gearPath() {
    constexpr int teeth = 8;
    constexpr double cx = 12.0, cy = 12.0;
    constexpr double outer = 9.4, inner = 7.2;
    const double step = 2.0 * M_PI / teeth;
    QPainterPath path;
    for (int i = 0; i < teeth; ++i) {
        const double a = i * step;
        const QPointF pts[4] = {
            {cx + inner * qCos(a - 0.30), cy + inner * qSin(a - 0.30)},
            {cx + outer * qCos(a - 0.17), cy + outer * qSin(a - 0.17)},
            {cx + outer * qCos(a + 0.17), cy + outer * qSin(a + 0.17)},
            {cx + inner * qCos(a + 0.30), cy + inner * qSin(a + 0.30)},
        };
        if (i == 0) path.moveTo(pts[0]); else path.lineTo(pts[0]);
        path.lineTo(pts[1]);
        path.lineTo(pts[2]);
        path.lineTo(pts[3]);
    }
    path.closeSubpath();
    return path;
}

}  // namespace

QPixmap pixmap(Kind kind, const QColor& color, int size, qreal dpr) {
    QPixmap pm(QSize(qRound(size * dpr), qRound(size * dpr)));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 24.0, size / 24.0);
    p.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    switch (kind) {
    case Kind::Gear:
        p.drawPath(gearPath());
        p.drawEllipse(QPointF(12, 12), 3.2, 3.2);
        break;
    case Kind::Refresh: {
        // Open circular arrow: ~290 degree arc with a filled arrow head at its end.
        const QRectF r(4.5, 4.5, 15, 15);
        QPainterPath arc;
        arc.arcMoveTo(r, 40);
        arc.arcTo(r, 40, 290);
        p.drawPath(arc);
        const double th = qDegreesToRadians(330.0);              // end of the arc
        const QPointF end(12 + 7.5 * qCos(th), 12 - 7.5 * qSin(th));
        const double tx = -qSin(th), ty = -qCos(th);             // direction of travel (screen coords)
        const double nx = -ty, ny = tx;                          // normal
        QPainterPath head;
        head.moveTo(end + QPointF(tx * 3.2, ty * 3.2));
        head.lineTo(end + QPointF(nx * 3.0, ny * 3.0));
        head.lineTo(end - QPointF(nx * 3.0, ny * 3.0));
        head.closeSubpath();
        p.setBrush(color);
        p.drawPath(head);
        p.setBrush(Qt::NoBrush);
        break;
    }
    case Kind::Monitor:
        p.drawRoundedRect(QRectF(3, 4, 18, 12), 2, 2);
        p.drawLine(QPointF(12, 16), QPointF(12, 20));
        p.drawLine(QPointF(8, 20), QPointF(16, 20));
        break;
    case Kind::Sun:
        p.drawEllipse(QPointF(12, 12), 3.8, 3.8);
        for (int i = 0; i < 8; ++i) {
            const double a = qDegreesToRadians(i * 45.0);
            p.drawLine(QPointF(12 + 7.2 * qCos(a), 12 + 7.2 * qSin(a)),
                       QPointF(12 + 9.6 * qCos(a), 12 + 9.6 * qSin(a)));
        }
        break;
    case Kind::Plus:
        p.drawLine(QPointF(12, 5), QPointF(12, 19));
        p.drawLine(QPointF(5, 12), QPointF(19, 12));
        break;
    case Kind::Trash: {
        p.drawLine(QPointF(4.5, 7), QPointF(19.5, 7));
        p.drawLine(QPointF(9, 7), QPointF(9, 4.5));
        p.drawLine(QPointF(9, 4.5), QPointF(15, 4.5));
        p.drawLine(QPointF(15, 4.5), QPointF(15, 7));
        QPainterPath body;
        body.moveTo(6.5, 7);
        body.lineTo(7.5, 19.5);
        body.lineTo(16.5, 19.5);
        body.lineTo(17.5, 7);
        p.drawPath(body);
        break;
    }
    }
    return pm;
}

QIcon icon(Kind kind, const QColor& color) {
    QIcon ic;
    ic.addPixmap(pixmap(kind, color, 96, 1.0));
    return ic;
}

QIcon appIcon() {
    QIcon fallback;
    for (int s : {16, 24, 32, 48, 64, 128, 256, 512})
        fallback.addFile(QStringLiteral(":/icons/displaybuddy-%1.png").arg(s), QSize(s, s));
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    // Prefer an installed theme icon (hicolor/.../displaybuddy.png), fall back to the bundled one.
    return QIcon::fromTheme(QStringLiteral("displaybuddy"), fallback);
#else
    return fallback;
#endif
}

}  // namespace Icons
