/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "testvariablesmanager.h"
#include "qgsexpression.h"
#include "qgsexpressioncontext.h"

#include "test/testmerginapi.h"
#include "inputtests.h"
#include "testutils.h"
#include "position/bluetoothpositionprovider.h"

TestVariablesManager::TestVariablesManager( VariablesManager *vm, PositionKit *pk, AppSettings *as )
{
  mVariablesManager = vm;
  mPositionKit = pk;
  mAppSettings = as;

  mPositionKit->setAppSettings( mAppSettings );
  mPositionKit->setPositionProvider( nullptr );

  mVariablesManager->setPositionKit( mPositionKit );
}

void TestVariablesManager::init()
{

}

void TestVariablesManager::cleanup()
{

}

void TestVariablesManager::positionVariables()
{
  mAppSettings->setGpsAntennaHeight(0);

  BluetoothPositionProvider *btProvider = new BluetoothPositionProvider( "AA:AA:FF:AA:00:10", "testBluetoothProvider" );
  mPositionKit->setPositionProvider( btProvider );

  NmeaParser parser;
  QString fullNmeaPositionFilePath = TestUtils::testDataDir() + "/position/nmea_petrzalka_full.txt";
  QFile fullNmeaFile( fullNmeaPositionFilePath );
  fullNmeaFile.open( QFile::ReadOnly );
  QVERIFY( fullNmeaFile.isOpen() );
  QgsGpsInformation position = parser.parseNmeaString( fullNmeaFile.readAll() );
  GeoPosition pos = GeoPosition::fromQgsGpsInformation( position );
  pos.verticalSpeed = 0;
  pos.magneticVariation = 0;
  emit btProvider->positionChanged( pos );

  QgsExpressionContext context;
  context << mVariablesManager->positionScope();
  evaluateExpression( QStringLiteral( "x(@position_coordinate)" ), QStringLiteral( "17.105920116666667" ), &context );
  evaluateExpression( QStringLiteral( "y(@position_coordinate)" ), QStringLiteral( "48.103135519999995" ), &context );
  evaluateExpression( QStringLiteral( "@position_latitude" ), QStringLiteral( "48.103135519999995" ), &context );
  evaluateExpression( QStringLiteral( "@position_longitude" ), QStringLiteral( "17.105920116666667" ), &context );
  evaluateExpression( QStringLiteral( "@position_altitude" ), QStringLiteral( "153.026" ), &context );
  evaluateExpression( QStringLiteral( "@position_ground_speed" ), QStringLiteral( "0.05" ), &context );
  evaluateExpression( QStringLiteral( "@position_vertical_speed" ), QStringLiteral( "0.00" ), &context );
  evaluateExpression( QStringLiteral( "@position_horizontal_accuracy" ), QStringLiteral( "0.03" ), &context );
  evaluateExpression( QStringLiteral( "@position_vertical_accuracy" ), QStringLiteral( "0.04" ), &context );
  evaluateExpression( QStringLiteral( "@position_magnetic_variation" ), QStringLiteral( "0.00" ), &context );
  evaluateExpression( QStringLiteral( "@position_direction" ), QStringLiteral( "0" ), &context );
  evaluateExpression( QStringLiteral( "@position_satellites_visible" ), QStringLiteral( "9" ), &context );
  evaluateExpression( QStringLiteral( "@position_satellites_used" ), QStringLiteral( "3" ), &context );
  evaluateExpression( QStringLiteral( "@position_hdop" ), QStringLiteral( "3.20" ), &context );
  evaluateExpression( QStringLiteral( "@position_gps_fix" ), QStringLiteral( "RTK float" ), &context );
  evaluateExpression( QStringLiteral( "@position_gps_antenna_height" ), QStringLiteral( "0.000" ), &context );
  evaluateExpression( QStringLiteral( "@position_provider_address" ), QStringLiteral( "AA:AA:FF:AA:00:10" ), &context );
  evaluateExpression( QStringLiteral( "@position_provider_name" ), QStringLiteral( "testBluetoothProvider" ), &context );
  evaluateExpression( QStringLiteral( "@position_provider_type" ), QStringLiteral( "external" ), &context );

  mAppSettings->setGpsAntennaHeight(1.6784);
  pos.verticalSpeed = 1.345;
  pos.magneticVariation = 14.34;
  emit btProvider->positionChanged( pos );
  context << mVariablesManager->positionScope();

  evaluateExpression( QStringLiteral( "@position_vertical_speed" ), QStringLiteral( "1.34" ), &context );
  evaluateExpression( QStringLiteral( "@position_magnetic_variation" ), QStringLiteral( "14.34" ), &context );
  evaluateExpression( QStringLiteral( "@position_gps_antenna_height" ), QStringLiteral( "1.678" ), &context );

  mAppSettings->setGpsAntennaHeight(0);
}

GeoPosition TestVariablesManager::testGeoPosition()
{
  GeoPosition geoInfo;
  geoInfo.utcDateTime = QDateTime::currentDateTime();
  geoInfo.latitude = -2.9207148;
  geoInfo.longitude = 51.3624998;
  geoInfo.elevation = 0.05;
  geoInfo.speed = 10.34;
  geoInfo.verticalSpeed = 11.34;
  geoInfo.hacc = 12.34;
  geoInfo.vacc = 13.34;
  geoInfo.magneticVariation = 14.34;
  geoInfo.satellitesVisible = 23;
  geoInfo.satellitesUsed = 21;
  geoInfo.hdop = 1.88;
  geoInfo.fixStatusString = QStringLiteral( "DGPS fix" );

  return geoInfo;
}

void TestVariablesManager::evaluateExpression( const QString &expStr, const QString &expectedValue, const QgsExpressionContext *context )
{
  QgsExpression exp( expStr );
  QVERIFY( exp.prepare( context ) );
  QVERIFY( !exp.hasParserError() );
  QVariant value = exp.evaluate();
  QVERIFY( !exp.hasEvalError() );
  QCOMPARE( value, expectedValue );
}
