/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

package uk.co.lutraconsulting;

import org.qtproject.qt.android.bindings.QtActivity;

import java.lang.Exception;

import android.util.Log;
import android.os.Bundle;
import android.os.Build;
import android.view.Display;
import android.view.Surface;
import android.view.View;
import android.view.DisplayCutout;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowInsets;
import android.view.WindowInsets.Type;
import android.view.WindowInsetsController;
import android.graphics.Insets;
import android.graphics.Color;

import android.content.Intent;
import android.net.Uri;
import android.content.ActivityNotFoundException;
import java.io.File;
import androidx.core.content.FileProvider;
import android.widget.Toast;

import androidx.core.view.WindowCompat;
import androidx.core.splashscreen.SplashScreen;

public class InputActivity extends QtActivity
{
  private static final String TAG = "Mergin Maps Input Activity";
  private boolean keepSplashScreenVisible = true;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    SplashScreen splashScreen = SplashScreen.installSplashScreen( this );
    super.onCreate(savedInstanceState);
    
    // this is to keep the screen on all the time so the device does not
    // go into sleep and recording is not interrupted
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    splashScreen.setKeepOnScreenCondition( () -> keepSplashScreenVisible );

    setCustomStatusAndNavBar();
  }

  public String homePath()
  {
    return getFilesDir().getAbsolutePath();
  }

  void setCustomStatusAndNavBar() 
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
      Log.d( TAG, "Unsupported Android version for painting behind system bars." );
      return;
    }
    else {
      WindowCompat.setDecorFitsSystemWindows(getWindow(), false);

      Window window = getWindow();

      // draw app edge-to-edge
      window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
      
      // make the status bar background color transparent
      window.setStatusBarColor(Color.TRANSPARENT);
      
      // make the navigation button background color transparent
      window.setNavigationBarColor(Color.TRANSPARENT);

      // do not show background dim for the navigation buttons
      window.setNavigationBarContrastEnforced(false); 

      // change the status bar text color to black
      WindowInsetsController insetsController = window.getDecorView().getWindowInsetsController();
    
      if (insetsController != null) {
          insetsController.setSystemBarsAppearance(WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS, WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS);
      }
    }
  }

  public String getSafeArea() {

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
      Log.d( TAG, "Unsupported Android version for painting behind system bars." );
      return ( "0,0,0,0" );
    }
    else {
      WindowInsets windowInsets = getWindow().getDecorView().getRootWindowInsets();

      if ( windowInsets == null ) {
        Log.d( TAG, "Try to ask for insets later" );
        return null;
      }

      Insets safeArea = windowInsets.getInsets( android.view.WindowInsets.Type.statusBars() | 
                                                android.view.WindowInsets.Type.navigationBars() | 
                                                android.view.WindowInsets.Type.displayCutout() );
                                                
      return ( "" + safeArea.top + "," + safeArea.right + "," + safeArea.bottom + "," + safeArea.left );
    }
  }

  public void hideSplashScreen()
  {
    keepSplashScreenVisible = false;
  }

public void showPDF(String filePath) {
  Log.d( TAG, "Expected file path: " + filePath );

  File file = new File(filePath);

  if ( !file.exists() ) {
      Log.d( TAG, "File does not exist: " + filePath );
      runOnUiThread(() -> Toast.makeText( getApplicationContext(), "File not available", Toast.LENGTH_SHORT).show() );
      return;
  } else {
      Log.d( TAG, "File exists: " + filePath );
  }

  Intent showFileIntent = new Intent( Intent.ACTION_VIEW );
  if ( showFileIntent.resolveActivity( getPackageManager() ) != null ) {

      try {
          Uri fileUri = FileProvider.getUriForFile( this, "uk.co.lutraconsulting.fileprovider", file );
          Log.d(TAG, "File URI: " + fileUri.toString());

          showFileIntent.setData( fileUri );
          showFileIntent.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_GRANT_READ_URI_PERMISSION);
          showFileIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

          startActivity(showFileIntent);
      } catch ( IllegalArgumentException e ) {
          Log.d(TAG, "FileProvider URI issue", e);
      }
  }
  else {
      runOnUiThread(() -> Toast.makeText( getApplicationContext(), "No application for opening this file", Toast.LENGTH_SHORT).show() );
  }
}


  public void quitGracefully()
  {
    String man = android.os.Build.MANUFACTURER.toUpperCase();

    Log.d( TAG, String.format("quitGracefully() from Java, MANUFACTURER: '%s'", man ) );
    
    //
    // QT app exit on back button causes crashes on some manufacturers (mainly Huawei, but also Samsung Galaxy recently).
    //
    // Let's play safe and only use this fix for HUAWEI phones for now.
    // If the fix proves itself in next release, we can add it for all other manufacturers.
    //
    // Qt bug: QTBUG-82617
    // See: https://developernote.com/2022/03/crash-at-std-thread-and-std-mutex-destructors-on-android/#comment-694101
    // See: https://stackoverflow.com/questions/61321845/qt-app-crashes-at-the-destructor-of-stdthread-on-android-10-devices
    //

    boolean shouldQuit = man.contains( "HUAWEI" );

    if ( shouldQuit )
    {
      try
      {
        finishAffinity();
        System.exit(0);
      }
      catch ( Exception exp )
      {
        exp.printStackTrace();
        Log.d( TAG, String.format( "quitGracefully() failed to execute: '%s'", exp.toString() ) );
      }
    }
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
  }
}
