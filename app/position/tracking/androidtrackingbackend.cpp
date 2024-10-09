/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "androidtrackingbackend.h"
#include "coreutils.h"
#include "inpututils.h"
#include "androidutils.h"

#include <android/log.h>

#include <QtCore/private/qandroidextras_p.h>

static AndroidTrackingBackend *sBackend = nullptr;


void servicePositionUpdated( JNIEnv *env, jclass clazz, jobject locationObj )
{
  __android_log_print( ANDROID_LOG_INFO, "CPP", "[service] [c++] new position" );

  QJniObject location( locationObj );
  if ( !location.isValid() )
  {
    __android_log_print( ANDROID_LOG_ERROR, "CPP", "[service] [c++] invalid location obj" );
    return;
  }

  const jdouble latitude = location.callMethod<jdouble>( "getLatitude" );
  const jdouble longitude = location.callMethod<jdouble>( "getLongitude" );
  const jlong timestamp = location.callMethod<jlong>( "getTime" );

  // TODO: add time as well?
  QgsPoint pt( longitude, latitude );

  QMetaObject::invokeMethod( sBackend, "positionChanged",
                             Qt::AutoConnection, Q_ARG( QgsPoint, pt ) );
}


AndroidTrackingBackend::AndroidTrackingBackend(
  AbstractTrackingBackend::UpdateFrequency frequency,
  QObject *parent )
  : AbstractTrackingBackend(
      frequency,
      AbstractTrackingBackend::SignalSlotSupport::Supported,
      AbstractTrackingBackend::TrackingMethod::UpdatesThroughFile,
      parent
    )
{
  Q_ASSERT( sBackend == nullptr );
  sBackend = this;

  switch ( frequency )
  {
    case AbstractTrackingBackend::Often:
      mDistanceFilter = 1;
      mUpdateInterval = 1000;
      break;

    case AbstractTrackingBackend::Normal:
      mDistanceFilter = 5;
      mUpdateInterval = 3000;
      break;

    case AbstractTrackingBackend::Occasional:
      mDistanceFilter = 20;
      mUpdateInterval = 10000;
      break;
  }


  // register the native methods

  JNINativeMethod methods[]
  {
    {
      "servicePositionUpdated",
      "(Landroid/location/Location;)V",
      reinterpret_cast<void *>( servicePositionUpdated )
    }
  };

  QJniEnvironment javaenv;
  javaenv.registerNativeMethods( "uk/co/lutraconsulting/PositionTrackingService", methods, 1 );

  setupForegroundUpdates();
}

AndroidTrackingBackend::~AndroidTrackingBackend()
{
  Q_ASSERT( sBackend == this );
  sBackend = nullptr;

  // stop the foreground service
  auto activity = QJniObject( QNativeInterface::QAndroidApplication::context() );
  QAndroidIntent serviceIntent( activity.object(), "uk/co/lutraconsulting/PositionTrackingService" );

  activity.callMethod<jboolean>(
    "stopService",
    "(Landroid/content/Intent;)Z",
    serviceIntent.handle().object() );
}

QList<QgsPoint> AndroidTrackingBackend::getAllUpdates()
{
  // TODO
  return QList<QgsPoint>();
}

void AndroidTrackingBackend::setupForegroundUpdates()
{
  __android_log_print( ANDROID_LOG_INFO, "CPP", "[c++] START SERVICE!" );

  // We need to ask for a permission to show notifications,
  // but it is not mandatory to start the foreground service
  // See: https://developer.android.com/develop/ui/views/notifications/notification-permission
  if ( !AndroidUtils::requestNotificationPermission() )
  {
    emit errorOccured( tr( "Enable notifications to see tracking in the notifications tray" ) );
    CoreUtils::log( QStringLiteral( "Android Tracking Backend" ), QStringLiteral( "Notifications are disabled" ) );
  }

  auto activity = QJniObject( QNativeInterface::QAndroidApplication::context() );
  QAndroidIntent serviceIntent( activity.object(), "uk/co/lutraconsulting/PositionTrackingService" );

  // TODO
  //serviceIntent.putExtra( QStringLiteral( "uk.co.lutraconsulting.tracking.distanceInterval" ), mDistanceFilter );
  //serviceIntent.putExtra( QStringLiteral( "uk.co.lutraconsulting.tracking.timeInterval" ), mUpdateInterval );

  // startForegroundService() needs Android >= 8 (API level 26)
  QJniObject result = activity.callObjectMethod(
                        "startForegroundService",
                        "(Landroid/content/Intent;)Landroid/content/ComponentName;",
                        serviceIntent.handle().object() );
}
