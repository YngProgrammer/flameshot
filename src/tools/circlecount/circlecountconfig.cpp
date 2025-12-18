// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "circlecountconfig.h"
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

CircleCountConfig::CircleCountConfig(QWidget* parent)
  : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);

    m_label = new QLabel(tr("Custom Number"), this);
    m_layout->addWidget(m_label);

    m_useCustomCheckBox = new QCheckBox(tr("Use custom number"), this);
    m_layout->addWidget(m_useCustomCheckBox);

    m_countSpinBox = new QSpinBox(this);
    m_countSpinBox->setMinimum(0);
    m_countSpinBox->setMaximum(9999);
    m_countSpinBox->setValue(1);
    m_countSpinBox->setEnabled(false);
    m_layout->addWidget(m_countSpinBox);

    connect(m_useCustomCheckBox,
            &QCheckBox::toggled,
            this,
            &CircleCountConfig::onCheckBoxToggled);
    connect(m_countSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &CircleCountConfig::onSpinBoxChanged);
}

void CircleCountConfig::setCustomCount(int count)
{
    m_countSpinBox->setValue(count);
}

void CircleCountConfig::setUseCustomCount(bool useCustom)
{
    m_useCustomCheckBox->setChecked(useCustom);
    m_countSpinBox->setEnabled(useCustom);
}

void CircleCountConfig::onCheckBoxToggled(bool checked)
{
    m_countSpinBox->setEnabled(checked);
    emit useCustomCountChanged(checked);
}

void CircleCountConfig::onSpinBoxChanged(int value)
{
    emit customCountChanged(value);
}
