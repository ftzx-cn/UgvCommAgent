#ifndef TANKWIDGET_H
#define TANKWIDGET_H

#include <QPainter>
#include <QWidget>

#include <spdlog/spdlog.h>

/**
 * @brief Industrial tank widget
 *
 * This widget displays a storage tank with liquid level, temperature,
 * and pressure indicators, suitable for process monitoring.
 */
class TankWidget : public QWidget
{
    Q_OBJECT

  public:
    enum TankShape
    {
        Cylindrical,
        Rectangular,
        Spherical
    };

    explicit TankWidget(QWidget *parent = nullptr);
    ~TankWidget() override;

    // Getters
    double currentPercentage() const
    {
        return m_currentPercentage;
    }
    double currentTemperature() const
    {
        return m_currentTemperature;
    }
    double currentPressure() const
    {
        return m_currentPressure;
    }
    TankShape shape() const
    {
        return m_shape;
    }

    // Setters
    void setPercentage(double percentage); // 0-100%
    void setTemperature(double temp);
    void setPressure(double pressure);
    void setShape(TankShape shape);
    void setCapacity(double capacity, const QString &unit = "cm");
    void setLiquidColor(const QColor &color);
    void setUsedValue(bool percentage, bool temp, bool pressure);
    void setHeightConfig(double minHeight, double maxHeight, QString unit);
    void setConnectedState(bool connected);

  protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override
    {
        return QSize(140, 280);
    }
    QSize minimumSizeHint() const override
    {
        return QSize(100, 200);
    }

  private:
    void drawTankBody(QPainter &painter);
    void drawLiquid(QPainter &painter);
    void drawScale(QPainter &painter);
    void drawReadings(QPainter &painter);

    double m_currentPercentage{50.0};
    double m_currentTemperature{25.0};
    double m_currentPressure{1.0};
    double m_capacity{1000.0};
    QString m_capacityUnit{"l"};
    TankShape m_shape{TankShape::Cylindrical};

    QColor m_liquidColor{QColor(100, 150, 255)};
    QColor m_tankColor{QColor(180, 180, 180)};

    bool m_isUsedPercentage{true};
    bool m_isUsedTemp{true};
    bool m_isUsedPressure{true};
    bool m_isConnected{false};

    double m_maxHeight{100};
    double m_minHeight{0};
    double m_currentHeight{50};
    QString m_heightUnit{""};
};

#endif // TANKWIDGET_H
