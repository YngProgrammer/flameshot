// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Jeremy Borgman

#ifndef FLAMESHOT_CACHEUTILS_H
#define FLAMESHOT_CACHEUTILS_H

class QString;
class QRect;
class CaptureToolObjects;

QString getCachePath();
QRect getLastRegion();
void setLastRegion(QRect const& newRegion);
void setLastToolObjects(const CaptureToolObjects& objects);
void getLastToolObjects(CaptureToolObjects& objects);
void clearLastToolObjects();

#endif // FLAMESHOT_CACHEUTILS_H
