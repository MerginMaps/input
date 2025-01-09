/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "inpututils.h"
#include "inpututils_p.h"

#include <QWindow>
#include <QScreen>
#include <QApplication>
#include <QCoreApplication>
#include <QPermissions>

#include "ios/iosutils.h"

#include "qgsruntimeprofiler.h"
#include "qcoreapplication.h"
#include "qgsgeometrycollection.h"
#include "qgslinestring.h"
#include "qgspolygon.h"
#include "qgsvectorlayer.h"
#include "inputmaptransform.h"
#include "coreutils.h"
#include "qgis.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgsdistancearea.h"
#include "qgslogger.h"
#include "qgsvectorlayer.h"
#include "qgsfeature.h"
#include "qgsapplication.h"
#include "qgsvaluerelationfieldformatter.h"
#include "qgsdatetimefieldformatter.h"
#include "qgslayertree.h"
#include "qgsprojectviewsettings.h"
#include "qgsvectorlayerutils.h"
#include "qgslinestring.h"
#include "qgspolygon.h"
#include "qgsmultipoint.h"
#include "qgsmultilinestring.h"
#include "qgsmultipolygon.h"

#include "mmstyle.h"
#include "featurelayerpair.h"
#include "inputmapsettings.h"
#include "qgsunittypes.h"
#include "qgsfeatureid.h"

#include "imageutils.h"
#include "variablesmanager.h"

#include <Qt>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <algorithm>
#include <limits>
#include <math.h>
#include <iostream>
#include <QStandardPaths>

static const QString DATE_TIME_FORMAT = QStringLiteral( "yyMMdd-hhmmss" );
static const QString INVALID_DATETIME_STR = QStringLiteral( "Invalid datetime" );

InputUtils::InputUtils( QObject *parent )
  : QObject( parent )
{
}

InputUtils::InputUtils( AndroidUtils *au, QObject *parent )
  : QObject( parent )
  , mAndroidUtils( au )
{
}

bool InputUtils::removeFile( const QString &filePath )
{
  QFile file( filePath );
  return file.remove( filePath );
}

bool InputUtils::copyFile( const QString &srcPath, const QString &dstPath )
{
  QString modSrcPath = srcPath;
  if ( srcPath.startsWith( "file://" ) )
  {
    modSrcPath = modSrcPath.replace( "file://", "" );
  }

  QFileInfo fi( dstPath );
  if ( !InputUtils::createDirectory( fi.absoluteDir().path() ) )
  {
    return false;
  }

  // https://github.com/MerginMaps/mobile/mobile/418
  // does not work for iOS files with format
  // file:assets-library://asset/asset.PNG%3Fid=A53AB989-6354-433A-9CB9-958179B7C14D&ext=PNG

  return QFile::copy( modSrcPath, dstPath );
}

bool InputUtils::createDirectory( const QString &path )
{
  QDir dir;
  return dir.mkpath( path );
}

QString InputUtils::getFileName( const QString &filePath )
{
  QFileInfo fileInfo( sanitizeName( filePath ) );
  return fileInfo.fileName();
}

QString InputUtils::sanitizeName( const QString &path )
{
#ifdef Q_OS_IOS
  QRegularExpression reAbs( "(.+)\\/asset\\.PNG%.Fid=(\\S+)&ext=" );
  QRegularExpressionMatch matchAbs = reAbs.match( path );
  if ( matchAbs.hasMatch() )
  {
    QString base = matchAbs.captured( 1 );
    QString name = matchAbs.captured( 2 );
    return base + "/" + name + ".png";
  }

  QRegularExpression reRel( "asset\\.PNG%.Fid=(\\S+)&ext=" );
  QRegularExpressionMatch matchRel = reRel.match( path );
  if ( matchRel.hasMatch() )
  {
    QString matched = matchRel.captured( 1 );
    return matched + ".png";
  }
#endif
  return path;
}

QString InputUtils::formatProjectName( const QString &fullProjectName )
{
  if ( fullProjectName.contains( "/" ) )
  {
    QStringList list = fullProjectName.split( "/" );
    if ( list.at( 0 ).isEmpty() )
    {
      return QString( "<b>%1</b>" ).arg( list.at( 1 ) );
    }
    else
    {
      return QString( "%1/<b>%2</b>" ).arg( list.at( 0 ) ).arg( list.at( 1 ) );
    }
  }
  else
  {
    return QString( "<b>%1</b>" ).arg( fullProjectName );
  }
}

QString InputUtils::formatNumber( const double number, int precision )
{
  return QString::number( number, 'f', precision );
}

QString InputUtils::formatDistanceInProjectUnit( const double distanceInMeters, int precision, QgsProject *project )
{
  if ( !project )
    return QString();

  return InputUtils::formatDistanceHelper( distanceInMeters, precision, project->distanceUnits() );
}

QString InputUtils::formatDistanceHelper( const double distanceInMeters, int precision, Qgis::DistanceUnit destUnit )
{
  if ( destUnit == Qgis::DistanceUnit::Unknown )
  {
    destUnit = Qgis::DistanceUnit::Meters;
  }

  const double factor = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, destUnit );
  const double distance = distanceInMeters * factor;
  const QString abbreviation = QgsUnitTypes::toAbbreviatedString( destUnit );

  return QString( "%L1 %2" ).arg( QString::number( distance, 'f', precision ), abbreviation );
}

QString InputUtils::formatAreaInProjectUnit( const double areaInSquareMeters, int precision, QgsProject *project )
{
  if ( !project )
    return QString();

  return InputUtils::formatAreaHelper( areaInSquareMeters, precision, project->areaUnits() );
}

QString InputUtils::formatAreaHelper( const double areaInSquareMeters, int precision, Qgis::AreaUnit destUnit )
{
  if ( destUnit == Qgis::AreaUnit::Unknown )
  {
    destUnit = Qgis::AreaUnit::SquareMeters;
  }

  const double factor = QgsUnitTypes::fromUnitToUnitFactor( Qgis::AreaUnit::SquareMeters, destUnit );
  const double area = areaInSquareMeters * factor;
  const QString abbreviation = QgsUnitTypes::toAbbreviatedString( destUnit );

  return QString( "%L1 %2" ).arg( QString::number( area, 'f', precision ), abbreviation );
}

QString InputUtils::formatDateTimeDiff( const QDateTime &tMin, const QDateTime &tMax )
{
  qint64 daysDiff = tMin.daysTo( tMax );

  // datetime is invalid
  if ( daysDiff < 0 )
  {
    return INVALID_DATETIME_STR;
  }

  // diff is maximum one day
  // Note that difference from 23:55 to 0:05 the next day counts as one day
  if ( daysDiff == 0 || daysDiff == 1 )
  {
    qint64 secsDiff = tMin.secsTo( tMax );
    if ( secsDiff < 0 )
    {
      return INVALID_DATETIME_STR;
    }
    else if ( secsDiff < 60 )
    {
      return tr( "just now" );
    }
    else if ( secsDiff < 60 * 60 )
    {
      int period = secsDiff / 60 ;
      return ( period > 1 ) ? tr( "%1 minutes ago" ).arg( period ) : tr( "%1 minute ago" ).arg( period );
    }
    else if ( secsDiff < 60 * 60 * 24 )
    {
      int period = secsDiff / ( 60 * 60 );
      return ( period > 1 ) ? tr( "%1 hours ago" ).arg( period ) : tr( "%1 hour ago" ).arg( period );
    }
    else
    {
      return ( daysDiff > 1 ) ? tr( "%1 days ago" ).arg( daysDiff ) : tr( "%1 day ago" ).arg( daysDiff );
    }
  }
  else if ( daysDiff < 7 )
  {
    return ( daysDiff > 1 ) ? tr( "%1 days ago" ).arg( daysDiff ) : tr( "%1 day ago" ).arg( daysDiff );
  }
  else if ( daysDiff < 31 )
  {
    int period = daysDiff / 7;
    return ( period > 1 ) ? tr( "%1 weeks ago" ).arg( period ) : tr( "%1 week ago" ).arg( period );
  }
  else if ( daysDiff < 365 )
  {
    int period = daysDiff / 31;
    return ( period > 1 ) ? tr( "%1 months ago" ).arg( period ) : tr( "%1 month ago" ).arg( period );
  }
  else
  {
    int period = daysDiff / 365;
    return ( period > 1 ) ? tr( "%1 years ago" ).arg( period ) : tr( "%1 year ago" ).arg( period );
  }

  return INVALID_DATETIME_STR;
}

void InputUtils::setExtentToFeature( const FeatureLayerPair &pair, InputMapSettings *mapSettings )
{

  if ( !mapSettings )
    return;

  if ( !pair.layer() )
    return;

  if ( !pair.feature().isValid() )
    return;

  QgsGeometry geom = pair.feature().geometry();
  if ( geom.isNull() || !geom.constGet() )
    return;

  QgsRectangle bbox = mapSettings->mapSettings().layerExtentToOutputExtent( pair.layer(), geom.boundingBox() );
  QgsRectangle currentExtent = mapSettings->mapSettings().extent();
  QgsPointXY currentExtentCenter = currentExtent.center();
  QgsPointXY featureCenter = bbox.center();

  double offsetX = currentExtentCenter.x() - featureCenter.x();
  double offsetY = currentExtentCenter.y() - featureCenter.y();
  currentExtent.setXMinimum( currentExtent.xMinimum() - offsetX );
  currentExtent.setXMaximum( currentExtent.xMaximum() - offsetX );
  currentExtent.setYMinimum( currentExtent.yMinimum() - offsetY );
  currentExtent.setYMaximum( currentExtent.yMaximum() - offsetY );
  mapSettings->setExtent( currentExtent );
}

QPointF InputUtils::geometryCenterToScreenCoordinates( const QgsGeometry &geom, InputMapSettings *mapSettings )
{
  QPointF screenPoint;

  if ( !mapSettings || geom.isNull() || !geom.constGet() )
    return screenPoint;

  QgsRectangle bbox = geom.boundingBox();
  screenPoint = mapSettings->coordinateToScreen( QgsPoint( bbox.center() ) );

  return screenPoint;
}

bool InputUtils::canExtentContainGeometry( const QgsGeometry &geom, InputMapSettings *mapSettings )
{
  QPointF screenPoint;

  if ( !mapSettings || geom.isNull() || !geom.constGet() )
    return false;

  QgsRectangle geomBbox = geom.boundingBox();
  QgsRectangle currentExtent = mapSettings->mapSettings().extent();

  if ( currentExtent.width() > geomBbox.width()
       && currentExtent.height() > geomBbox.height() )
  {
    return true;
  }
  return false;
}


double InputUtils::convertCoordinateString( const QString &rationalValue )
{
  QStringList values = rationalValue.split( "," );
  if ( values.size() != 3 ) return 0;

  double degrees = ratherZeroThanNaN( convertRationalNumber( values.at( 0 ) ) );
  double minutes = ratherZeroThanNaN( convertRationalNumber( values.at( 1 ) ) );
  double seconds = ratherZeroThanNaN( convertRationalNumber( values.at( 2 ) ) );

  double result = degrees + minutes / 60 + seconds / 3600;
  return result;
}

QString InputUtils::degreesString( const QgsPoint &point )
{
  if ( point.isEmpty() )
  {
    return QLatin1String();
  }

  // QGeoCoordinate formatter uses lat/long order, but we (and QGIS) use long/lat order,
  // so here we need to first pass y and then x.
  return QGeoCoordinate( point.y(), point.x() ).toString( QGeoCoordinate::DegreesMinutesWithHemisphere );
}

double InputUtils::convertRationalNumber( const QString &rationalValue )
{
  if ( rationalValue.isEmpty() )
    return std::numeric_limits<double>::quiet_NaN();

  QStringList number = rationalValue.split( "/" );
  if ( number.size() != 2 )
    return std::numeric_limits<double>::quiet_NaN();

  double numerator = number.at( 0 ).toDouble();
  double denominator = number.at( 1 ).toDouble();
  if ( denominator == 0 )
    return denominator;

  return numerator / denominator;
}

double InputUtils::mapSettingsScale( InputMapSettings *ms )
{
  if ( !ms ) return 1;
  return 1 / ms->mapUnitsPerPixel();
}

double InputUtils::mapSettingsOffsetX( InputMapSettings *ms )
{
  if ( !ms ) return 0;
  return -ms->visibleExtent().xMinimum();
}

double InputUtils::mapSettingsOffsetY( InputMapSettings *ms )
{
  if ( !ms ) return 0;
  return -ms->visibleExtent().yMaximum();
}

double InputUtils::mapSettingsDPR( InputMapSettings *ms )
{
  if ( !ms ) return 1;
  return ms->devicePixelRatio();
}

QgsGeometry InputUtils::transformGeometry( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &sourceCRS, const QgsCoordinateReferenceSystem &destinationCRS, const QgsCoordinateTransformContext &context )
{
  QgsGeometry g( geometry );

  QgsCoordinateTransform ct( sourceCRS, destinationCRS, context );
  if ( !ct.isShortCircuited() )
  {
    try
    {
      g.transform( ct );
    }
    catch ( QgsCsException &e )
    {
      Q_UNUSED( e )
      return QgsGeometry();
    }
  }

  return g;
}

QgsGeometry InputUtils::transformGeometry( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &sourceCRS, QgsVectorLayer *targetLayer )
{
  if ( !targetLayer || !targetLayer->isValid() )
  {
    return QgsGeometry();
  }

  return transformGeometry( geometry, sourceCRS, targetLayer->crs(), targetLayer->transformContext() );
}

QgsGeometry InputUtils::transformGeometryToMapWithLayer( const QgsGeometry &geometry, QgsVectorLayer *sourceLayer, InputMapSettings *targetSettings )
{
  if ( !sourceLayer || !sourceLayer->isValid() || !targetSettings )
  {
    return QgsGeometry();
  }

  return transformGeometry( geometry, sourceLayer->crs(), targetSettings->destinationCrs(), targetSettings->transformContext() );
}

QgsGeometry InputUtils::transformGeometryToMapWithCRS( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &sourceCRS, InputMapSettings *targetSettings )
{
  if ( !targetSettings )
  {
    return QgsGeometry();
  }

  return transformGeometry( geometry, sourceCRS, targetSettings->destinationCrs(), targetSettings->transformContext() );
}

QgsGeometry InputUtils::extractGeometry( const FeatureLayerPair &pair )
{
  if ( !pair.isValid() )
    return QgsGeometry();

  return pair.feature().geometry();
}

QString InputUtils::geometryLengthAsString( const QgsGeometry &geometry )
{
  QgsDistanceArea distanceArea;
  distanceArea.setEllipsoid( QStringLiteral( "WGS84" ) );
  distanceArea.setSourceCrs( QgsCoordinateReferenceSystem::fromEpsgId( 4326 ), QgsCoordinateTransformContext() );

  qreal length = distanceArea.measureLength( geometry );

  if ( qgsDoubleNear( length, 0 ) )
  {
    return "0 m";
  }

  return distanceArea.formatDistance( length, 2, distanceArea.lengthUnits() );
}

static void addLineString( const QgsLineString *line, QVector<double> &data )
{
  data << line->numPoints();
  const double *x = line->xData();
  const double *y = line->yData();
  for ( int i = 0; i < line->numPoints(); ++i )
  {
    data << x[i] << y[i];
  }
}

static void addSingleGeometry( const QgsAbstractGeometry *geom, Qgis::GeometryType type, QVector<double> &data )
{
  switch ( type )
  {
    case Qgis::GeometryType::Point:
    {
      const QgsPoint *point = qgsgeometry_cast<const QgsPoint *>( geom );
      if ( point )
      {
        data << 0 << point->x() << point->y();
      }
      break;
    }

    case Qgis::GeometryType::Line:
    {
      const QgsLineString *line = qgsgeometry_cast<const QgsLineString *>( geom );
      if ( line )
      {
        data << 1;
        addLineString( line, data );
      }
      break;
    }

    case Qgis::GeometryType::Polygon:
    {
      const QgsPolygon *poly = qgsgeometry_cast<const QgsPolygon *>( geom );
      if ( poly )
      {
        if ( const QgsLineString *line = qgsgeometry_cast<const QgsLineString *>( poly->exteriorRing() ) )
        {
          data << 2;
          addLineString( line, data );
        }
        for ( int i = 0; i < poly->numInteriorRings(); ++i )
        {
          if ( const QgsLineString *line = qgsgeometry_cast<const QgsLineString *>( poly->interiorRing( i ) ) )
          {
            data << 2;
            addLineString( line, data );
          }
        }
      }
      break;
    }

    case Qgis::GeometryType::Unknown:
    case Qgis::GeometryType::Null:
      break;
  }
}

QVector<double> InputUtils::extractGeometryCoordinates( const QgsGeometry &geometry )
{
  if ( geometry.isNull() )
    return QVector<double>();

  QVector<double> data;

  const QgsAbstractGeometry *geom = geometry.constGet();
  Qgis::GeometryType geomType = geometry.type();
  const QgsGeometryCollection *collection = qgsgeometry_cast<const QgsGeometryCollection *>( geom );
  if ( collection && !collection->isEmpty() )
  {
    for ( int i = 0; i < collection->numGeometries(); ++i )
    {
      addSingleGeometry( collection->geometryN( i ), geomType, data );
    }
  }
  else
  {
    addSingleGeometry( geom, geomType, data );
  }

  return data;
}

QString InputUtils::filesToString( QList<MerginFile> files )
{
  QStringList resultList;
  for ( MerginFile file : files )
  {
    resultList << file.path;
  }
  return resultList.join( ", " );
}

QString InputUtils::bytesToHumanSize( double bytes )
{
  return CoreUtils::bytesToHumanSize( bytes );
}

bool InputUtils::acquireCameraPermission()
{
  if ( appPlatform() == QStringLiteral( "android" ) && mAndroidUtils )
  {
    return mAndroidUtils->requestCameraPermission();
  }
  else
  {
    auto status = qApp->checkPermission( QCameraPermission{} );

    if ( Qt::PermissionStatus::Undetermined == status )
    {
      // TODO this is async so it should be treated as such
      qApp->requestPermission( QCameraPermission{}, []( const QPermission & permission ) {} );
      return false;
    }

    return status == Qt::PermissionStatus::Granted;
  }
}

bool InputUtils::isBluetoothTurnedOn()
{
  if ( appPlatform() == QStringLiteral( "android" ) && mAndroidUtils )
  {
    return mAndroidUtils->isBluetoothTurnedOn();
  }
  return true;
}

void InputUtils::turnBluetoothOn()
{
  if ( appPlatform() == QStringLiteral( "android" ) && mAndroidUtils )
  {
    mAndroidUtils->turnBluetoothOn();
  }
}

void InputUtils::quitApp()
{
  if ( appPlatform() == QStringLiteral( "android" ) )
  {
    AndroidUtils::quitApp();
  }
  else
  {
    QCoreApplication::quit();
  }
}

QString InputUtils::appPlatform()
{
#if defined( ANDROID )
  const QString platform = "android";
#elif defined( Q_OS_IOS )
  const QString platform = "ios";
#elif defined( Q_OS_WIN32 )
  const QString platform = "win";
#elif defined( Q_OS_LINUX )
  const QString platform = "linux";
#elif defined( Q_OS_MAC )
  const QString platform = "macos";
#else
  const QString platform = "unknown";
#endif
  return platform;
}

bool InputUtils::isMobilePlatform()
{
  QString platform = appPlatform();
  return platform == QStringLiteral( "android" ) || platform == QStringLiteral( "ios" );
}

QString InputUtils::appDataDir()
{
  QString dataDir = QString::fromLocal8Bit( qgetenv( "QGIS_QUICK_DATA_PATH" ) ) ;
  return dataDir;
}

void InputUtils::onQgsLogMessageReceived( const QString &message, const QString &tag, Qgis::MessageLevel level )
{
  QString levelStr;
  switch ( level )
  {
    case Qgis::MessageLevel::Warning:
      levelStr = "Warning";
      break;
    case Qgis::MessageLevel::Critical:
      levelStr = "Error";
      break;
    default:
      break;
  }

  CoreUtils::log( "QGIS " + tag, levelStr + ": " + message );
}

bool InputUtils::cpDir( const QString &srcPath, const QString &dstPath, bool onlyDiffable )
{
  bool result  = true;
  QDir parentDstDir( QFileInfo( dstPath ).path() );
  if ( !parentDstDir.mkpath( dstPath ) )
  {
    CoreUtils::log( "cpDir", QString( "Cannot make path %1" ).arg( dstPath ) );
    return false;
  }

  QDir srcDir( srcPath );
  const QFileInfoList fileInfoList = srcDir.entryInfoList( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden );
  foreach ( const QFileInfo &info, fileInfoList )
  {
    QString fileName = info.fileName();

#ifdef ANDROID
    // https://bugreports.qt.io/browse/QTBUG-114219
    if ( fileName.startsWith( "assets:/" ) )
    {
      fileName.remove( 0, 8 );
    }
#endif

    QString srcItemPath = srcPath + "/" + fileName;
    QString dstItemPath = dstPath + "/" + fileName;

    if ( info.isDir() )
    {
      if ( !cpDir( srcItemPath, dstItemPath ) )
      {
        CoreUtils::log( "cpDir", QString( "Cannot copy a dir from %1 to %2" ).arg( srcItemPath ).arg( dstItemPath ) );
        result = false;
      }
    }
    else if ( info.isFile() )
    {
      if ( onlyDiffable && !MerginApi::isFileDiffable( fileName ) )
        continue;

      if ( !QFile::copy( srcItemPath, dstItemPath ) )
      {
        if ( !QFile::remove( dstItemPath ) )
        {
          CoreUtils::log( "cpDir", QString( "Cannot remove a file from %1" ).arg( dstItemPath ) );
          result =  false;
        }
        if ( !QFile::copy( srcItemPath, dstItemPath ) )
        {
          CoreUtils::log( "cpDir", QString( "Cannot overwrite a file %1 with %2" ).arg( dstItemPath ).arg( dstItemPath ) );
          result =  false;
        }
      }
      QFile::setPermissions( dstItemPath, QFile::ReadUser | QFile::WriteUser | QFile::ReadOwner | QFile::WriteOwner );
    }
    else
    {
      CoreUtils::log( "cpDir", QString( "Unhandled item %1 in cpDir" ).arg( info.filePath() ) );
    }
  }
  return result;
}

QString InputUtils::renameWithDateTime( const QString &srcPath, const QDateTime &dateTime )
{
  if ( QFile::exists( srcPath ) )
  {
    QFileInfo info( srcPath );
    QString timestamp = ( dateTime.isValid() ) ? dateTime.toString( DATE_TIME_FORMAT ) : QDateTime::currentDateTime().toString( DATE_TIME_FORMAT );
    QString newFilename = QString( "%1.%2" ).arg( timestamp ).arg( info.suffix() );
    QString newPath( info.absolutePath() + "/" + newFilename );

    if ( QFile::rename( srcPath, newPath ) ) return newPath;
  }

  return QString();
}

bool InputUtils::renameFile( const QString &srcPath, const QString &dstPath )
{
  QFileInfo fi( dstPath );
  if ( !InputUtils::createDirectory( fi.absoluteDir().path() ) )
  {
    return false;
  }
  return QFile::rename( srcPath, dstPath );
}

QString InputUtils::htmlLink(
  const QString &text,
  const QColor &color,
  const QString &url,
  const QString &url2,
  bool underline,
  bool bold )
{
  return InputUtilsPrivate::htmlLink(
           text, color, url, url2, underline, bold
         );
}

double InputUtils::ratherZeroThanNaN( double d )
{
  return ( isnan( d ) ) ? 0.0 : d;
}

/**
 * Makes QgsCoordinateReferenceSystem::fromEpsgId accessible for QML components
 */
QgsCoordinateReferenceSystem InputUtils::coordinateReferenceSystemFromEpsgId( long epsg )
{
  return QgsCoordinateReferenceSystem::fromEpsgId( epsg );
}

QgsPointXY InputUtils::pointXY( double x, double y )
{
  return QgsPointXY( x, y );
}

QgsPoint InputUtils::point( double x, double y, double z, double m )
{
  return QgsPoint( x, y, z, m );
}

QgsGeometry InputUtils::emptyGeometry()
{
  return QgsGeometry();
}

QgsFeature InputUtils::emptyFeature()
{
  return QgsFeature();
}

bool InputUtils::isEmptyGeometry( const QgsGeometry &geometry )
{
  return geometry.isEmpty();
}

QgsPoint InputUtils::coordinateToPoint( const QGeoCoordinate &coor )
{
  return QgsPoint( coor.longitude(), coor.latitude(), coor.altitude() );
}

QgsPointXY InputUtils::transformPointXY( const QgsCoordinateReferenceSystem &srcCrs,
    const QgsCoordinateReferenceSystem &destCrs,
    const QgsCoordinateTransformContext &context,
    const QgsPointXY &srcPoint )
{
  // we do not want to transform empty points,
  // QGIS would convert them to a valid (0, 0) points
  if ( srcPoint.isEmpty() )
  {
    return QgsPointXY();
  }

  try
  {
    QgsCoordinateTransform ct( srcCrs, destCrs, context );
    if ( ct.isValid() )
    {
      if ( !ct.isShortCircuited() )
      {
        const QgsPointXY pt = ct.transform( srcPoint );
        return pt;
      }
      else
      {
        return srcPoint;
      }
    }
  }
  catch ( QgsCsException &cse )
  {
    Q_UNUSED( cse )
  }

  return QgsPointXY();
}

QgsPoint InputUtils::transformPoint( const QgsCoordinateReferenceSystem &srcCrs,
                                     const QgsCoordinateReferenceSystem &destCrs,
                                     const QgsCoordinateTransformContext &context,
                                     const QgsPoint &srcPoint )
{
  // we do not want to transform empty points,
  // QGIS would convert them to a valid (0, 0) points
  if ( srcPoint.isEmpty() )
  {
    return QgsPoint();
  }

  try
  {
    QgsCoordinateTransform ct( srcCrs, destCrs, context );
    if ( ct.isValid() )
    {
      if ( !ct.isShortCircuited() )
      {
        const QgsPointXY transformed = ct.transform( srcPoint.x(), srcPoint.y() );
        const QgsPoint pt( transformed.x(), transformed.y(), srcPoint.z(), srcPoint.m() );
        return pt;
      }
      else
      {
        return srcPoint;
      }
    }
  }
  catch ( QgsCsException &cse )
  {
    Q_UNUSED( cse )
  }

  return QgsPoint();
}

QPointF InputUtils::transformPointToScreenCoordinates( const QgsCoordinateReferenceSystem &srcCrs, InputMapSettings *mapSettings, const QgsPoint &srcPoint )
{
  if ( !mapSettings || srcPoint.isEmpty() )
    return QPointF();

  QgsPoint mapcrsPoint = transformPoint( srcCrs, mapSettings->destinationCrs(), mapSettings->transformContext(), srcPoint );
  return mapSettings->coordinateToScreen( mapcrsPoint );
}

double InputUtils::screenUnitsToMeters( InputMapSettings *mapSettings, int baseLengthPixels )
{
  if ( mapSettings == nullptr ) return 0.0;

  QgsDistanceArea mDistanceArea;
  mDistanceArea.setEllipsoid( QStringLiteral( "WGS84" ) );
  mDistanceArea.setSourceCrs( mapSettings->destinationCrs(), mapSettings->transformContext() );

  // calculate the geographic distance from the central point of extent
  // to the specified number of points on the right side
  QSize s = mapSettings->outputSize();
  QPoint pointCenter( s.width() / 2, s.height() / 2 );
  QgsPointXY p1 = mapSettings->screenToCoordinate( pointCenter );
  QgsPointXY p2 = mapSettings->screenToCoordinate( pointCenter + QPoint( baseLengthPixels, 0 ) );
  return mDistanceArea.measureLine( p1, p2 );
}

QgsPoint InputUtils::mapPointToGps( QPointF mapPosition, InputMapSettings *mapSettings )
{
  if ( !mapSettings )
    return QgsPoint();

  if ( mapPosition.isNull() )
    return QgsPoint();

  QgsPoint positionMapCrs = mapSettings->screenToCoordinate( mapPosition );
  QgsCoordinateReferenceSystem crsGPS = coordinateReferenceSystemFromEpsgId( 4326 );

  const QgsPointXY transformedXY = transformPoint(
                                     mapSettings->destinationCrs(),
                                     crsGPS,
                                     QgsCoordinateTransformContext(),
                                     positionMapCrs
                                   );

  if ( transformedXY.isEmpty() )
  {
    // point could not be transformed
    return QgsPoint();
  }

  return QgsPoint( transformedXY );
}

bool InputUtils::fileExists( const QString &path )
{
  QFileInfo check_file( path );
  // check if file exists and if yes: Is it really a file and no directory?
  return ( check_file.exists() && check_file.isFile() );
}

QString InputUtils::resolveTargetDir( const QString &homePath, const QVariantMap &config, const FeatureLayerPair &pair, QgsProject *activeProject )
{
  QString expression;
  QMap<QString, QVariant> collection = config.value( QStringLiteral( "PropertyCollection" ) ).toMap();
  QMap<QString, QVariant> props = collection.value( QStringLiteral( "properties" ) ).toMap();

  if ( !props.isEmpty() )
  {
    QMap<QString, QVariant> propertyRootPath = props.value( QStringLiteral( "propertyRootPath" ) ).toMap();
    expression = propertyRootPath.value( QStringLiteral( "expression" ), QString() ).toString();
  }

  if ( !expression.isEmpty() )
  {
    return evaluateExpression( pair, activeProject, expression );
  }
  else
  {
    QString defaultRoot = config.value( QStringLiteral( "DefaultRoot" ) ).toString();
    if ( defaultRoot.isEmpty() )
    {
      return homePath;
    }
    else
    {
      return defaultRoot;
    }
  }
}

QString InputUtils::resolvePrefixForRelativePath( int relativeStorageMode, const QString &homePath, const QString &targetDir )
{
  if ( relativeStorageMode == 1 )
  {
    return homePath;
  }
  else if ( relativeStorageMode == 2 )
  {
    return targetDir;
  }
  else
  {
    return QString();
  }
}

QString InputUtils::getAbsolutePath( const QString &path, const QString &prefixPath )
{
  return ( prefixPath.isEmpty() ) ? path : QStringLiteral( "%1/%2" ).arg( prefixPath ).arg( path );
}

QString InputUtils::resolvePath( const QString &path, const QString &homePath, const QVariantMap &config, const FeatureLayerPair &pair, QgsProject *activeProject )
{
  int relativeStorageMode = config.value( QStringLiteral( "RelativeStorage" ) ).toInt();
  QString targetDir = resolveTargetDir( homePath, config, pair, activeProject );
  QString prefixToRelativePath = resolvePrefixForRelativePath( relativeStorageMode, homePath, targetDir );

  return getAbsolutePath( path, prefixToRelativePath );
}

QString InputUtils::getRelativePath( const QString &path, const QString &prefixPath )
{
  QString modPath = path;
  QString filePrefix( "file://" );

  if ( path.startsWith( filePrefix ) )
  {
    modPath = modPath.replace( filePrefix, QString() );
  }

  if ( prefixPath.isEmpty() ) return modPath;

  // Do not use a canonical path for non-existing path
  if ( !QFileInfo( path ).exists() )
  {
    if ( !prefixPath.isEmpty() && modPath.startsWith( prefixPath ) )
    {
      return modPath.replace( prefixPath, QString() );
    }
  }
  else
  {
    QDir absoluteDir( modPath );
    QDir prefixDir( prefixPath );
    QString canonicalPath = absoluteDir.canonicalPath();
    QString prefixCanonicalPath = prefixDir.canonicalPath() + "/";

    if ( prefixCanonicalPath.length() > 1 && canonicalPath.startsWith( prefixCanonicalPath ) )
    {
      return canonicalPath.replace( prefixCanonicalPath, QString() );
    }
  }

  return QString();
}

void InputUtils::logMessage( const QString &message, const QString &tag, Qgis::MessageLevel level )
{
  QgsMessageLog::logMessage( message, tag, level );
}

void InputUtils::log( const QString &context, const QString &message )
{
  CoreUtils::log( context, message );
}

const QUrl InputUtils::getThemeIcon( const QString &name )
{
  QString path = QStringLiteral( "qrc:/%1.svg" ).arg( name );
  QgsDebugMsgLevel( QStringLiteral( "Using icon %1 from %2" ).arg( name, path ), 2 );
  return QUrl( path );
}

const QUrl InputUtils::getFormEditorType( const QString &widgetNameIn, const QVariantMap &config, const QgsField &field, const QgsRelation &relation, const QString &editorTitle )
{
  QString widgetName = widgetNameIn.toLower();

  QString path( "../form/editors/%1.qml" );

  if ( widgetName == QStringLiteral( "range" ) )
  {
    if ( config.contains( "Style" ) )
    {
      if ( config["Style"] == QStringLiteral( "Slider" ) )
      {
        return QUrl( path.arg( QLatin1String( "MMFormSliderEditor" ) ) );
      }
      else if ( config["Style"] == QStringLiteral( "SpinBox" ) )
      {
        return QUrl( path.arg( QLatin1String( "MMFormNumberEditor" ) ) );
      }
    }
    return QUrl( path.arg( QLatin1String( "MMFormTextEditor" ) ) );
  }
  else if ( widgetName == QStringLiteral( "datetime" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormCalendarEditor" ) ) );
  }
  else if ( field.name().contains( "qrcode", Qt::CaseInsensitive ) || field.alias().contains( "qrcode", Qt::CaseInsensitive ) )
  {
    return QUrl( path.arg( QStringLiteral( "MMFormScannerEditor" ) ) );
  }
  else if ( widgetName == QStringLiteral( "textedit" ) )
  {
    if ( config.value( "IsMultiline" ).toBool() )
    {
      return QUrl( path.arg( QStringLiteral( "MMFormTextMultilineEditor" ) ) );
    }
    return QUrl( path.arg( QLatin1String( "MMFormTextEditor" ) ) );
  }
  else if ( widgetName == QStringLiteral( "checkbox" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormSwitchEditor" ) ) );
  }
  else if ( widgetName == QStringLiteral( "valuerelation" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormValueRelationEditor" ) ) );
  }
  else if ( widgetName == QStringLiteral( "valuemap" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormValueMapEditor" ) ) );
  }
  else if ( widgetName == QStringLiteral( "externalresource" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormPhotoEditor" ) ) );
  }
  else if ( widgetName == QStringLiteral( "richtext" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormRichTextViewer" ) ) );
  }
  else if ( widgetName == QStringLiteral( "spacer" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormSpacer" ) ) );
  }
  else if ( widgetName == QStringLiteral( "relation" ) )
  {
    // check if we should use gallery or word tags
    bool useGallery = false;

    QgsVectorLayer *layer = relation.referencingLayer();
    if ( layer && layer->isValid() )
    {
      QgsFields fields = layer->fields();
      for ( int i = 0; i < fields.size(); i++ )
      {
        // Lets try by widget type
        QgsEditorWidgetSetup setup = layer->editorWidgetSetup( i );
        if ( setup.type() == QStringLiteral( "ExternalResource" ) )
        {
          useGallery = true;
          break;
        }
      }
    }

    // Mind this hack - fields with `no-gallery-use` won't use gallery, but normal word tags instead
    if ( editorTitle.contains( "nogallery", Qt::CaseInsensitive ) )
    {
      useGallery = false;
    }

    if ( useGallery )
    {
      return QUrl( path.arg( QLatin1String( "MMFormGalleryEditor" ) ) );
    }
    else
    {
      return QUrl( path.arg( QLatin1String( "MMFormRelationEditor" ) ) );
    }
  }
  else if ( widgetName == QStringLiteral( "relationreference" ) )
  {
    return QUrl( path.arg( QLatin1String( "MMFormRelationReferenceEditor" ) ) );
  }

  return QUrl( path.arg( QLatin1String( "MMFormTextEditor" ) ) );
}

const QgsEditorWidgetSetup InputUtils::getEditorWidgetSetup( const QgsField &field )
{
  if ( field.isNumeric() )
    return getEditorWidgetSetup( field, QStringLiteral( "Range" ) );
  else if ( field.isDateOrTime() )
    return getEditorWidgetSetup( field, QStringLiteral( "DateTime" ) );
  else if ( field.type() == QVariant::Bool )
    return getEditorWidgetSetup( field, QStringLiteral( "CheckBox" ) );
  else
    return getEditorWidgetSetup( field, QStringLiteral( "TextEdit" ) );
}

const QgsEditorWidgetSetup InputUtils::getEditorWidgetSetup( const QgsField &field, const QString &widgetType, const QVariantMap &additionalArgs )
{
  if ( field.name() == QStringLiteral( "fid" ) )
    return QgsEditorWidgetSetup( QStringLiteral( "Hidden" ), QVariantMap() );

  if ( widgetType.isEmpty() )
  {
    return QgsEditorWidgetSetup( QStringLiteral( "TextEdit" ), QVariantMap() );
  }
  else
  {
    QMultiMap<QString, QVariant> config;
    config = config.unite( QMultiMap( additionalArgs ) );

    if ( widgetType == QStringLiteral( "TextEdit" ) )
    {
      config.insert( QStringLiteral( "isMultiline" ), false );
      config.insert( QStringLiteral( "UseHtml" ), false );
    }
    else if ( widgetType == QStringLiteral( "DateTime" ) )
    {
      config.insert( QStringLiteral( "field_format" ), QgsDateTimeFieldFormatter::DATETIME_FORMAT );
      config.insert( QStringLiteral( "display_format" ), QgsDateTimeFieldFormatter::DATETIME_FORMAT );
    }
    else if ( widgetType == QStringLiteral( "Range" ) )
    {
      config.insert( QStringLiteral( "Style" ), QStringLiteral( "SpinBox" ) );
      config.insert( QStringLiteral( "Precision" ), QStringLiteral( "0" ) );
      config.insert( QStringLiteral( "Min" ), QString::number( INT_MIN ) );
      config.insert( QStringLiteral( "Max" ), QString::number( INT_MAX ) );
      config.insert( QStringLiteral( "Step" ), 1 );
    }
    else if ( widgetType == QStringLiteral( "ExternalResource" ) )
    {
      config.insert( QStringLiteral( "RelativeStorage" ), QStringLiteral( "1" ) );
      config.insert( QStringLiteral( "StorageMode" ), QStringLiteral( "0" ) );
      config.insert( QStringLiteral( "PropertyCollection" ), QVariantMap() );
      QgsPropertyCollection collection;
      config.insert( QStringLiteral( "PropertyCollection" ), collection.toVariant( QgsPropertiesDefinition() ) );
    }
    else if ( widgetType == QStringLiteral( "RelationReference" ) )
    {
      config.insert( QStringLiteral( "AllowNULL" ), true );
    }

    QVariantMap cfg;
    QList<QString> keys = config.uniqueKeys();
    for ( int i = 0; i < keys.size(); i++ )
    {
      cfg.insert( keys.at( i ), config.value( keys.at( i ) ) );
    }

    return QgsEditorWidgetSetup( widgetType, cfg );
  }
}

QString InputUtils::geometryFromLayer( QgsVectorLayer *layer )
{
  if ( layer )
  {
    switch ( layer->geometryType() )
    {
      case Qgis::GeometryType::Point: return QStringLiteral( "point" );
      case Qgis::GeometryType::Line: return QStringLiteral( "linestring" );
      case Qgis::GeometryType::Polygon: return QStringLiteral( "polygon" );
      case Qgis::GeometryType::Null: return QStringLiteral( "nullGeo" );
      default: return QString();
    }
  }
  return QString();
}

bool InputUtils::isPointLayer( QgsVectorLayer *layer )
{
  return geometryFromLayer( layer ) == "point";
}

bool InputUtils::isLineLayer( QgsVectorLayer *layer )
{
  return geometryFromLayer( layer ) == "linestring";
}

bool InputUtils::isPolygonLayer( QgsVectorLayer *layer )
{
  return geometryFromLayer( layer ) == "polygon";
}

bool InputUtils::isNoGeometryLayer( QgsVectorLayer *layer )
{
  return geometryFromLayer( layer ) == "nullGeo";
}

bool InputUtils::isMultiPartLayer( QgsVectorLayer *layer )
{
  if ( !layer )
  {
    return false;
  }
  return QgsWkbTypes::isMultiType( layer->wkbType() );
}

bool InputUtils::isSpatialLayer( QgsVectorLayer *layer )
{
  if ( !layer )
  {
    return false;
  }
  return layer->isSpatial();
}

qreal InputUtils::calculateScreenDpr()
{
  return InputUtilsPrivate::calculateScreenDpr();
}

qreal InputUtils::calculateDpRatio()
{
  return InputUtilsPrivate::calculateDpRatio();
}

bool InputUtils::equals( const QPointF &a, const QPointF &b, double epsilon )
{
  return qgsDoubleNear( a.x(), b.x(), epsilon ) && qgsDoubleNear( a.y(), b.y(), epsilon );
}

bool InputUtils::equals( const QgsPointXY &a, const QgsPointXY &b, double epsilon )
{
  if ( a.isEmpty() && b.isEmpty() )
    return true;
  if ( a.isEmpty() != b.isEmpty() )
    return false;

  return qgsDoubleNear( a.x(), b.x(), epsilon ) && qgsDoubleNear( a.y(), b.y(), epsilon );
}

bool InputUtils::equals( const QgsPoint &a, const QgsPoint &b, double epsilon )
{
  if ( a.isEmpty() && b.isEmpty() )
    return true;
  if ( a.isEmpty() != b.isEmpty() )
    return false;

  return qgsDoubleNear( a.x(), b.x(), epsilon ) && qgsDoubleNear( a.y(), b.y(), epsilon );
}

QString InputUtils::formatPoint(
  const QgsPoint &point,
  QgsCoordinateFormatter::Format format,
  int decimals,
  QgsCoordinateFormatter::FormatFlags flags )
{
  return QgsCoordinateFormatter::format( point, format, decimals, flags );
}

QString InputUtils::formatDistance( double distance,
                                    Qgis::DistanceUnit units,
                                    int decimals,
                                    Qgis::SystemOfMeasurement destSystem )
{
  double destDistance;
  Qgis::DistanceUnit destUnits;

  humanReadableDistance( distance, units, destSystem, destDistance, destUnits );

  return QStringLiteral( "%1 %2" )
         .arg( QString::number( destDistance, 'f', decimals ) )
         .arg( QgsUnitTypes::toAbbreviatedString( destUnits ) );
}

void InputUtils::humanReadableDistance( double srcDistance, Qgis::DistanceUnit srcUnits,
                                        Qgis::SystemOfMeasurement destSystem,
                                        double &destDistance, Qgis::DistanceUnit &destUnits )
{
  if ( ( destSystem == Qgis::SystemOfMeasurement::Metric ) || ( destSystem == Qgis::SystemOfMeasurement::Unknown ) )
  {
    return formatToMetricDistance( srcDistance, srcUnits, destDistance, destUnits );
  }
  else if ( destSystem == Qgis::SystemOfMeasurement::Imperial )
  {
    return formatToImperialDistance( srcDistance, srcUnits, destDistance, destUnits );
  }
  else if ( destSystem == Qgis::SystemOfMeasurement::USCS )
  {
    return formatToUSCSDistance( srcDistance, srcUnits, destDistance, destUnits );
  }
  else
  {
    Q_ASSERT( false ); //should never happen
  }
}

void InputUtils::formatToMetricDistance( double srcDistance,
    Qgis::DistanceUnit srcUnits,
    double &destDistance,
    Qgis::DistanceUnit &destUnits )
{
  double dist = srcDistance * QgsUnitTypes::fromUnitToUnitFactor( srcUnits, Qgis::DistanceUnit::Millimeters );
  if ( dist < 0 )
  {
    destDistance = 0;
    destUnits = Qgis::DistanceUnit::Millimeters;
    return;
  }

  double mmToKm = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Kilometers, Qgis::DistanceUnit::Millimeters );
  if ( dist > mmToKm )
  {
    destDistance = dist / mmToKm;
    destUnits = Qgis::DistanceUnit::Kilometers;
    return;
  }

  double mmToM = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, Qgis::DistanceUnit::Millimeters );
  if ( dist > mmToM )
  {
    destDistance = dist / mmToM;
    destUnits = Qgis::DistanceUnit::Meters;
    return;
  }

  double mmToCm = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Centimeters, Qgis::DistanceUnit::Millimeters );
  if ( dist > mmToCm )
  {
    destDistance = dist / mmToCm;
    destUnits = Qgis::DistanceUnit::Centimeters;
    return;
  }

  destDistance = dist;
  destUnits = Qgis::DistanceUnit::Millimeters;
}

void InputUtils::formatToImperialDistance( double srcDistance,
    Qgis::DistanceUnit srcUnits,
    double &destDistance,
    Qgis::DistanceUnit &destUnits )
{
  double dist = srcDistance * QgsUnitTypes::fromUnitToUnitFactor( srcUnits, Qgis::DistanceUnit::Feet );
  if ( dist < 0 )
  {
    destDistance = 0;
    destUnits = Qgis::DistanceUnit::Feet;
    return;
  }

  double feetToMile = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Miles, Qgis::DistanceUnit::Feet );
  if ( dist > feetToMile )
  {
    destDistance = dist / feetToMile;
    destUnits = Qgis::DistanceUnit::Miles;
    return;
  }

  double feetToYard = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Yards, Qgis::DistanceUnit::Feet );
  if ( dist > feetToYard )
  {
    destDistance = dist / feetToYard;
    destUnits = Qgis::DistanceUnit::Yards;
    return;
  }

  destDistance = dist;
  destUnits = Qgis::DistanceUnit::Feet;
  return;
}

void InputUtils::formatToUSCSDistance( double srcDistance,
                                       Qgis::DistanceUnit srcUnits,
                                       double &destDistance,
                                       Qgis::DistanceUnit &destUnits )
{
  double dist = srcDistance * QgsUnitTypes::fromUnitToUnitFactor( srcUnits, Qgis::DistanceUnit::Feet );
  if ( dist < 0 )
  {
    destDistance = 0;
    destUnits = Qgis::DistanceUnit::Feet;
    return;
  }

  double feetToMile = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::NauticalMiles, Qgis::DistanceUnit::Feet );
  if ( dist > feetToMile )
  {
    destDistance = dist / feetToMile;
    destUnits = Qgis::DistanceUnit::NauticalMiles;
    return;
  }

  double feetToYard = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Yards, Qgis::DistanceUnit::Feet );
  if ( dist > feetToYard )
  {
    destDistance = dist / feetToYard;
    destUnits = Qgis::DistanceUnit::Yards;
    return;
  }

  destDistance = dist;
  destUnits = Qgis::DistanceUnit::Feet;
  return;
}

QString InputUtils::dumpScreenInfo() const
{
  QString msg;
  // take the first top level window
  const QWindowList windows = QGuiApplication::topLevelWindows();
  if ( !windows.isEmpty() )
  {
    QScreen *screen = windows.at( 0 )->screen();
    double dpiX = screen->physicalDotsPerInchX();
    double dpiY = screen->physicalDotsPerInchY();
    int height = screen->geometry().height();
    int width = screen->geometry().width();
    double sizeX = static_cast<double>( width ) / dpiX * 25.4;
    double sizeY = static_cast<double>( height ) / dpiY * 25.4;

    msg += tr( "screen resolution: %1x%2 px\n" ).arg( width ).arg( height );
    msg += tr( "screen DPI: %1x%2\n" ).arg( dpiX ).arg( dpiY );
    msg += tr( "screen size: %1x%2 mm\n" ).arg( QString::number( sizeX, 'f', 0 ), QString::number( sizeY, 'f', 0 ) );
    msg += tr( "reported device pixel ratio: %1\n" ).arg( screen->devicePixelRatio() );
    msg += tr( "calculated device pixel ratio: %1\n" ).arg( calculateScreenDpr() );
    msg += tr( "used dp scale: %1" ).arg( calculateDpRatio() );
  }
  else
  {
    msg += QLatin1String( "screen info: application is not initialized!" );
  }
  return msg;
}

QString InputUtils::evaluateExpression( const FeatureLayerPair &pair, QgsProject *activeProject, const QString &expression )
{
  QList<QgsExpressionContextScope *> scopes;
  scopes << QgsExpressionContextUtils::globalScope();
  scopes << QgsExpressionContextUtils::projectScope( activeProject );
  scopes << QgsExpressionContextUtils::layerScope( pair.layer() );

  QgsExpressionContext context( scopes );
  context.setFeature( pair.feature() );
  QgsExpression expr( expression );
  return expr.evaluate( &context ).toString();
}

QString InputUtils::fieldType( const QgsField &field )
{
  return QVariant( field.type() ).typeName();
}

QString InputUtils::dateTimeFieldFormat( const QString &fieldFormat )
{
  if ( QgsDateTimeFieldFormatter::DATE_FORMAT == fieldFormat )
  {
    return QString( "Date" );
  }
  else if ( QgsDateTimeFieldFormatter::TIME_FORMAT == fieldFormat )
  {
    return QString( "Time" );
  }
  // cppcheck-suppress duplicateBranch
  else if ( QgsDateTimeFieldFormatter::DATETIME_FORMAT == fieldFormat )
  {
    return QString( "Date Time" );
  }
  else
  {
    return QString( "Date Time" );
  }
}

bool InputUtils::isFeatureIdValid( qint64 featureId )
{
  return !FID_IS_NEW( featureId ) && !FID_IS_NULL( featureId );
}

QgsRectangle InputUtils::stakeoutPathExtent(
  MapPosition *mapPosition,
  const FeatureLayerPair &targetFeature,
  InputMapSettings *mapSettings,
  double mapExtentOffset
)
{
  if ( !mapPosition || !mapSettings || !targetFeature.isValid() )
    return QgsRectangle();

  QgsRectangle extent = mapSettings->extent();

  // We currently support only point geometries
  if ( targetFeature.layer()->geometryType() != Qgis::GeometryType::Point )
    return extent;

  if ( !mapPosition->positionKit() || !mapPosition->mapSettings() )
    return extent;

  //
  // In order to compute stakeout extent, we first compute distance to target feature and
  // based on that we update the extent and scale. Logic for scale computation is in distanceToScale function.
  // Moreover, when distance to target point is lower then 1 meter, extent is centered to target point, otherwise
  // it is centered to GPS position. This has been added in order to reduce "jumps" of canvas when user is near the target.
  //

  QgsPoint gpsPointRaw = mapPosition->positionKit()->positionCoordinate();

  qreal distance = distanceBetweenGpsAndFeature( gpsPointRaw, targetFeature, mapSettings );
  qreal scale = distanceToScale( distance );
  qreal panelOffset = 0; // (px) used as an offset in order to center point in visible extent (we center to gpsPoint/target point + this offset)

  if ( mapExtentOffset > 0 )
  {
    panelOffset = mapExtentOffset / 2.0;
  }

  if ( distance <= 1 )
  {
    // center to target point
    QgsPoint targetPointRaw( extractPointFromFeature( targetFeature ) );
    QgsPointXY targetPointInMapCRS = transformPoint(
                                       targetFeature.layer()->crs(),
                                       mapSettings->destinationCrs(),
                                       mapSettings->transformContext(),
                                       targetPointRaw
                                     );

    if ( targetPointInMapCRS.isEmpty() )
    {
      // unsuccessful transform
      return extent;
    }

    QgsPointXY targetPointInCanvasXY = mapSettings->coordinateToScreen( QgsPoint( targetPointInMapCRS ) );
    QgsPointXY centerInCanvasXY( targetPointInCanvasXY.x(), targetPointInCanvasXY.y() + panelOffset );
    QgsPointXY center = mapSettings->screenToCoordinate( centerInCanvasXY.toQPointF() );

    extent = mapSettings->mapSettings().computeExtentForScale( center, scale );
  }
  else
  {
    // center to GPS position
    QgsPointXY gpsPointInCanvasXY = mapPosition->screenPosition();
    QgsPointXY centerInCanvasXY( gpsPointInCanvasXY.x(), gpsPointInCanvasXY.y() + panelOffset );
    QgsPointXY center = mapSettings->screenToCoordinate( centerInCanvasXY.toQPointF() );

    extent = mapSettings->mapSettings().computeExtentForScale( center, scale );
  }

  return extent;
}

QgsGeometry InputUtils::stakeoutGeometry( const QgsPoint &mapPosition, const FeatureLayerPair &target, InputMapSettings *mapSettings )
{
  if ( !mapSettings || !target.isValid() )
    return QgsGeometry();

  QgsPointXY targetInLayerCoordinates = target.feature().geometry().asPoint();
  QgsPointXY t = transformPointXY( target.layer()->crs(), mapSettings->destinationCrs(), mapSettings->transformContext(), targetInLayerCoordinates );

  QVector<QgsPoint> points { mapPosition, QgsPoint( t ) };

  return QgsGeometry::fromPolyline( points );
}

qreal InputUtils::distanceToScale( qreal distance )
{
  // Stakeout extent scale is computed based on these (empirically found) conditions:
  //   - if distance is > 10m, use 1:205 scale (~ 5m on mobile)
  //   - if distance is 3-10m, use 1:105 scale (~ 2m on mobile)
  //   - if distance is 1-3m,  use 1:55 scale  (~ 1m on mobile)
  //   - if distance is < 1m,  use 1:25 scale  (~ 0.5m on mobile)

  qreal scale = 205;

  if ( distance <= 1 )
  {
    scale = 25;
  }
  else if ( distance <= 3 && distance > 1 )
  {
    scale = 55;
  }
  else if ( distance <= 10 && distance > 3 )
  {
    scale = 105;
  }

  return scale;
}

qreal InputUtils::distanceBetweenGpsAndFeature( QgsPoint gpsPosition, const FeatureLayerPair &targetFeature, InputMapSettings *mapSettings )
{
  if ( !mapSettings || !targetFeature.isValid() )
    return -1;

  // We calculate distance only between points
  if ( targetFeature.layer()->geometryType() != Qgis::GeometryType::Point )
    return -1;

  // Transform gps position to map CRS
  QgsPointXY transformedPosition = transformPoint(
                                     coordinateReferenceSystemFromEpsgId( 4326 ),
                                     mapSettings->destinationCrs(),
                                     mapSettings->transformContext(),
                                     gpsPosition
                                   );

  if ( transformedPosition.isEmpty() )
  {
    return -1;
  }

  // Transform target point to map CRS
  QgsPoint target( extractPointFromFeature( targetFeature ) );
  QgsPointXY transformedTarget = transformPoint(
                                   targetFeature.layer()->crs(),
                                   mapSettings->destinationCrs(),
                                   mapSettings->transformContext(),
                                   target
                                 );

  if ( transformedTarget.isEmpty() )
  {
    return -1;
  }

  QgsDistanceArea distanceArea;
  distanceArea.setSourceCrs( mapSettings->destinationCrs(), mapSettings->transformContext() );

  qreal distance = distanceArea.measureLine( transformedPosition, transformedTarget );
  distance = distanceArea.convertLengthMeasurement( distance, Qgis::DistanceUnit::Meters );

  return distance;
}

qreal InputUtils::angleBetweenGpsAndFeature( QgsPoint gpsPoint, const FeatureLayerPair &targetFeature, InputMapSettings *mapSettings )
{
  if ( !mapSettings || !targetFeature.isValid() )
    return -1;

  QgsVectorLayer *layer = targetFeature.layer();
  QgsFeature f = targetFeature.feature();

  // Only points are supported
  if ( layer->geometryType() != Qgis::GeometryType::Point )
    return -1;

  // Transform gps position to map CRS
  QgsPointXY transformedPosition = transformPoint(
                                     coordinateReferenceSystemFromEpsgId( 4326 ),
                                     mapSettings->destinationCrs(),
                                     mapSettings->transformContext(),
                                     gpsPoint
                                   );

  if ( transformedPosition.isEmpty() )
  {
    return -1;
  }

  // Transform target point to map CRS
  QgsPoint target( extractPointFromFeature( targetFeature ) );
  QgsPointXY transformedTarget = transformPoint(
                                   targetFeature.layer()->crs(),
                                   mapSettings->destinationCrs(),
                                   mapSettings->transformContext(),
                                   target
                                 );

  if ( transformedTarget.isEmpty() )
  {
    return -1;
  }

  QgsDistanceArea distanceArea;
  distanceArea.setSourceCrs( mapSettings->destinationCrs(), mapSettings->transformContext() );

  return distanceArea.bearing( transformedPosition, transformedTarget );
}

QString InputUtils::featureTitle( const FeatureLayerPair &pair, QgsProject *project )
{
  if ( !project || !pair.isValid() )
    return QString();

  QString title;

  QgsVectorLayer *layer = pair.layer();

  // can't use QgsExpressionContextUtils::globalProjectLayerScopes() because it uses QgsProject::instance()
  QList<QgsExpressionContextScope *> scopes;
  scopes << QgsExpressionContextUtils::globalScope();
  scopes << QgsExpressionContextUtils::projectScope( project );
  scopes << QgsExpressionContextUtils::layerScope( layer );

  QgsExpressionContext context( scopes );
  context.setFeature( pair.feature() );
  QgsExpression expr( pair.layer()->displayExpression() );
  title = expr.evaluate( &context ).toString();

  if ( title.isEmpty() )
    title = QStringLiteral( "Feature %1" ).arg( pair.feature().id() );

  return title;
}

FeatureLayerPair InputUtils::createFeatureLayerPair(
  QgsVectorLayer *layer,
  const QgsGeometry &geometry,
  VariablesManager *variablesmanager,
  QgsExpressionContextScope *additionalScope )
{
  if ( !layer )
    return FeatureLayerPair();

  QgsAttributes attrs( layer->fields().count() );
  QgsExpressionContext context = layer->createExpressionContext();

  if ( variablesmanager )
    context << variablesmanager->positionScope();

  if ( additionalScope )
    context << additionalScope;

  QgsFeature feat = QgsVectorLayerUtils::createFeature( layer, geometry, attrs.toMap(), &context );
  return FeatureLayerPair( feat, layer );
}

void InputUtils::createEditBuffer( QgsVectorLayer *layer )
{
  if ( layer )
  {
    if ( !layer->editBuffer() )
    {
      layer->startEditing();
    }
  }
}

FeatureLayerPair InputUtils::changeFeaturePairGeometry( FeatureLayerPair featurePair, const QgsGeometry &geometry )
{
  QgsVectorLayer *vlayer = featurePair.layer();
  if ( vlayer )
  {
    InputUtils::createEditBuffer( vlayer );
    QgsGeometry g( geometry );
    vlayer->changeGeometry( featurePair.feature().id(), g );
    vlayer->triggerRepaint();
    QgsFeature f = vlayer->getFeature( featurePair.feature().id() );
    return FeatureLayerPair( f, featurePair.layer() );
  }
  else
  {
    // invalid pair
    return FeatureLayerPair();
  }
}

QgsPointXY InputUtils::extractPointFromFeature( const FeatureLayerPair &feature )
{
  if ( !feature.isValid() || geometryFromLayer( feature.layer() ) != "point" )
    return QgsPointXY();

  QgsFeature f = feature.feature();
  const QgsAbstractGeometry *g = f.geometry().constGet();

  return QgsPoint( dynamic_cast< const QgsPoint * >( g )->toQPointF() );
}

bool InputUtils::isPointLayerFeature( const FeatureLayerPair &feature )
{
  if ( !feature.isValid() || geometryFromLayer( feature.layer() ) != "point" )
    return false;
  const QgsAbstractGeometry *g = feature.feature().geometry().constGet();
  const QgsPoint *point = dynamic_cast< const QgsPoint * >( g );
  return point != nullptr;
}

void InputUtils::zoomToProject( QgsProject *qgsProject, InputMapSettings *mapSettings )
{
  if ( !qgsProject || !mapSettings )
  {
    qDebug() << "Cannot zoom to extent, MapSettings or QgsProject is not defined";
    return;
  }
  QgsRectangle extent;

  QgsProjectViewSettings *viewSettings = qgsProject->viewSettings();
  extent = viewSettings->presetFullExtent();
  if ( extent.isNull() )
  {
    bool hasWMS;
    QStringList WMSExtent = qgsProject->readListEntry( "WMSExtent", QStringLiteral( "/" ), QStringList(), &hasWMS );

    if ( hasWMS && ( WMSExtent.length() == 4 ) )
    {
      extent.set( WMSExtent[0].toDouble(), WMSExtent[1].toDouble(), WMSExtent[2].toDouble(), WMSExtent[3].toDouble() );
    }
    else // set layers extent
    {
      const QVector<QgsMapLayer *> layers = qgsProject->layers<QgsMapLayer *>();
      for ( const QgsMapLayer *layer : layers )
      {
        QgsRectangle layerExtent = mapSettings->mapSettings().layerExtentToOutputExtent( layer, layer->extent() );
        extent.combineExtentWith( layerExtent );
      }
    }
  }

  if ( extent.isEmpty() )
  {
    extent.grow( qgsProject->crs().isGeographic() ? 0.01 : 1000.0 );
  }
  extent.scale( 1.05 );
  mapSettings->setExtent( extent );
}

QUrl InputUtils::loadIconFromLayer( QgsMapLayer *layer )
{
  if ( !layer )
    return QString();

  QgsVectorLayer *vectorLayer = qobject_cast<QgsVectorLayer *>( layer );

  if ( vectorLayer )
  {
    Qgis::GeometryType geometry = vectorLayer->geometryType();
    return iconFromGeometry( geometry );
  }
  else
    return MMStyle::rasterLayerNoColorOverlayIcon();
}

QUrl InputUtils::loadIconFromFeature( QgsFeature feature )
{
  return iconFromGeometry( feature.geometry().type() );
}

QUrl InputUtils::iconFromGeometry( const Qgis::GeometryType &geometry )
{
  switch ( geometry )
  {
    case Qgis::GeometryType::Point: return MMStyle::pointLayerNoColorOverlayIcon();
    case Qgis::GeometryType::Line: return MMStyle::lineLayerNoColorOverlayIcon();
    case Qgis::GeometryType::Polygon: return MMStyle::polygonLayerNoColorOverlayIcon();
    default: return MMStyle::tableLayerNoColorOverlayIcon();
  }
}

bool InputUtils::rescaleImage( const QString &path, QgsProject *activeProject )
{
  int quality = activeProject->readNumEntry( QStringLiteral( "Mergin" ), QStringLiteral( "PhotoQuality" ), 0 );
  return ImageUtils::rescale( path, quality );
}

QgsGeometry InputUtils::createGeometryForLayer( QgsVectorLayer *layer )
{
  QgsGeometry geometry;

  if ( !layer )
  {
    return geometry;
  }

  bool isMulti = QgsWkbTypes::isMultiType( layer->wkbType() );

  switch ( layer->geometryType() )
  {
    case Qgis::GeometryType::Point:
    {
      if ( isMulti )
      {
        QgsMultiPoint *multiPoint = new QgsMultiPoint();
        geometry.set( multiPoint );
      }
      else
      {
        QgsPoint *point = new QgsPoint();
        geometry.set( point );
      }
      break;
    }

    case Qgis::GeometryType::Line:
    {
      if ( isMulti )
      {
        QgsMultiLineString *multiLine = new QgsMultiLineString();
        geometry.set( multiLine );
      }
      else
      {
        QgsLineString *line = new QgsLineString();
        geometry.set( line );
      }
      break;
    }

    case Qgis::GeometryType::Polygon:
    {
      if ( isMulti )
      {
        QgsLineString *line = new QgsLineString();
        QgsPolygon *polygon = new QgsPolygon( line );
        QgsMultiPolygon *multiPolygon = new QgsMultiPolygon();
        multiPolygon->addGeometry( polygon );
        geometry.set( multiPolygon );
      }
      else
      {
        QgsLineString *line = new QgsLineString();
        QgsPolygon *polygon = new QgsPolygon( line );
        geometry.set( polygon );
      }
      break;
    }

    default:
      break;
  }

  if ( QgsWkbTypes::hasZ( layer->wkbType() ) )
  {
    geometry.get()->addZValue( 0 );
  }

  if ( QgsWkbTypes::hasM( layer->wkbType() ) )
  {
    geometry.get()->addMValue( 0 );
  }

  return geometry;
}

QString InputUtils::invalidGeometryWarning( QgsVectorLayer *layer )
{
  QString msg;
  if ( !layer )
  {
    return msg;
  }

  int nPoints = 1;
  if ( layer->geometryType() == Qgis::GeometryType::Line )
  {
    nPoints = 2;
  }
  else if ( layer->geometryType() == Qgis::GeometryType::Polygon )
  {
    nPoints = 3;
  }

  if ( QgsWkbTypes::isMultiType( layer->wkbType() ) )
  {
    return tr( "You need to add at least %1 point(s) to every part." ).arg( nPoints );
  }
  else
  {
    return tr( "You need to add at least %1 point(s)." ).arg( nPoints );
  }
}

void InputUtils::updateFeature( const FeatureLayerPair &pair )
{
  if ( !pair.layer() )
  {
    return;
  }

  if ( !pair.feature().isValid() )
  {
    return;
  }

  if ( !pair.layer()->isEditable() )
  {
    pair.layer()->startEditing();
  }

  QgsFeature f( pair.feature() );
  pair.layer()->updateFeature( f );
  pair.layer()->commitChanges();
  pair.layer()->triggerRepaint();
}

QString InputUtils::imageGalleryLocation()
{
  QStringList galleryPaths = QStandardPaths::standardLocations( QStandardPaths::PicturesLocation );

  if ( galleryPaths.isEmpty() )
  {
    CoreUtils::log( QStringLiteral( "Image Picker" ), QStringLiteral( "Could not find standard path to image gallery" ) );
    return QString();
  }

  return galleryPaths.last();
}

QString InputUtils::layerAttribution( QgsMapLayer *layer )
{
  if ( !layer || !layer->isValid() )
  {
    return QString();
  }

  QStringList rights = layer->metadata().rights();
  if ( !rights.isEmpty() )
  {
    return rights.join( QStringLiteral( ", " ) );
  }

  return QString();
}

const double PROFILER_THRESHOLD = 0.001;
static double qgsRuntimeProfilerExtractModelAsText( QStringList &lines, const QString &group, const QModelIndex &parent, int level )
{
  double total_elapsed = 0.0;

  const int rc = QgsApplication::profiler()->rowCount( parent );
  for ( int r = 0; r < rc; r++ )
  {
    QModelIndex rowIndex = QgsApplication::profiler()->index( r, 0, parent );
    if ( QgsApplication::profiler()->data( rowIndex, QgsRuntimeProfilerNode::Group ).toString() != group )
      continue;
    bool ok;
    double elapsed = QgsApplication::profiler()->data( rowIndex, QgsRuntimeProfilerNode::Elapsed ).toDouble( &ok );
    if ( !ok )
      elapsed = 0.0;
    total_elapsed += elapsed;

    if ( elapsed > PROFILER_THRESHOLD )
    {
      QString name = QgsApplication::profiler()->data( rowIndex, QgsRuntimeProfilerNode::Name ).toString();
      lines << QStringLiteral( "  %1 %2: %3 sec" ).arg( QStringLiteral( ">" ).repeated( level + 1 ),  name, QString::number( elapsed, 'f', 3 ) );
    }
    total_elapsed += qgsRuntimeProfilerExtractModelAsText( lines, group, rowIndex, level + 1 );
  }
  return total_elapsed;
}

QVector<QString> InputUtils::qgisProfilerLog()
{
  QVector<QString> lines;
  const QString project = QgsProject::instance()->fileName();

  if ( !project.isEmpty() )
  {
    lines << QStringLiteral( "QgsProject filename: %1" ).arg( project );
  }

  lines << QStringLiteral( "List of QgsRuntimeProfiler events above %1 sec" ).arg( QString::number( PROFILER_THRESHOLD, 'f', 3 ) );

  const auto groups = QgsApplication::profiler()->groups();
  for ( const QString &g : groups )
  {
    QVector<QString> groupLines;
    double elapsed = qgsRuntimeProfilerExtractModelAsText( groupLines, g, QModelIndex(), 0 );
    if ( elapsed > PROFILER_THRESHOLD )
    {
      lines << QStringLiteral( "  %1: total %2 sec" ).arg( g, QString::number( elapsed, 'f', 3 ) );
      lines << groupLines;
    }
  }
  return lines;
}

QList<QgsPoint> InputUtils::parsePositionUpdates( const QString &data )
{
  QList<QgsPoint> parsedUpdates;
  QStringList positions = data.split( '\n', Qt::SkipEmptyParts );

  if ( positions.isEmpty() )
  {
    return parsedUpdates;
  }

  for ( int ix = 0; ix < positions.size(); ix++ )
  {
    QStringList coordinates = positions[ix].split( ' ', Qt::SkipEmptyParts );

    if ( coordinates.size() != 4 )
    {
      continue;
    }

    QgsPoint geop(
      coordinates[0].toDouble(), // long
      coordinates[1].toDouble(), // lat
      coordinates[2].toDouble(), // alt
      coordinates[3].toDouble(), // UTC time in secs
      Qgis::WkbType::PointZM // explicitly mention the point type
    );

    parsedUpdates << geop;
  }

  return parsedUpdates;
}

QString InputUtils::getManufacturer()
{
#ifdef Q_OS_ANDROID
  return AndroidUtils::getManufacturer();
#elif defined(Q_OS_IOS)
  return IosUtils::getManufacturer();
#endif
  return QStringLiteral( "N/A" );
}

QString InputUtils::getDeviceModel()
{
#ifdef Q_OS_ANDROID
  return AndroidUtils::getDeviceModel();
#elif defined(Q_OS_IOS)
  return IosUtils::getDeviceModel();
#endif
  return QStringLiteral( "N/A" );
}

bool InputUtils::openLink( const QString &homePath, const QString &link )
{
  if ( link.startsWith( LOCAL_FILE_PREFIX ) )
  {
    QString relativePath = link.mid( QString( LOCAL_FILE_PREFIX ).length() );
    QString absoluteLinkPath = homePath + QDir::separator() + relativePath;
    if ( !fileExists( absoluteLinkPath ) )
    {
      return false;
    }
#ifdef Q_OS_ANDROID
    if ( !mAndroidUtils->openFile( absoluteLinkPath ) )
    {
      return false;
    }
#elif defined(Q_OS_IOS)
    if ( ! IosUtils::openFile( absoluteLinkPath ) )
    {
      return false;
    }
#else
    // Desktop environments
    QUrl fileUrl = QUrl::fromLocalFile( absoluteLinkPath );
    if ( !QDesktopServices::openUrl( fileUrl ) )
    {
      return false;
    }
#endif
  }
  else
  {
    QDesktopServices::openUrl( QUrl( link ) );
  }

  return true;
}

double InputUtils::pixelDistanceBetween( const QPointF &p1, const QPointF &p2 )
{
  return std::hypot( p1.x() - p2.x(), p1.y() - p2.y() );
}
