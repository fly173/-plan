#pragma once

#include "../core/DesignTypes.h"

#include <QWidget>

class ConstraintPlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConstraintPlotWidget(QWidget* parent = nullptr);

    void setConstraintResult(const ucav::ConstraintResult& result);
    void setSelectedPoint(double wingLoading, double thrustToWeight);

signals:
    void designPointSelected(double wingLoading, double thrustToWeight);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QRectF plotArea() const;
    QPointF toScreen(double wingLoading, double thrustToWeight) const;
    QPointF fromScreen(const QPointF& point) const;

    ucav::ConstraintResult m_result;
    bool m_hasResult = false;
    bool m_hasSelectedPoint = false;
    double m_selectedWingLoading = 0.0;
    double m_selectedThrustToWeight = 0.0;
};
