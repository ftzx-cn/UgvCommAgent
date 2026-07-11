//
// Created by ftzx on 25-1-21.
//
#include "cywiplineedit.h"

#include <QHBoxLayout>
#include <QRegularExpressionValidator>
#include <QKeyEvent>
#include <QToolTip>

CywIpLineEdit::CywIpLineEdit(QWidget *parent) : QLineEdit{parent}
{
    // 设置主布局及其属性
    auto *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(2, 0, 2, 0);
    // 设置点号框背景透明,内容居中显示
    m_dot1->setAttribute(Qt::WA_TranslucentBackground);
    m_dot2->setAttribute(Qt::WA_TranslucentBackground);
    m_dot3->setAttribute(Qt::WA_TranslucentBackground);
    m_dot1->setAlignment(Qt::AlignCenter);
    m_dot2->setAlignment(Qt::AlignCenter);
    m_dot3->setAlignment(Qt::AlignCenter);
    // 设置IP输入框不显示边框,背景透明,内容居中显示
    m_edit1->setFrame(false);
    m_edit2->setFrame(false);
    m_edit3->setFrame(false);
    m_edit4->setFrame(false);
    m_edit1->setAttribute(Qt::WA_TranslucentBackground);
    m_edit2->setAttribute(Qt::WA_TranslucentBackground);
    m_edit3->setAttribute(Qt::WA_TranslucentBackground);
    m_edit4->setAttribute(Qt::WA_TranslucentBackground);
    m_edit1->setAlignment(Qt::AlignCenter);
    m_edit2->setAlignment(Qt::AlignCenter);
    m_edit3->setAlignment(Qt::AlignCenter);
    m_edit4->setAlignment(Qt::AlignCenter);
    // 向主布局添加IP输入框和点号框
    layout->addWidget(m_edit1);
    layout->addWidget(m_dot1);
    layout->addWidget(m_edit2);
    layout->addWidget(m_dot2);
    layout->addWidget(m_edit3);
    layout->addWidget(m_dot3);
    layout->addWidget(m_edit4);
    // 设置IP输入框的输入验证器
    const QRegularExpressionValidator *va = new QRegularExpressionValidator(
        QRegularExpression("^(?:2(?:[0-4][0-9]|5[0-5])|[0-1]?[0-9]?[0-9])$"));
    m_edit1->setValidator(va);
    m_edit2->setValidator(va);
    m_edit3->setValidator(va);
    m_edit4->setValidator(va);
    // 为4个IP输入框安装事件过滤器,输入点号或空格时自动跳转到下一输入框,输入退格键时自动跳转到上一输入框
    m_edit1->installEventFilter(this);
    m_edit2->installEventFilter(this);
    m_edit3->installEventFilter(this);
    m_edit4->installEventFilter(this);
    // 每个输入框内容变化时检查是否需要自动跳转到下一输入框
    connect(m_edit1, &QLineEdit::textChanged, [=]
            { 
                emit textChanged();
                moveFocus(m_edit1, m_edit2); });
    connect(m_edit2, &QLineEdit::textChanged, [=]
            { 
                emit textChanged();
                moveFocus(m_edit2, m_edit3); });
    connect(m_edit3, &QLineEdit::textChanged, [=]
            { 
                emit textChanged();
                moveFocus(m_edit3, m_edit4); });
    connect(m_edit4, &QLineEdit::textChanged, [=]
            { emit textChanged(); });
    // 无效输入时显示提示信息
    connect(m_edit1, &QLineEdit::inputRejected, this, &CywIpLineEdit::showTip);
    connect(m_edit2, &QLineEdit::inputRejected, this, &CywIpLineEdit::showTip);
    connect(m_edit3, &QLineEdit::inputRejected, this, &CywIpLineEdit::showTip);
    connect(m_edit4, &QLineEdit::inputRejected, this, &CywIpLineEdit::showTip);
}

bool CywIpLineEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (const auto *edit = qobject_cast<QLineEdit *>(watched))
    {
        if (event->type() == QEvent::KeyPress)
        {
            const auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Period || keyEvent->key() == Qt::Key_Space)
            {
                if (edit != m_edit4)
                {
                    focusNextChild();
                    return true;
                }
            }

            if (keyEvent->key() == Qt::Key_Backspace && edit->cursorPosition() == 0)
            {
                if (edit != m_edit1)
                {
                    focusPreviousChild();
                    return true;
                }
            }
        }
    }
    return QLineEdit::eventFilter(watched, event);
}

QString CywIpLineEdit::getIP() const
{
    QString edittext[4];
    QList<QLineEdit *> edits = this->findChildren<QLineEdit *>();
    // 如每个编辑框不为空，删除字符串首的0
    for (int i = 0; i < 4; ++i)
    {
        if (!edits[i]->text().isEmpty())
        {
            edittext[i] = QString::number(edits[i]->text().toInt());
        }
    }
    return QString("%1.%2.%3.%4").arg(edittext[0], edittext[1], edittext[2], edittext[3]);
}

void CywIpLineEdit::setIP(const QString &ipstr) const
{
    if (QStringList parts = ipstr.split('.'); parts.size() == 4)
    {
        m_edit1->setText(parts[0]);
        m_edit2->setText(parts[1]);
        m_edit3->setText(parts[2]);
        m_edit4->setText(parts[3]);
    }
}

void CywIpLineEdit::moveFocus(const QLineEdit *current, QLineEdit *next)
{
    if (current->text().length() == 3 && next)
    {
        next->setFocus();
        next->selectAll();
    }
}

void CywIpLineEdit::showTip()
{
    QToolTip::showText(QWidget::mapToGlobal(qobject_cast<QLineEdit *>(this->focusWidget())->pos()), "请输入0-255之间的整数",
                       this);
}
