/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

package uk.co.lutraconsulting;

import android.Manifest;
import android.os.Build;
import android.os.IBinder;
import android.app.Service;
import android.content.Intent;
import android.app.PendingIntent;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.util.Log;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;

import android.location.Location;
import android.location.LocationManager;
import android.location.LocationListener;
import android.location.GnssStatus;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Locale;

import uk.co.lutraconsulting.MMAndroidPosition;

import java.util.Timer;
import java.util.TimerTask;

public class PositionTrackingService extends Service {

    private static final String TAG = "PositionTrackingService";

    // Channel for notifications
    public static final String CHANNEL_ID = "PositionTrackingServiceChannel";

    public static final int SERVICE_ID = 1010;
    
    private MMAndroidPosition mAndroidPos = null;

    private static native void servicePositionUpdated(Location location);

    @Override
    public void onCreate() {
        super.onCreate();

        Log.i("CPP", "[java] [service] onCreate()");

        MMAndroidPosition.Callback callback = new MMAndroidPosition.Callback() {
            @Override
            public void onPositionChanged(@NonNull Location location, GnssStatus gnssStatus) {
                Log.i("CPP", "[java] [service] new pos " + location.getLatitude() + " " + location.getLongitude());
                // notify tracking backend (c++ code)
                servicePositionUpdated(location);
            }
        };

        mAndroidPos = new MMAndroidPosition(this, callback, true);
    }

    @Override
    public IBinder onBind( Intent intent ) {
        // We don't provide binding, so return null
        return null;
    }

    @Override
    public void onDestroy() {

        Log.i("CPP", "[java] [service] onDestroy()");

        mAndroidPos.stop();

        super.onDestroy();
    }
    
    @Override
    public void onTaskRemoved(Intent rootIntent){
        Log.i("CPP", "[java] [service] onTaskRemoved()");
        
        // this does not seem to be called

        // TODO: maybe restart with AlarmManager?
        // https://stackoverflow.com/questions/26842675/continue-service-even-if-application-is-cleared-from-recent-app
        
        super.onTaskRemoved(rootIntent);
    }

    @Override
    public int onStartCommand( Intent intent, int flags, int startId ) {

        Log.i("CPP", "[java] [service] onStartCommand()");

        if ( Build.VERSION.SDK_INT < Build.VERSION_CODES.O ) {
            Log.e("CPP", "ERROR #UNSUPPORTED: tracking is not supported on your Android version ( Android O (8.0) required )" );
            stopSelf();

            return START_NOT_STICKY; // do not bother recreating it
        }

        // Build channel for notifications
        NotificationChannel serviceChannel = new NotificationChannel(
            CHANNEL_ID,
            "Foreground Service Channel",
            NotificationManager.IMPORTANCE_LOW
        );
        
        NotificationManager manager = getSystemService( NotificationManager.class );
        manager.createNotificationChannel( serviceChannel );

        // Build notification for position tracking
        Intent notificationIntent = new Intent( this, InputActivity.class );

        PendingIntent pendingIntent = PendingIntent.getActivity( this, 0, notificationIntent, PendingIntent.FLAG_IMMUTABLE );

        Notification.Builder notificationBuilder = new Notification.Builder( this, CHANNEL_ID )
            .setSmallIcon( R.drawable.ic_notification )
            .setContentTitle( "Tracking in progress" )
            .setColor( getResources().getColor( R.color.grassColor ) )
            .setContentIntent( pendingIntent );

        if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.S ) {  // only Android >= 12
            notificationBuilder.setForegroundServiceBehavior( Notification.FOREGROUND_SERVICE_IMMEDIATE );
        }

        Notification notification = notificationBuilder.build();

        // TODO: use ServiceCompat? (this variant of startForeground needs Android >= 10 (API level 29)
        startForeground( SERVICE_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION );

        mAndroidPos.start();

        return START_STICKY;
    }

}
