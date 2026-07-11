#ifndef PUMPWIDGET_H
#define PUMPWIDGET_H

#include <QPainter>
#include <QTimer>
#include <QWidget>

#include <spdlog/spdlog.h>

/**
 * @brief Industrial pump widget
 *
 * This widget displays a pump with running/stopped states and rotation animation,
 * suitable for process control visualization.
 */
class PumpWidget : public QWidget {
    Q_OBJECT

  public:
    enum RunningState { Stopped, Starting, Running, Stopping, Fault };
    enum RemoteState { Local, Remote, Null };

    explicit PumpWidget(QWidget *parent = nullptr);
    ~PumpWidget() override;

    // Getters
    RunningState runningState() const { return m_runningState; }
    RemoteState remoteState() const { return m_remoteState; }
    double speed() const { return m_speed; } // RPM or percentage
    bool isRunning() const { return m_runningState == Running; }

    // Setters
    void setRunningState(RunningState state);
    void setRemoteState(RemoteState state);
    void setSpeed(double speed);
    void start();
    void stop();

  signals:
    void runningStateChanged(RunningState state);
    void remoteStateChanaged(RemoteState state);

  protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(200, 200); }
    QSize minimumSizeHint() const override { return QSize(200, 200); }

  private slots:
    void onAnimationTimer();

  private:
    void drawPumpBody(QPainter &painter);
    void drawImpeller(QPainter &painter);
    void drawPipes(QPainter &painter);
    void drawStatus(QPainter &painter);

    RunningState m_runningState{RunningState::Stopped};
    RemoteState m_remoteState{RemoteState::Null};
    double m_speed{50.0};
    double m_rotationAngle{0.0};

    QTimer *m_animationTimer{new QTimer(this)};

    QColor m_runningColor{QColor(0, 200, 0)};
    QColor m_stoppedColor{QColor(150, 150, 150)};
    QColor m_faultColor{QColor(255, 0, 0)};
};

#endif // PUMPWIDGET_H
