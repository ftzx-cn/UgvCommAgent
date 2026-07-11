#include "PumpWidget.h"
#include <QPainterPath>
#include <QtMath>

PumpWidget::PumpWidget(QWidget *parent) : QWidget(parent)
{
    connect(m_animationTimer, &QTimer::timeout, this, &PumpWidget::onAnimationTimer);
    m_animationTimer->setInterval(50); // 20 FPS
}

PumpWidget::~PumpWidget()
{
}

void PumpWidget::setRunningState(RunningState state)
{
    if (m_runningState == state)
        return;

    // Control animation based on state
    m_runningState = state;
    if (m_runningState == Running)
    {
        m_animationTimer->start();
    }
    else if (m_runningState == Stopped || m_runningState == Fault)
    {
        m_animationTimer->stop();
    }

    update();
    emit runningStateChanged(m_runningState);
}

void PumpWidget::setRemoteState(RemoteState state)
{
    if (m_remoteState != state)
    {
        m_remoteState = state;
        update();
        emit remoteStateChanaged(state);
    }
}

void PumpWidget::setSpeed(double speed)
{
    if (speed < 0.0)
        speed = 0.0;
    if (speed > 100.0)
        speed = 100.0;

    if (qAbs(m_speed - speed) > 0.1)
    {
        m_speed = speed;
        update();
    }
}

void PumpWidget::start()
{
    setRunningState(Starting);
    //spdlog::trace("应用程序调用PumpWidget::start()启动加药泵");
    // if (m_speed == 0.0)
    // setSpeed(100.0);
}

void PumpWidget::stop()
{
    setRunningState(Stopping);
    //spdlog::trace("应用程序调用PumpWidget::stop()停止加药泵");
    // setSpeed(0.0);
}

void PumpWidget::onAnimationTimer()
{
    if (m_runningState == Running || m_runningState == Stopping)
    {
        // Rotation speed proportional to pump speed
        double rotationSpeed = (m_speed / 100.0) * 20.0; // degrees per frame
        m_rotationAngle += rotationSpeed;
        if (m_rotationAngle >= 360.0)
            m_rotationAngle -= 360.0;
        update();
    }
}

void PumpWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // painter.fillRect(rect(), QColor(40, 40, 40));

    drawPipes(painter);
    drawPumpBody(painter);
    drawImpeller(painter);
    drawStatus(painter);
}

void PumpWidget::drawPumpBody(QPainter &painter)
{
    int centerX = width() / 2;
    int centerY = height() / 2;
    int pumpSize = qMin(width(), height()) * 0.35;

    // Draw pump casing (circular)
    QRadialGradient casingGradient(centerX, centerY - pumpSize / 4, pumpSize * 1.5);
    casingGradient.setColorAt(0.0, QColor(180, 180, 180));
    casingGradient.setColorAt(0.5, QColor(140, 140, 140));
    casingGradient.setColorAt(1.0, QColor(100, 100, 100));

    painter.setBrush(casingGradient);
    painter.setPen(QPen(QColor(80, 80, 80), 2));
    painter.drawEllipse(centerX - pumpSize, centerY - pumpSize, pumpSize * 2, pumpSize * 2);

    // Draw inlet volute
    painter.setPen(QPen(QColor(60, 60, 60), 2));
    painter.setBrush(QColor(120, 120, 120));

    QPainterPath volutePath;
    volutePath.moveTo(centerX - pumpSize * 0.7, centerY);
    volutePath.arcTo(centerX - pumpSize, centerY - pumpSize, pumpSize * 2, pumpSize * 2, 180, 180);
    painter.drawPath(volutePath);
}

void PumpWidget::drawImpeller(QPainter &painter)
{
    int centerX = width() / 2;
    int centerY = height() / 2;
    int impellerSize = qMin(width(), height()) * 0.25;

    painter.save();
    painter.translate(centerX, centerY);
    painter.rotate(m_rotationAngle);

    // Determine color based on state
    QColor impellerColor;
    switch (m_runningState)
    {
    case Running:
    case Starting:
        impellerColor = m_runningColor;
        break;
    case Fault:
        impellerColor = m_faultColor;
        break;
    default:
        impellerColor = m_stoppedColor;
        break;
    }

    // Draw impeller blades
    int numBlades = 6;
    for (int i = 0; i < numBlades; ++i)
    {
        painter.save();
        painter.rotate(i * 360.0 / numBlades);

        QLinearGradient bladeGradient(0, 0, 0, impellerSize);
        bladeGradient.setColorAt(0.0, impellerColor.lighter(130));
        bladeGradient.setColorAt(1.0, impellerColor.darker(110));

        painter.setBrush(bladeGradient);
        painter.setPen(QPen(impellerColor.darker(140), 1));

        QPainterPath bladePath;
        bladePath.moveTo(0, 0);
        bladePath.quadTo(impellerSize * 0.3, impellerSize * 0.5, impellerSize * 0.2, impellerSize);
        bladePath.lineTo(0, impellerSize * 0.8);
        bladePath.closeSubpath();

        painter.drawPath(bladePath);
        painter.restore();
    }

    // Draw center hub
    QRadialGradient hubGradient(0, 0, impellerSize * 0.3);
    hubGradient.setColorAt(0.0, QColor(160, 160, 160));
    hubGradient.setColorAt(1.0, QColor(100, 100, 100));

    painter.setBrush(hubGradient);
    painter.setPen(QPen(QColor(80, 80, 80), 2));
    painter.drawEllipse(static_cast<int>(-impellerSize * 0.3), static_cast<int>(-impellerSize * 0.3),
                        static_cast<int>(impellerSize * 0.6), static_cast<int>(impellerSize * 0.6));

    painter.restore();
}

void PumpWidget::drawPipes(QPainter &painter)
{
    int centerX = width() / 2;
    int centerY = height() / 2;
    int pipeWidth = width() / 5;
    int pumpSize = qMin(width(), height()) * 0.35;

    // Determine pipe color based on state
    QColor pipeColor;
    if (m_runningState == Running)
        pipeColor = m_runningColor;
    else if (m_runningState == Fault)
        pipeColor = m_faultColor;
    else
        pipeColor = m_stoppedColor;

    // Inlet pipe (left)
    QLinearGradient inletGradient(0, centerY - pipeWidth / 2, 0, centerY + pipeWidth / 2);
    inletGradient.setColorAt(0.0, pipeColor.darker(150));
    inletGradient.setColorAt(0.5, pipeColor);
    inletGradient.setColorAt(1.0, pipeColor.darker(150));

    painter.setBrush(inletGradient);
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.drawRect(5, centerY - pipeWidth / 2, centerX - pumpSize - 5, pipeWidth);

    // Outlet pipe (right)
    QLinearGradient outletGradient(0, centerY - pipeWidth / 2, 0, centerY + pipeWidth / 2);
    outletGradient.setColorAt(0.0, pipeColor.darker(150));
    outletGradient.setColorAt(0.5, pipeColor);
    outletGradient.setColorAt(1.0, pipeColor.darker(150));

    painter.setBrush(outletGradient);
    painter.drawRect(centerX + pumpSize, centerY - pipeWidth / 2, width() - centerX - pumpSize - 5, pipeWidth);

    // Draw flow direction arrows when running
    if (m_runningState == Running)
    {
        painter.setPen(QPen(Qt::yellow, 2));
        painter.setBrush(Qt::yellow);

        // Inlet arrow
        QPolygon inletArrow;
        int arrowY = centerY;
        int arrowX = centerX - pumpSize - 15;
        inletArrow << QPoint(arrowX - 10, arrowY) << QPoint(arrowX, arrowY - 6) << QPoint(arrowX, arrowY + 6);
        painter.drawPolygon(inletArrow);

        // Outlet arrow
        QPolygon outletArrow;
        arrowX = centerX + pumpSize + 15;
        outletArrow << QPoint(arrowX + 10, arrowY) << QPoint(arrowX, arrowY - 6) << QPoint(arrowX, arrowY + 6);
        painter.drawPolygon(outletArrow);
    }
}

void PumpWidget::drawStatus(QPainter &painter)
{
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    font.setBold(true);
    painter.setFont(font);

    // Draw state text
    QString stateText{"加药泵: "};
    switch (m_runningState)
    {
    case Stopped:
        stateText += "停止";
        break;
    case Starting:
        stateText += "正在启动";
        break;
    case Running:
        stateText += "运行";
        break;
    case Stopping:
        stateText += "正在停机";
        break;
    case Fault:
        stateText += "发生错误";
        break;
    }

    switch (m_remoteState)
    {
    case Local:
        stateText += " | 本地";
        break;
    case Remote:
        stateText += " | 远程";
        break;
    case Null:
        stateText += " | N/A";
        break;
    default:
        break;
    }

    painter.drawText(QRectF(0, 2, width(), 16), Qt::AlignCenter, stateText);

    // Draw speed if running
    // if (m_runningState == Running || m_runningState == Starting)
    // {
    //     QString speedText = QString::number(m_speed, 'f', 0) + "%";
    //     painter.drawText(QRectF(0, height() - 18, width(), 16), Qt::AlignCenter, speedText);
    // }
}
