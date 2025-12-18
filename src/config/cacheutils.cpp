// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Jeremy Borgman

#include "cacheutils.h"
#include "src/tools/abstracttwopointtool.h"
#include "src/tools/arrow/arrowtool.h"
#include "src/tools/capturecontext.h"
#include "src/tools/circle/circletool.h"
#include "src/tools/circlecount/circlecounttool.h"
#include "src/tools/pixelate/pixelatetool.h"
#include "src/tools/rectangle/rectangletool.h"
#include "src/widgets/capture/capturetoolobjects.h"
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QRect>
#include <QStandardPaths>
#include <QString>

// Cache file format version for tool objects
static const quint32 TOOL_CACHE_VERSION = 1;

QString getCachePath()
{
    auto cachePath =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!QDir(cachePath).exists()) {
        QDir().mkpath(cachePath);
    }
    return cachePath;
}

void setLastRegion(QRect const& newRegion)
{
    auto cachePath = getCachePath() + "/region.txt";

    QFile file(cachePath);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);
        out << newRegion;
        file.close();
    }
}

QRect getLastRegion()
{
    auto cachePath = getCachePath() + "/region.txt";
    QFile file(cachePath);

    QRect lastRegion;
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream input(&file);
        input >> lastRegion;
        file.close();
    } else {
        lastRegion = QRect(0, 0, 0, 0);
    }

    return lastRegion;
}

// Helper function to write two-point tool data
static void writeTwoPointToolData(QDataStream& out,
                                  AbstractTwoPointTool* tool)
{
    out << tool->points().first;
    out << tool->points().second;
    out << tool->color();
    out << tool->size();
}

// Helper function to read two-point tool data and initialize a tool
static void readTwoPointToolData(QDataStream& in,
                                 QPoint& p1,
                                 QPoint& p2,
                                 QColor& color,
                                 int& thickness)
{
    in >> p1 >> p2 >> color >> thickness;
}

void setLastToolObjects(const CaptureToolObjects& objects)
{
    auto cachePath = getCachePath() + "/toolobjects.bin";

    QFile file(cachePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    // Write version
    out << TOOL_CACHE_VERSION;

    // Get the tool objects using const method
    auto toolList = objects.captureToolObjects();

    // Count only the supported tool types (two-point tools and circle count)
    qint32 supportedCount = 0;
    for (const auto& tool : toolList) {
        if (tool.isNull()) {
            continue;
        }
        switch (tool->type()) {
            case CaptureTool::TYPE_ARROW:
            case CaptureTool::TYPE_RECTANGLE:
            case CaptureTool::TYPE_CIRCLE:
            case CaptureTool::TYPE_PIXELATE:
            case CaptureTool::TYPE_CIRCLECOUNT:
                supportedCount++;
                break;
            default:
                // Skip unsupported tool types (path tools, text, etc.)
                break;
        }
    }

    // Write count of supported tools only
    out << supportedCount;

    // Write each supported tool
    for (const auto& tool : toolList) {
        if (tool.isNull()) {
            continue;
        }

        // Write tool type and data only for supported types
        switch (tool->type()) {
            case CaptureTool::TYPE_ARROW:
            case CaptureTool::TYPE_RECTANGLE:
            case CaptureTool::TYPE_CIRCLE:
            case CaptureTool::TYPE_PIXELATE: {
                auto* twoPointTool =
                  dynamic_cast<AbstractTwoPointTool*>(tool.data());
                if (twoPointTool) {
                    out << static_cast<qint32>(tool->type());
                    writeTwoPointToolData(out, twoPointTool);
                }
                break;
            }
            case CaptureTool::TYPE_CIRCLECOUNT: {
                auto* circleCountTool =
                  dynamic_cast<CircleCountTool*>(tool.data());
                if (circleCountTool) {
                    out << static_cast<qint32>(tool->type());
                    writeTwoPointToolData(out, circleCountTool);
                    out << static_cast<qint32>(circleCountTool->count());
                }
                break;
            }
            default:
                // Skip unsupported tool types (path tools, text, etc.)
                // These would require class modifications to properly serialize
                break;
        }
    }

    file.close();
}

void getLastToolObjects(CaptureToolObjects& objects)
{
    auto cachePath = getCachePath() + "/toolobjects.bin";

    // Check if the cache file exists before trying to open it
    if (!QFile::exists(cachePath)) {
        return;
    }

    QFile file(cachePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    // Read and verify version
    quint32 version;
    in >> version;
    if (version != TOOL_CACHE_VERSION) {
        file.close();
        return;
    }

    // Read count
    qint32 count;
    in >> count;

    // Read each tool
    for (qint32 i = 0; i < count; ++i) {
        qint32 toolTypeInt;
        in >> toolTypeInt;
        auto toolType = static_cast<CaptureTool::Type>(toolTypeInt);

        switch (toolType) {
            case CaptureTool::TYPE_ARROW:
            case CaptureTool::TYPE_RECTANGLE:
            case CaptureTool::TYPE_CIRCLE:
            case CaptureTool::TYPE_PIXELATE: {
                QPoint p1, p2;
                QColor color;
                int thickness;
                readTwoPointToolData(in, p1, p2, color, thickness);

                // Create the appropriate tool
                AbstractTwoPointTool* tool = nullptr;
                switch (toolType) {
                    case CaptureTool::TYPE_ARROW:
                        tool = new ArrowTool();
                        break;
                    case CaptureTool::TYPE_RECTANGLE:
                        tool = new RectangleTool();
                        break;
                    case CaptureTool::TYPE_CIRCLE:
                        tool = new CircleTool();
                        break;
                    case CaptureTool::TYPE_PIXELATE:
                        tool = new PixelateTool();
                        break;
                    default:
                        break;
                }

                if (tool) {
                    // Initialize the tool with a context
                    CaptureContext ctx;
                    ctx.color = color;
                    ctx.toolSize = thickness;
                    ctx.mousePos = p1;
                    tool->drawStart(ctx);
                    tool->drawMove(p2);
                    tool->drawEnd(p2);
                    objects.append(tool);
                }
                break;
            }
            case CaptureTool::TYPE_CIRCLECOUNT: {
                QPoint p1, p2;
                QColor color;
                int thickness;
                readTwoPointToolData(in, p1, p2, color, thickness);
                qint32 countValue;
                in >> countValue;

                auto* tool = new CircleCountTool();
                CaptureContext ctx;
                ctx.color = color;
                ctx.toolSize = thickness;
                ctx.mousePos = p1;
                tool->drawStart(ctx);
                tool->drawMove(p2);
                tool->drawEnd(p2);
                tool->setCount(countValue);
                objects.append(tool);
                break;
            }
            default:
                // Skip unsupported tool types
                break;
        }
    }

    file.close();
}

void clearLastToolObjects()
{
    auto cachePath = getCachePath() + "/toolobjects.bin";
    QFile::remove(cachePath);
}
