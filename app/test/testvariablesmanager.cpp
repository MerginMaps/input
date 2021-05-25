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
#include "positionkit.h"

#include "test/testmerginapi.h"
#include "inputtests.h"
#include "testutils.h"

TestVariablesManager::TestVariablesManager( VariablesManager *vm )
{
  mVariablesManager = vm;
  QVERIFY( mVariablesManager );
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
  QGeoPositionInfo geoInfo = testGeoInfo();
  bool useGpsPoint = true;
  double direction = 123.45;

  QgsExpressionContext context;
  context << mVariablesManager->positionScope( geoInfo, direction, useGpsPoint );
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
}

QGeoPositionInfo TestVariablesManager::testGeoInfo()
{
  QGeoPositionInfo geoInfo;
  geoInfo.setTimestamp( QDateTime::currentDateTime() );
  geoInfo.setCoordinate( QGeoCoordinate( -2.9207148, 51.3624998, 0.05 ) );
  geoInfo.setAttribute( QGeoPositionInfo::GroundSpeed, 10.34 );
  geoInfo.setAttribute( QGeoPositionInfo::VerticalSpeed, 11.34 );
  geoInfo.setAttribute( QGeoPositionInfo::HorizontalAccuracy, 12.34 );
  geoInfo.setAttribute( QGeoPositionInfo::VerticalAccuracy, 13.34 );
  geoInfo.setAttribute( QGeoPositionInfo::MagneticVariation, 14.34 );

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

