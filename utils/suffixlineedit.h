#pragma once

#ifndef SUFFIXLINEEDIT_H
#define SUFFIXLINEEDIT_H

#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>

class SuffixLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit SuffixLineEdit(QWidget *parent = nullptr);
    ~SuffixLineEdit() override = default;

    void setSuffix(const QString &suffix) { m_suffixLabel->setText(suffix); }
    QString suffix() const { return m_suffixLabel->text(); }

signals:
    void textChanged(const QString &text);

private:
    QLineEdit *m_lineEdit{new QLineEdit(this)};
    QLabel *m_suffixLabel{new QLabel(this)};
};

#endif // SUFFIXLINEEDIT_H