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
#include "qgsvectorlayer.h"
#if VERSION_INT >= 30500
// this header only exists in QGIS >= 3.6
#include "qgsexpressioncontextutils.h"
#endif
#include <QDebug>

const QString Loader::LOADING_FLAG_FILE_PATH = QString( "%1/.input_loading_project" ).arg( QStandardPaths::standardLocations( QStandardPaths::TempLocation ).first() );

Loader::Loader( QObject *parent )
  : QObject( parent )
{
  // we used to have our own QgsProject instance, but unfortunately few pieces of qgis_core
  // still work with QgsProject::instance() singleton hardcoded (e.g. vector layer's feature
  // iterator uses it for virtual fields, causing minor bugs with expressions)
  // so for the time being let's just stick to using the singleton until qgis_core is completely fixed
  mProject = QgsProject::instance();
}

QgsProject *Loader::project()
{
  return mProject;
}

void Loader::setPositionKit( QgsQuickPositionKit *kit )
{
  mPositionKit = kit;
  emit positionKitChanged();
}

void Loader::setRecording( bool isRecordingOn )
{
  if ( mRecording != isRecordingOn )
  {
    mRecording = isRecordingOn;
    emit recordingChanged();
  }
}

bool Loader::load( const QString &filePath )
{
  qDebug() << "Loading " << filePath;
  // Just clear project if empty
  if ( filePath.isEmpty() )
  {
    mProject->clear();
    emit projectReloaded();
    return true;
  }

  emit loadingStarted();
  QFile flagFile( LOADING_FLAG_FILE_PATH );
  flagFile.open( QIODevice::WriteOnly );
  flagFile.close();

  // Give some time to other (GUI) processes before loading a project in the main thread
  QEventLoop loop;
  QTimer t;
  t.connect( &t, &QTimer::timeout, &loop, &QEventLoop::quit );
  t.start( 10 );
  loop.exec();

  bool res = true;
  if ( mProject->fileName() != filePath )
  {
    res = mProject->read( filePath );
    emit projectReloaded();
  }

  flagFile.remove();
  emit loadingFinished();
  return res;
}

bool Loader::reloadProject( QString projectDir )
{
  if ( mProject->homePath() == projectDir )
  {
    return load( mProject->fileName() );
  }
  return false;
}

void Loader::zoomToProject( QgsQuickMapSettings *mapSettings )
{
  if ( !mapSettings )
  {
    qDebug() << "Cannot zoom to layers extent, mapSettings is not defined";
    return;
  }

  const QVector<QgsMapLayer *> layers = mProject->layers<QgsMapLayer *>();
  QgsRectangle extent;
  for ( const QgsMapLayer *layer : layers )
  {
    QgsRectangle layerExtent = mapSettings->mapSettings().layerExtentToOutputExtent( layer, layer->extent() );
    extent.combineExtentWith( layerExtent );
  }
  extent.scale( 1.05 );
  mapSettings->setExtent( extent );
}

QString Loader::featureTitle( QgsQuickFeatureLayerPair pair )
{
  QgsExpressionContext context( globalProjectLayerScopes( pair.layer() ) );
  context.setFeature( pair.feature() );
  QgsExpression expr( pair.layer()->displayExpression() );
  return expr.evaluate( &context ).toString();
}

QString Loader::mapTipHtml( QgsQuickFeatureLayerPair pair )
{
  QgsExpressionContext context( globalProjectLayerScopes( pair.layer() ) );
  context.setFeature( pair.feature() );
  return QgsExpression::replaceExpressionText( pair.layer()->mapTipTemplate(), &context );
}

QString Loader::mapTipType( QgsQuickFeatureLayerPair pair )
{
  // Stripping extra CR char to unify Windows lines with Unix.
  QString mapTip = pair.layer()->mapTipTemplate().replace( QStringLiteral( "\r" ), QStringLiteral( "" ) );
  if ( mapTip.startsWith( "# image\n" ) )
    return "image";
  else if ( mapTip.startsWith( "# fields\n" ) || mapTip.isEmpty() )
    return "fields";
  else
    return "html";
}

QString Loader::mapTipImage( QgsQuickFeatureLayerPair pair )
{
  QgsExpressionContext context( globalProjectLayerScopes( pair.layer() ) );
  context.setFeature( pair.feature() );
  QString mapTip = pair.layer()->mapTipTemplate();
  QStringList lst = mapTip.split( '\n' ); // first line is "# image"
  if ( lst.count() >= 2 )
    return QgsExpression::replaceExpressionText( lst[1], &context );
  else
    return QString();
}

QStringList Loader::mapTipFields( QgsQuickFeatureLayerPair pair )
{
  QString mapTip = pair.layer()->mapTipTemplate();
  QStringList lst;
  const QgsFields fields = pair.layer()->fields();
  const int LIMIT = 3;  // max. 3 fields can fit in the preview

  if ( mapTip.isEmpty() )
  {
    // user has not provided any map tip - let's use first two fields to show
    // at least something.
    QString featureTitleExpression = pair.layer()->displayExpression();
    for ( QgsField field : fields )
    {
      if ( featureTitleExpression != field.name() )
        lst << field.displayName();  // yes, using alias, not the original field name
      if ( lst.count() == LIMIT )
        break;
    }
  }
  else
  {
    // user has specified "# fields" on the first line and then each next line is a field name
    QStringList lines = mapTip.split( '\n' );
    for ( int i = 1; i < lines.count(); ++i ) // starting from index to avoid first line with "# fields"
    {
      int index = fields.indexFromName( lines[i] );
      if ( index >= 0 )
        lst << fields[index].displayName();  // yes, using alias, not the original field name
      if ( lst.count() == LIMIT )
        break;
    }
  }
  return lst;
}

void Loader::appStateChanged( Qt::ApplicationState state )
{
#if VERSION_INT >= 30500 // depends on https://github.com/qgis/QGIS/pull/8622
  if ( !mRecording && mPositionKit )
  {
    if ( state == Qt::ApplicationActive )
    {
      mPositionKit->source()->startUpdates();
    }
    else
    {
      mPositionKit->source()->stopUpdates();
    }
  }
#else
  Q_UNUSED( state );
#endif
}

QList<QgsExpressionContextScope *> Loader::globalProjectLayerScopes( QgsMapLayer *layer )
{
  // can't use QgsExpressionContextUtils::globalProjectLayerScopes() because it uses QgsProject::instance()
  QList<QgsExpressionContextScope *> scopes;
  scopes << QgsExpressionContextUtils::globalScope();
  scopes << QgsExpressionContextUtils::projectScope( mProject );
  scopes << QgsExpressionContextUtils::layerScope( layer );
  return scopes;
}
