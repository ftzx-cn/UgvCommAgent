#include "suffixlineedit.h"

SuffixLineEdit::SuffixLineEdit(QWidget *parent)
    : QLineEdit{parent}
{
    auto layout=new QHBoxLayout(this);
    layout->setContentsMargins(2, 0, 2, 0);
    layout->setSpacing(0);

    m_suffixLabel->setAttribute(Qt::WA_TranslucentBackground);
    m_suffixLabel->setAlignment(Qt::AlignCenter);

    m_lineEdit->setFrame(false);
    m_lineEdit->setAttribute(Qt::WA_TranslucentBackground);
    m_lineEdit->setAlignment(Qt::AlignRight);

    layout->addWidget(m_lineEdit);
    layout->addWidget(m_suffixLabel);

    connect(m_lineEdit, &QLineEdit::textChanged, this, [=]()
            { emit textChanged(m_lineEdit->text()); });
}
