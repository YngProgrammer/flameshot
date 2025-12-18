// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include <QWidget>

class QSpinBox;
class QVBoxLayout;
class QLabel;
class QCheckBox;

class CircleCountConfig : public QWidget
{
    Q_OBJECT
public:
    explicit CircleCountConfig(QWidget* parent = nullptr);

    void setCustomCount(int count);
    void setUseCustomCount(bool useCustom);

signals:
    void customCountChanged(int count);
    void useCustomCountChanged(bool useCustom);

private slots:
    void onCheckBoxToggled(bool checked);
    void onSpinBoxChanged(int value);

private:
    QVBoxLayout* m_layout;
    QCheckBox* m_useCustomCheckBox;
    QSpinBox* m_countSpinBox;
    QLabel* m_label;
};
