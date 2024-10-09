/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ANDROIDTRACKINGBACKEND_H
#define ANDROIDTRACKINGBACKEND_H

#include "abstracttrackingbackend.h"
#include <QObject>
#include <qglobal.h>
#include <QFile>
#include <QJniObject>

class AndroidTrackingBackend : public AbstractTrackingBackend
{
    Q_OBJECT
  public:
    explicit AndroidTrackingBackend(
      UpdateFrequency updateFrequency,
      QObject *parent = nullptr
    );

    virtual ~AndroidTrackingBackend();

    QList<QgsPoint> getAllUpdates() override;

  private:
    void setupForegroundUpdates();

    qreal mDistanceFilter = 0;
    qreal mUpdateInterval = 0; // ms
};

#endif // ANDROIDTRACKINGBACKEND_H
