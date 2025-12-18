// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Jeremy Borgman

#include "cacheutils.h"
#include "src/tools/abstractpathtool.h"
#include "src/tools/abstracttwopointtool.h"
#include "src/tools/arrow/arrowtool.h"
#include "src/tools/capturecontext.h"
#include "src/tools/circle/circletool.h"
#include "src/tools/circlecount/circlecounttool.h"
#include "src/tools/line/linetool.h"
#include "src/tools/marker/markertool.h"
#include "src/tools/pencil/penciltool.h"
#include "src/tools/pixelate/pixelatetool.h"
#include "src/tools/rectangle/rectangletool.h"
#include "src/tools/text/texttool.h"
#include "src/widgets/capture/capturetoolobjects.h"
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFont>
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

    // Get the tool objects (need to cast away const for the getter)
    auto& mutableObjects = const_cast<CaptureToolObjects&>(objects);
    auto toolList = mutableObjects.captureToolObjects();

    // Write count
    out << static_cast<qint32>(toolList.size());

    // Write each tool
    for (const auto& tool : toolList) {
        if (tool.isNull()) {
            continue;
        }

        // Write tool type
        out << static_cast<qint32>(tool->type());

        switch (tool->type()) {
            case CaptureTool::TYPE_PENCIL:
            case CaptureTool::TYPE_DRAWER:
            case CaptureTool::TYPE_MARKER: {
                auto* pathTool = dynamic_cast<AbstractPathTool*>(tool.data());
                if (pathTool) {
                    // We need to access the points, but they're protected
                    // Use a workaround: serialize bounding rect, color, size
                    // and the tool's process output
                    out << pathTool->boundingRect();
                    // Access via the virtual methods we have
                    const QPoint* posPtr = pathTool->pos();
                    out << (posPtr ? *posPtr : QPoint());
                    out << pathTool->size();
                    // We don't have direct access to color and points
                    // This is a limitation - for path tools, we'd need to
                    // modify the class to expose serialization
                }
                break;
            }
            case CaptureTool::TYPE_ARROW:
            case CaptureTool::TYPE_RECTANGLE:
            case CaptureTool::TYPE_CIRCLE:
            case CaptureTool::TYPE_PIXELATE: {
                auto* twoPointTool =
                  dynamic_cast<AbstractTwoPointTool*>(tool.data());
                if (twoPointTool) {
                    writeTwoPointToolData(out, twoPointTool);
                }
                break;
            }
            case CaptureTool::TYPE_CIRCLECOUNT: {
                auto* circleCountTool =
                  dynamic_cast<CircleCountTool*>(tool.data());
                if (circleCountTool) {
                    writeTwoPointToolData(out, circleCountTool);
                    out << static_cast<qint32>(circleCountTool->count());
                }
                break;
            }
            case CaptureTool::TYPE_TEXT: {
                auto* textTool = dynamic_cast<TextTool*>(tool.data());
                if (textTool) {
                    out << textTool->boundingRect();
                    const QPoint* posPtr = textTool->pos();
                    out << (posPtr ? *posPtr : QPoint());
                    out << textTool->size();
                    // Note: text content and font aren't easily accessible
                    // without modifying the class
                }
                break;
            }
            default:
                // Skip unsupported tool types
                break;
        }
    }

    file.close();
}

void getLastToolObjects(CaptureToolObjects& objects)
{
    auto cachePath = getCachePath() + "/toolobjects.bin";

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
            case CaptureTool::TYPE_PENCIL:
            case CaptureTool::TYPE_DRAWER:
            case CaptureTool::TYPE_MARKER: {
                // Path tools require more complex deserialization
                // Skip for now - would need class modifications
                QRect bounds;
                QPoint pos;
                int size;
                in >> bounds >> pos >> size;
                break;
            }
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
                    // Initialize the tool with a fake context
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
            case CaptureTool::TYPE_TEXT: {
                // Text tool requires more complex deserialization
                QRect bounds;
                QPoint pos;
                int size;
                in >> bounds >> pos >> size;
                // Skip text tools for now - would need class modifications
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
