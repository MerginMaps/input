/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "abstracttrackingbackend.h"

AbstractTrackingBackend::AbstractTrackingBackend( QObject *parent )
  : QObject{parent}
{

}

AbstractTrackingBackend::UpdateFrequency AbstractTrackingBackend::updateFrequency() const
{
  return mUpdateFrequency;
}

void AbstractTrackingBackend::setUpdateFrequency( const UpdateFrequency &newUpdateFrequency )
{
  if ( mUpdateFrequency == newUpdateFrequency )
    return;
  mUpdateFrequency = newUpdateFrequency;
  emit updateFrequencyChanged( mUpdateFrequency );
}
