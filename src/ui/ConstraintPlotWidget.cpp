#include "ConstraintPlotWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>

ConstraintPlotWidget::ConstraintPlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(620, 420);
    setMouseTracking(true);
}

void ConstraintPlotWidget::setConstraintResult(const ucav::ConstraintResult& result)
{
    m_result = result;
    m_hasResult = true;
    update();
}

void ConstraintPlotWidget::setSelectedPoint(double wingLoading, double thrustToWeight)
{
    m_selectedWingLoading = wingLoading;
    m_selectedThrustToWeight = thrustToWeight;
    m_hasSelectedPoint = true;
    update();
}

QRectF ConstraintPlotWidget::plotArea() const
{
    return QRectF(68, 28, width() - 104, height() - 86);
}

QPointF ConstraintPlotWidget::toScreen(double wingLoading, double thrustToWeight) const
{
    const QRectF area = plotArea();
    const double x = area.left() + (wingLoading - 1500.0) / (9500.0 - 1500.0) * area.width();
    const double y = area.bottom() - (thrustToWeight - 0.0) / 1.2 * area.height();
    return QPointF(x, y);
}

QPointF ConstraintPlotWidget::fromScreen(const QPointF& point) const
{
    const QRectF area = plotArea();
    const double ws = 1500.0 + (point.x() - area.left()) / area.width() * (9500.0 - 1500.0);
    const double tw = (area.bottom() - point.y()) / area.height() * 1.2;
    return QPointF(std::max(1500.0, std::min(9500.0, ws)), std::max(0.0, std::min(1.2, tw)));
}

void ConstraintPlotWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(250, 251, 252));

    const QRectF area = plotArea();
    painter.setPen(QPen(QColor(52, 64, 84), 1));
    painter.drawRect(area);

    painter.setPen(QPen(QColor(220, 225, 232), 1));
    for (int i = 1; i < 8; ++i) {
        const double x = area.left() + i * area.width() / 8.0;
        painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
    }
    for (int i = 1; i < 6; ++i) {
        const double y = area.top() + i * area.height() / 6.0;
        painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }

    painter.setPen(QPen(QColor(35, 45, 60), 1));
    painter.drawText(QRectF(area.left(), area.bottom() + 28, area.width(), 24),
                     Qt::AlignCenter, "翼载荷 W/S (N/m^2)");
    painter.save();
    painter.translate(18, area.center().y());
    painter.rotate(-90);
    painter.drawText(QRectF(-120, -10, 240, 20), Qt::AlignCenter, "推重比 T/W");
    painter.restore();

    if (!m_hasResult) {
        painter.setPen(QColor(100, 116, 139));
        painter.drawText(area, Qt::AlignCenter, "点击“计算总体参数”生成界限线图");
        return;
    }

    if (m_result.landingWingLoadingLimit > 1500.0) {
        const double x = toScreen(m_result.landingWingLoadingLimit, 0.0).x();
        painter.fillRect(QRectF(x, area.top(), std::max(0.0, area.right() - x), area.height()),
                         QColor(255, 228, 230, 120));
    }

    const QColor colors[] = {
        QColor(37, 99, 235), QColor(220, 38, 38), QColor(22, 163, 74),
        QColor(147, 51, 234), QColor(245, 158, 11)
    };

    int colorIndex = 0;
    for (const ucav::ConstraintCurve& curve : m_result.curves) {
        QPainterPath path;
        bool started = false;
        for (const ucav::ConstraintPoint& point : curve.points) {
            const QPointF screen = toScreen(point.wingLoading, std::min(1.2, point.thrustToWeight));
            if (!started) {
                path.moveTo(screen);
                started = true;
            } else {
                path.lineTo(screen);
            }
        }
        const QColor color = colors[colorIndex % 5];
        painter.setPen(QPen(color, 2));
        painter.drawPath(path);
        painter.drawText(QPointF(area.right() - 128, area.top() + 22 + colorIndex * 20),
                         QString::fromStdString(curve.name));
        ++colorIndex;
    }

    if (m_hasSelectedPoint) {
        const QPointF selected = toScreen(m_selectedWingLoading, m_selectedThrustToWeight);
        painter.setPen(QPen(QColor(15, 23, 42), 2));
        painter.setBrush(QColor(15, 23, 42));
        painter.drawEllipse(selected, 5, 5);
        painter.drawText(selected + QPointF(8, -8), "设计点");
    }
}

void ConstraintPlotWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_hasResult || !plotArea().contains(event->pos())) {
        return;
    }

    const QPointF values = fromScreen(event->pos());
    setSelectedPoint(values.x(), values.y());
    emit designPointSelected(values.x(), values.y());
}
