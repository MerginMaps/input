/***************************************************************************
  app.h
  --------------------------------------
  Date                 : Nov 2017
  Copyright            : (C) 2017 by Peter Petrik
  Email                : peter.petrik@lutraconsulting.co.uk
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "loader.h"
#include <QDebug>

Loader::Loader(QObject* parent):QObject(parent)
{}

QgsProject* Loader::project() {
    return &mProject;
}

void Loader::load(const QString& filePath) {
    qDebug() << "Loading " << filePath;
    if (mProject.fileName() != filePath) {
        bool res = mProject.read(filePath);
        Q_ASSERT(res);
        qDebug() << " ******** OK ";

        emit projectReloaded();
    } else {
        qDebug() << " ******** SKIPPED ";
    }
}
