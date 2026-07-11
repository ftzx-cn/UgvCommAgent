//************************************************************
// Created by ftzx on 25-1-21.
// FileName: cywiplineedit.h
// Description: QT的IP地址框组件// 模块描述
// Version: 1.0// 版本信息
// Function List: // 主要函数及其功能
// 1. QString getIP() 获取IP地址，形式为000.000.000.000的字符串
// 2. setIP(const QString &ipstr) 设置IP地址
// History: // 历史修改记录
//***********************************************************/
#pragma once

#ifndef CYWIPLINEEDIT_H
#define CYWIPLINEEDIT_H

#include <QLineEdit>
#include <QLabel>
#include <QString>

class CywIpLineEdit final : public QLineEdit
{
    Q_OBJECT

public:
    explicit CywIpLineEdit(QWidget *parent = nullptr);

    ~CywIpLineEdit() override = default;

    // 获取IP地址，形式为000.000.000.000的字符串
    [[nodiscard]] QString getIP() const;

    // 设置IP地址
    void setIP(const QString &ipstr) const;

signals:
    void textChanged();

private:
    QLineEdit *m_edit1{new QLineEdit(this)};
    QLineEdit *m_edit2{new QLineEdit(this)};
    QLineEdit *m_edit3{new QLineEdit(this)};
    QLineEdit *m_edit4{new QLineEdit(this)};

    QLabel *m_dot1{new QLabel(".", this)};
    QLabel *m_dot2{new QLabel(".", this)};
    QLabel *m_dot3{new QLabel(".", this)};

    // 每一位编辑框有效值达到3位数，焦点移到下一编辑框
    static void moveFocus(const QLineEdit *current, QLineEdit *next);

    // 输入错误，显示提示信息
    void showTip();

    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // CYWIPLINEEDIT_H
