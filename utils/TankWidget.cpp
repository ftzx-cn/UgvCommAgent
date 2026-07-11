#include "TankWidget.h"
#include <QtMath>

TankWidget::TankWidget(QWidget *parent) : QWidget(parent) { setMinimumSize(100, 200); }

TankWidget::~TankWidget() {}

void TankWidget::setPercentage(double percentage) {
    if (percentage < 0.0)
        percentage = 0.0;
    if (percentage > 100.0)
        percentage = 100.0;

    if (qAbs(m_currentPercentage - percentage) > 0.1) {
        m_currentPercentage = percentage;
        m_currentHeight = m_minHeight + ((m_maxHeight - m_minHeight) / 100) * m_currentPercentage;
        update();
    }
}

void TankWidget::setTemperature(double temp) {
    if (qAbs(m_currentTemperature - temp) > 0.1) {
        m_currentTemperature = temp;
        update();
    }
}

void TankWidget::setPressure(double pressure) {
    if (qAbs(m_currentPressure - pressure) > 0.01) {
        m_currentPressure = pressure;
        update();
    }
}

void TankWidget::setShape(TankShape shape) {
    if (m_shape != shape) {
        m_shape = shape;
        update();
    }
}

void TankWidget::setCapacity(double capacity, const QString &unit) {
    m_capacity = capacity;
    m_capacityUnit = unit;
    update();
}

void TankWidget::setLiquidColor(const QColor &color) {
    m_liquidColor = color;
    update();
}

void TankWidget::setUsedValue(bool percentage, bool temp, bool pressure) {
    m_isUsedPercentage = percentage;
    m_isUsedTemp = temp;
    m_isUsedPressure = pressure;
}

void TankWidget::setHeightConfig(double minHeight, double maxHeight, QString unit = "cm") {
    if (minHeight < maxHeight) {
        m_minHeight = minHeight;
        m_maxHeight = maxHeight;
        // 裁剪当前高度
        if (m_currentHeight < m_minHeight)
            m_currentHeight = m_minHeight;
        if (m_currentHeight > m_maxHeight)
            m_currentHeight = m_maxHeight;
        // 重新计算百分比
        m_currentPercentage = (m_currentHeight - m_minHeight) / (m_maxHeight - m_minHeight) * 100.0;
    } else {
        // spdlog::warn("设置桶液位高度范围错误:最低高度 大小或等于 最高高度");
    }
    if (!unit.isNull()) {
        m_heightUnit = unit;
    }
}

void TankWidget::setConnectedState(bool connected) {
    if (m_isConnected != connected) {
        m_isConnected = connected;
        update();
    }
}

void TankWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // painter.fillRect(rect(), QColor(40, 40, 40));

    drawTankBody(painter);
    drawLiquid(painter);
    drawScale(painter);
    drawReadings(painter);
}

void TankWidget::drawTankBody(QPainter &painter) {
    int tankWidth = width() * 0.5;
    int tankHeight = height() - 100; // Reserve 100 pixels for text at bottom
    int tankX = (width() - tankWidth) / 2;
    int tankY = 25; // Top margin

    // Draw tank based on shape
    switch (m_shape) {
        case Cylindrical:
            {
                // Draw top ellipse
                QLinearGradient topGradient(tankX, tankY, tankX, tankY + 10);
                topGradient.setColorAt(0.0, m_tankColor.lighter(130));
                topGradient.setColorAt(1.0, m_tankColor);

                painter.setBrush(topGradient);
                painter.setPen(QPen(QColor(100, 100, 100), 2));
                painter.drawEllipse(tankX, tankY, tankWidth, 10);

                // Draw cylinder body
                QLinearGradient bodyGradient(tankX, tankY, tankX + tankWidth, tankY);
                bodyGradient.setColorAt(0.0, m_tankColor.darker(120));
                bodyGradient.setColorAt(0.5, m_tankColor);
                bodyGradient.setColorAt(1.0, m_tankColor.darker(120));

                painter.setBrush(bodyGradient);
                painter.setPen(QPen(QColor(100, 100, 100), 2));
                painter.drawRect(tankX, tankY + 5, tankWidth, tankHeight);

                // Draw bottom ellipse
                painter.setBrush(m_tankColor.darker(110));
                painter.drawEllipse(tankX, tankY + tankHeight, tankWidth, 10);
                break;
            }
        case Rectangular:
            {
                QLinearGradient gradient(tankX, tankY, tankX + tankWidth, tankY);
                gradient.setColorAt(0.0, m_tankColor.darker(120));
                gradient.setColorAt(0.5, m_tankColor);
                gradient.setColorAt(1.0, m_tankColor.darker(120));

                painter.setBrush(gradient);
                painter.setPen(QPen(QColor(100, 100, 100), 2));
                painter.drawRect(tankX, tankY, tankWidth, tankHeight);
                break;
            }
        case Spherical:
            {
                QRadialGradient gradient(tankX + tankWidth / 2, tankY + tankHeight / 2 - tankHeight / 6,
                                         tankHeight / 2);
                gradient.setColorAt(0.0, m_tankColor.lighter(130));
                gradient.setColorAt(0.7, m_tankColor);
                gradient.setColorAt(1.0, m_tankColor.darker(130));

                painter.setBrush(gradient);
                painter.setPen(QPen(QColor(100, 100, 100), 2));
                painter.drawEllipse(tankX, tankY, tankWidth, tankHeight);
                break;
            }
    }

    // Make tank semi-transparent to see liquid
    // painter.setOpacity(0.7);
    // painter.fillRect(tankX + 2, tankY + 2, tankWidth - 4, tankHeight - 4,
    //                  QColor(60, 60, 60, 100));
    // painter.setOpacity(1.0);
}

void TankWidget::drawLiquid(QPainter &painter) {
    const int tankWidth = width() * 0.5;
    const int tankHeight = height() - 100;
    const int tankX = (width() - tankWidth) / 2;
    const int tankY = 25;

    const double liquidHeight = (tankHeight - 4) * (m_currentPercentage / 100.0);
    const int liquidY = tankY + tankHeight - 2 - static_cast<int>(liquidHeight);

    if (liquidHeight <= 0)
        return;

    // ----- 1. 液体填充 -----
    QLinearGradient liquidGradient(tankX, liquidY, tankX, liquidY + liquidHeight);
    liquidGradient.setColorAt(0.0, m_liquidColor.lighter(120));
    liquidGradient.setColorAt(0.5, m_liquidColor);
    liquidGradient.setColorAt(1.0, m_liquidColor.darker(110));
    painter.setBrush(liquidGradient);
    painter.setPen(Qt::NoPen);

    switch (m_shape) {
        case Cylindrical:
        case Rectangular:
            painter.drawRect(tankX + 2, liquidY, tankWidth - 4, static_cast<int>(liquidHeight));
            break;
        case Spherical:
            painter.setClipRect(tankX + 2, liquidY, tankWidth - 4, static_cast<int>(liquidHeight));
            painter.drawEllipse(tankX + 2, tankY + 2, tankWidth - 4, tankHeight - 4);
            painter.setClipping(false);
            break;
    }

    // ----- 2. 液面波浪线 -----
    painter.setPen(QPen(m_liquidColor.lighter(140), 2));

    switch (m_shape) {
        case Cylindrical:
        case Rectangular:
            {
                static constexpr int steps = 30;
                static constexpr int amplitude = 2;
                static constexpr int wavelength = 14;
                const double leftX = tankX + 4;
                const double rightX = tankX + tankWidth - 4;
                const double stepX = (rightX - leftX) / steps;

                QVarLengthArray<QPointF, 32> points(steps + 1);
                for (int i = 0; i <= steps; ++i) {
                    const double x = leftX + i * stepX;
                    const double y = liquidY + amplitude * qSin(2 * M_PI * (x - leftX) / wavelength);
                    points[i] = QPointF(x, y);
                }
                painter.drawPolyline(points.data(), points.size());
                break;
            }

        case Spherical:
            {
                const int innerLeft = tankX + 2;
                const int innerTop = tankY + 2;
                const int innerWidth = tankWidth - 4;
                const int innerHeight = tankHeight - 4;
                const double a = innerWidth / 2.0;
                const double b = innerHeight / 2.0;
                const double cx = innerLeft + a;
                const double cy = innerTop + b;
                const double dy = liquidY - cy;

                if (qAbs(dy) >= b - 0.5) {
                    painter.drawPoint(QPointF(cx, liquidY)); // 边界情况
                    break;
                }

                const double halfWidth = a * sqrt(1.0 - (dy * dy) / (b * b));
                const double leftX = cx - halfWidth;
                const double rightX = cx + halfWidth;
                if (rightX - leftX <= 2.0) {
                    painter.drawPoint(QPointF(cx, liquidY));
                    break;
                }

                static constexpr int steps = 20;
                static constexpr int amplitude = 1;
                static constexpr int wavelength = 10;
                const double stepX = (rightX - leftX) / steps;

                QVarLengthArray<QPointF, 24> points(steps + 1);
                for (int i = 0; i <= steps; ++i) {
                    const double x = leftX + i * stepX;
                    const double y = liquidY + amplitude * qSin(2 * M_PI * (x - leftX) / wavelength);
                    points[i] = QPointF(x, y);
                }
                painter.drawPolyline(points.data(), points.size());
                break;
            }
    }
}

void TankWidget::drawScale(QPainter &painter) {
    int tankWidth = width() * 0.5;
    int tankHeight = height() - 100;
    int tankX = (width() - tankWidth) / 2;
    int tankY = 25;
    int tankRight = tankX + tankWidth;

    painter.save();
    painter.setPen(QPen(Qt::black, 1));
    QFont font = painter.font();
    font.setPointSize(7);
    painter.setFont(font);

    QFontMetrics fm(font);
    int textHeight = fm.height();

    // Draw level marks
    for (int i = 0; i <= 10; ++i) {
        int y = tankY + tankHeight - (i * tankHeight / 10);
        int level = i * 10;

        if (i % 2 == 0) {
            painter.drawLine(tankRight + 2, y, tankRight + 8, y);
            painter.drawText(tankRight + 10, y - 6, 30, 12, Qt::AlignLeft | Qt::AlignVCenter,
                             QString::number(level) + "%");
        } else {
            painter.drawLine(tankRight + 2, y, tankRight + 5, y);
        }
    }

    // ---------- 左侧高度刻度 ----------
    const int leftScaleLineEnd = tankX - 2;              // 刻度线右端（罐体左边缘）
    const int leftScaleLineStart = leftScaleLineEnd - 6; // 长刻度线向左6像素
    const double stepHeight = (m_maxHeight - m_minHeight) / 10.0;
    const int precision = (stepHeight < 0.5) ? 2 : 1;

    // 数字显示区域：右边缘对齐刻度线左端（留2像素间距）
    const int numberRightEdge = leftScaleLineStart - 2; // 数字文本右边界
    const int numberWidth = 30;
    for (int i = 0; i <= 10; ++i) {
        const int y = tankY + tankHeight - (i * tankHeight / 10);
        const double heightValue = m_minHeight + i * stepHeight;
        if (i % 2 == 0) {
            painter.drawLine(leftScaleLineStart, y, leftScaleLineEnd, y);
            QRect textRect(numberRightEdge - numberWidth, y - textHeight / 2, numberWidth, textHeight);
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(heightValue, 'f', precision));
        } else {
            painter.drawLine(leftScaleLineStart + 2, y, leftScaleLineEnd, y);
        }
    }

    // ---------- 单位标注（与刻度线右端对齐，而非数字右对齐）----------
    const int unitRightEdge = tankX; // 单位右边界对齐刻度线右端
    const QString unitText = "单位:" + m_heightUnit;
    const int unitWidth = fm.horizontalAdvance(unitText);
    const int yTop = tankY; // 最顶部刻度线的 Y 坐标
    QRect unitRect(unitRightEdge - unitWidth, yTop - textHeight - 8, unitWidth, textHeight);
    painter.drawText(unitRect, Qt::AlignRight | Qt::AlignVCenter, unitText);

    painter.restore();
}

void TankWidget::drawReadings(QPainter &painter) {
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    // Calculate starting position for bottom text (leave 80 pixels at bottom)
    int readingY = height() - 70;
    int lineHeight = 18;

    // Level
    if (m_isConnected) {
        QString PercentageText =
            m_isUsedPercentage ? QString("液位比: %1%").arg(m_currentPercentage, 0, 'f', 1) : QString("液位比: N/A");
        painter.drawText(5, readingY, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter, PercentageText);

        QString heightText = m_isUsedPercentage ? QString("液位高度: %1" + m_heightUnit).arg(m_currentHeight, 0, 'f', 1)
                                                : QString("液位高度: N/A");
        painter.drawText(5, readingY + lineHeight, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter,
                         heightText);

        // Temperature
        QString tempText =
            m_isUsedTemp
                ? QString("温度: %1%2C").arg(m_currentTemperature, 0, 'f', 1).arg(QString::fromUtf8("\xC2\xB0"))
                : QString("温度: N/A");
        painter.drawText(5, readingY + lineHeight * 2, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter,
                         tempText);

        // Pressure
        QString pressureText =
            m_isUsedPressure ? QString("压力: %1 bar").arg(m_currentPressure, 0, 'f', 2) : QString("压力: N/A");
        painter.drawText(5, readingY + lineHeight * 3, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter,
                         pressureText);
    } else {
        QString PercentageText = QString("液位比: N/A");
        painter.drawText(5, readingY, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter, PercentageText);

        QString heightText = QString("液位高度: N/A");
        painter.drawText(5, readingY + lineHeight, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter,
                         heightText);

        // Temperature
        QString tempText = QString("温度: N/A");
        painter.drawText(5, readingY + lineHeight * 2, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter,
                         tempText);

        // Pressure
        QString pressureText = QString("压力: N/A");
        painter.drawText(5, readingY + lineHeight * 3, width() - 10, lineHeight, Qt::AlignHCenter | Qt::AlignVCenter,
                         pressureText);
    }

    // Volume (at top)
    // double currentVolume = m_capacity * (m_currentPercentage / 100.0);
    // font.setBold(true);
    // font.setPointSize(9);
    // painter.setFont(font);
    // QString volumeText =
    //     QString("%1 / %2 %3").arg(currentVolume, 0, 'f', 0).arg(m_capacity, 0, 'f', 0).arg(m_capacityUnit);
    // QString volumeText =
    //     QString("%1 / %2 %3").arg(m_minHeight, 0, 'f', 0).arg(m_maxHeight, 0, 'f', 0).arg("cm");
    painter.drawText(0, 2, width(), 18, Qt::AlignCenter, "加药桶");
}
