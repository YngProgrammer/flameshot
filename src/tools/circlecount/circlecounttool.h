// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "src/tools/abstracttwopointtool.h"
#include <QPointer>

class CircleCountConfig;

class CircleCountTool : public AbstractTwoPointTool
{
    Q_OBJECT
public:
    explicit CircleCountTool(QObject* parent = nullptr);
    ~CircleCountTool() override;

    QIcon icon(const QColor& background, bool inEditor) const override;
    QString name() const override;
    QString description() const override;
    QString info() override;
    bool isValid() const override;

    QRect mousePreviewRect(const CaptureContext& context) const override;
    QRect boundingRect() const override;

    CaptureTool* copy(QObject* parent = nullptr) override;
    void process(QPainter& painter, const QPixmap& pixmap) override;
    void paintMousePreview(QPainter& painter,
                           const CaptureContext& context) override;
    
    QWidget* configurationWidget() override;
    
    bool useCustomCount() const { return m_useCustomCount; }

protected:
    CaptureTool::Type type() const override;
    void copyParams(const CircleCountTool* from, CircleCountTool* to);

public slots:
    void drawStart(const CaptureContext& context) override;
    void pressed(CaptureContext& context) override;

private slots:
    void updateCustomCount(int count);
    void updateUseCustomCount(bool useCustom);

private:
    QString m_tempString;
    bool m_valid;
    bool m_useCustomCount;
    int m_customCount;
    QPointer<CircleCountConfig> m_confW;
};
