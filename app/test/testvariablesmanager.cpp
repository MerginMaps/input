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
  mVariablesManager->setPositionKit( mPositionKit );
  //QVERIFY( mVariablesManager );
}

void TestVariablesManager::init()
{

}

void TestVariablesManager::cleanup()
{

}

void TestVariablesManager::positionVariables()
{
  // Init test data
  //~ GeoPosition geoInfo = testGeoPosition();
  //~ bool useGpsPoint = true;
  //~ double direction = 123.45;
  //~ double antennaHeight = 3.654;
  //~ QString providerId = QStringLiteral( "devicegps" );
  //~ QString providerName = QStringLiteral( "internal" );
  //~ QString providerType = QStringLiteral( "internal" );

  BluetoothPositionProvider *btProvider = new BluetoothPositionProvider( "AA:AA:FF:AA:00:10", "testBluetoothProvider" );
  mPositionKit->setPositionProvider( btProvider );

  NmeaParser parser;
  QString fullNmeaPositionFilePath = TestUtils::testDataDir() + "/position/nmea_petrzalka_full.txt";
  QFile fullNmeaFile( fullNmeaPositionFilePath );
  fullNmeaFile.open( QFile::ReadOnly );
  QVERIFY( fullNmeaFile.isOpen() );
  QgsGpsInformation position = parser.parseNmeaString( fullNmeaFile.readAll() );
  emit btProvider->positionChanged( GeoPosition::fromQgsGpsInformation( position ) );

  qDebug() << mPositionKit->latitude();
  qDebug() << mPositionKit->longitude();
  qDebug() << mPositionKit->horizontalAccuracy();

  QgsExpressionContext context;
  context << mVariablesManager->positionScope();
  evaluateExpression( QStringLiteral( "x(@position_coordinate)" ), QStringLiteral( "51.3624998" ), &context );
  evaluateExpression( QStringLiteral( "y(@position_coordinate)" ), QStringLiteral( "-2.9207148" ), &context );
  evaluateExpression( QStringLiteral( "@position_latitude" ), QStringLiteral( "-2.9207148" ), &context );
  evaluateExpression( QStringLiteral( "@position_longitude" ), QStringLiteral( "51.3624998" ), &context );
  evaluateExpression( QStringLiteral( "@position_altitude" ), QStringLiteral( "0.05" ), &context );
  evaluateExpression( QStringLiteral( "@position_ground_speed" ), QStringLiteral( "10.34" ), &context );
  evaluateExpression( QStringLiteral( "@position_vertical_speed" ), QStringLiteral( "11.34" ), &context );
  evaluateExpression( QStringLiteral( "@position_horizontal_accuracy" ), QStringLiteral( "12.34" ), &context );
  evaluateExpression( QStringLiteral( "@position_vertical_accuracy" ), QStringLiteral( "13.34" ), &context );
  evaluateExpression( QStringLiteral( "@position_magnetic_variation" ), QStringLiteral( "14.34" ), &context );
  evaluateExpression( QStringLiteral( "@position_direction" ), QStringLiteral( "123" ), &context );
  evaluateExpression( QStringLiteral( "@position_satellites_visible" ), QStringLiteral( "23" ), &context );
  evaluateExpression( QStringLiteral( "@position_satellites_used" ), QStringLiteral( "21" ), &context );
  evaluateExpression( QStringLiteral( "@position_hdop" ), QStringLiteral( "1.88" ), &context );
  evaluateExpression( QStringLiteral( "@position_gps_fix" ), QStringLiteral( "DGPS fix" ), &context );
  evaluateExpression( QStringLiteral( "@position_gps_antenna_height" ), QStringLiteral( "3.654" ), &context );
  evaluateExpression( QStringLiteral( "@position_provider_address" ), QStringLiteral( "devicegps" ), &context );
  evaluateExpression( QStringLiteral( "@position_provider_name" ), QStringLiteral( "internal" ), &context );
  evaluateExpression( QStringLiteral( "@position_provider_type" ), QStringLiteral( "internal" ), &context );
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
