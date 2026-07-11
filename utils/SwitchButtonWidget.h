#ifndef SWITCHBUTTONWIDGET_H
#define SWITCHBUTTONWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

#include <spdlog/spdlog.h>

/**
 * @brief Industrial style switch button widget
 * 
 * This widget displays a toggle switch button with smooth animation,
 * suitable for on/off control operations.
 */
class SwitchButtonWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SwitchButtonWidget(QWidget *parent = nullptr);
    ~SwitchButtonWidget() override;

    // Getters
    bool isChecked() const { return m_checked; }
    bool isEnabled() const { return m_enabled; }

    // Setters
    void setChecked(bool checked);
    void setEnabled(bool enabled);
    void toggle();

signals:
    void toggled(bool checked);
    void clicked();

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual QSize sizeHint() const override { return QSize(80, 40); }
    virtual QSize minimumSizeHint() const override { return QSize(60, 30); }

private:
    void drawBackground(QPainter &painter);
    void drawHandle(QPainter &painter);

    bool m_checked{false};
    bool m_enabled{true};
    bool m_pressed{false};

    QColor m_onColor{QColor(0, 200, 0)};
    QColor m_offColor{QColor(150, 150, 150)};
    QColor m_disabledColor{QColor(80, 80, 80)};
};

#endif // SWITCHBUTTONWIDGET_H












