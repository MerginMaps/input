/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef INPUTUTILS_H
#define INPUTUTILS_H

#define STR1(x)  #x
#define STR(x)  STR1(x)

#include <QObject>
#include <QtGlobal>
#include <QUuid>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtPositioning/QGeoCoordinate>
#include <QModelIndex>

#include <limits>

#include "inputconfig.h"
#include "inputhelp.h"
#include "merginapi.h"
#include "androidutils.h"
#include "featurelayerpair.h"
#include "inputmapsettings.h"
#include "qgis.h"
#include "qgsexpressioncontextutils.h"
#include "qgsmessagelog.h"
#include "qgspoint.h"
#include "qgspointxy.h"
#include "qgsunittypes.h"
#include "inputmapsettings.h"
#include "featurelayerpair.h"
#include "qgscoordinateformatter.h"
#include "position/mapposition.h"

class QgsFeature;
class QgsVectorLayer;
class QgsCoordinateReferenceSystem;

class VariablesManager;

class InputUtils: public QObject
{
    Q_OBJECT
  public:
    explicit InputUtils( QObject *parent = nullptr );
    explicit InputUtils( AndroidUtils *au, QObject *parent = nullptr );
    ~InputUtils() override = default;

    /**
     * Copies file from srcPath to dstPath. If destination directory is
     * not exists — tries to create it first.
     * \param srcPath Absolute path to a source file.
     * \param dstPath Absolute path to a destination file.
     * \result True on success, False otherwise.
     */
    Q_INVOKABLE static bool copyFile( const QString &srcPath, const QString &dstPath );

    /**
     * Creates the directory path. It will also create all parent
     * directories necessary to create the directory.
     * \param path directory path to create.
     * \result True on success, False otherwise.
     */
    Q_INVOKABLE static bool createDirectory( const QString &path );

    Q_INVOKABLE QString getFileName( const QString &filePath );
    Q_INVOKABLE QString formatProjectName( const QString &fullProjectName );
    Q_INVOKABLE QString formatNumber( const double number, int precision = 1 );
    Q_INVOKABLE QString formatDistanceInProjectUnit( const double distanceInMeters, int precision, QgsProject *project );
    Q_INVOKABLE QString formatAreaInProjectUnit( const double areaInSquareMeters, int precision, QgsProject *project );

    static QString formatDistanceHelper( const double distanceInMeters, int precision, Qgis::DistanceUnit destUnit );
    static QString formatAreaHelper( const double areaInSquareMeters, int precision, Qgis::AreaUnit destUnit );

    Q_INVOKABLE void setExtentToFeature( const FeatureLayerPair &pair, InputMapSettings *mapSettings );

    /**
     * Returns the screen coordinates for a geometry's bounding box centroid
     * Geometry must be in canvas CRS
     */
    Q_INVOKABLE QPointF geometryCenterToScreenCoordinates( const QgsGeometry &geom, InputMapSettings *mapSettings );


    /**
     * Returns the true if the geometry could fully be contained in the current screen otherwise false
     * Geometry must be in canvas CRS
     */
    Q_INVOKABLE bool canExtentContainGeometry( const QgsGeometry &geom, InputMapSettings *mapSettings );

    // utility functions to extract information from map settings
    // (in theory this data should be directly available from .MapTransform
    // but they are not currently, so this is a workaround we need for display of markers)

    Q_INVOKABLE double mapSettingsScale( InputMapSettings *ms );
    Q_INVOKABLE double mapSettingsOffsetX( InputMapSettings *ms );
    Q_INVOKABLE double mapSettingsOffsetY( InputMapSettings *ms );
    Q_INVOKABLE double mapSettingsDPR( InputMapSettings *ms );

    //! Converts geometry to destination CRS if not already
    static QgsGeometry transformGeometry( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &sourceCRS, QgsVectorLayer *targetLayer );
    static QgsGeometry transformGeometry( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &sourceCRS, const QgsCoordinateReferenceSystem &destinationCRS, const QgsCoordinateTransformContext &context );

    //! Helper methods to use for transforming geometry from QML as overriding does not work properly there
    Q_INVOKABLE static QgsGeometry transformGeometryToMapWithLayer( const QgsGeometry &geometry, QgsVectorLayer *sourceLayer, InputMapSettings *targetSettings );
    Q_INVOKABLE static QgsGeometry transformGeometryToMapWithCRS( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &sourceCRS, InputMapSettings *targetSettings );

    /**
     * Function extracts QgsGeometry from the given pair.
     * If layer's CRS does not match canvas CRS, geometry is transformed to canvas CRS.
     */
    Q_INVOKABLE static QgsGeometry extractGeometry( const FeatureLayerPair &pair );

    Q_INVOKABLE static QString geometryLengthAsString( const QgsGeometry &geometry );

    /**
     * Extract geometry coordinates from the given geometry.
     *
     * The output can encode also multi-part geometries or even geometry collections.
     * We pass a single array out of the function, so this is the encoding of coordinates:
     * - point:  <value 0> <x> <y>
     * - linestring: <value 1> <number of points> <x1> <y1> ... <xn> <yn>
     * - polygon: <value 2> <number of points> <x1> <y1> ... <xn> <yn>
     *
     * The output is a chain of sub-geometries. Polygon's holes (interior rings) are treated just
     * like exterior ring because when we create a single path, Qt automatically detects which rings
     * are holes by using even-odd fill rule.
     *
     * Geometry passed to this function must have the same CRS as mapsettings' canvas CRS.
     */
    Q_INVOKABLE static QVector<double> extractGeometryCoordinates( const QgsGeometry &geometry );

    /**
     * Renames a file located at a given path with a dateTime. Tend to be use to avoid name conflicts.
     * \param srcPath Absolute path to a file.
     * \param dateTime new name of a file. If is invalid, current dateTime is used.
     * \result Either absolute path of a rename file or empty string.
     */
    Q_INVOKABLE static QString renameWithDateTime( const QString &srcPath, const QDateTime &dateTime = QDateTime() );

    /**
     * Renames a file located at a given path to a given new name
     * \param srcPath absolute path to a file.
     * \param dstPath a new name of a file.
     * \result true on success, false otherwise.
     */
    Q_INVOKABLE static bool renameFile( const QString &srcPath, const QString &dstPath );

    /**
     * Convinient way how to create text with html <a> tag for QML
     *
     * htmlLink("Learn more", __style.forestGreen, "www.merginmaps.com")
     *
     * You can use also QString formatting to include text before and after link, e.g.
     * htmlLink("my text %1link%2", __style.forestGreen, "www.merginmaps.com")
     *
     * You can use also use 2 links, e.g.
     * htmlLink("my text %1link_1%3 and %2link_2%3", __style.forestGreen, "www.merginmaps.com", false, true "www.merginmaps.com/login")
     *
     * \param text Text to insert html <a> tag
     * \param color of the link
     * \param url url of the first link
     * \param url2 url of the second link; empty string means only single link is required
     * \param underline whether the link is underlined
     * \param bold whether the link is bold
     * \result text with inserted html <a> link
     */
    Q_INVOKABLE static QString htmlLink( const QString &text,
                                         const QColor &color,
                                         const QString &url = "internal-signal",
                                         const QString &url2 = "",
                                         bool underline = true,
                                         bool bold = true
                                       );

    /**
     * Converts bytes to human readable size (e.g. 1GB, 500MB)
     */
    Q_INVOKABLE static QString bytesToHumanSize( double bytes );

    /**
     * Opens the specified link in an appropriate application. For "project://" links, it converts them to
     * absolute paths and opens with default file handlers. Other links are opened in the default web browser.
     * @param link The link to open, either a "project://" link or a standard URL.
     */
    Q_INVOKABLE bool openLink( const QString &homePath, const QString &link );

    Q_INVOKABLE bool acquireCameraPermission();

    Q_INVOKABLE bool isBluetoothTurnedOn();

    Q_INVOKABLE void turnBluetoothOn();

    Q_INVOKABLE void quitApp();

    Q_INVOKABLE static QString getManufacturer();

    Q_INVOKABLE static QString getDeviceModel();

    /**
     * Method copies all entries from given source path to destination path. If cannot copy a file for the first time,
     * removes it and tries again (overwrite a file). If failes again, skips the file, sets result to false and continue.
     * \param srcPath Source path
     * \param dstPath Destination path
     * \result True if operation was fully successful otherwise false.
     */
    static bool cpDir( const QString &srcPath, const QString &dstPath, bool onlyDiffable = false );

    static QString filesToString( QList<MerginFile> files );

    /** InputApp platform */
    static QString appPlatform();
    static bool isMobilePlatform();

    static QString appDataDir();

    /**
     * Converts string in rational number format to double.
     * @param rationalValue String - expecting value in format "numerator/denominator" (e.g "123/100").
     * @return -1 if format is invalid, otherwise double value of given string.
     */
    static double convertRationalNumber( const QString &rationalValue );

    /**
     * Converts string coordinate in EXIF rational number format (rational64u[3] - degrees,minutes,seconds) to double.
     * @param rationalValue String - expecting value in format "12/1,3/1,456789/10000".
     * @return 0 if format is invalid, otherwise double for given string coordinate.
     */
    static double convertCoordinateString( const QString &rationalValue );

    /** Formats coordinates from point into degrees format with minutes, seconds and hemisphere */
    Q_INVOKABLE static QString degreesString( const QgsPoint &point );

    /**
     * @brief Creates formatted string of difference for given tMin and tMax datetimes (in minutes, hours, ... ago).
     * Note, that tMin < tMax, otherwise arguments are invalid.
     * @param tMin Datetime in the past
     * @param tMax Datetime after tMin, currentDateTime by default.
     * @return Formatted string
     */
    static QString formatDateTimeDiff( const QDateTime &tMin, const QDateTime &tMax = QDateTime::currentDateTimeUtc() );

    /**
      * Creates crs from epsg code in QML
      */
    Q_INVOKABLE static QgsCoordinateReferenceSystem coordinateReferenceSystemFromEpsgId( long epsg );

    /**
      * Creates QgsPointXY in QML
      */
    Q_INVOKABLE static QgsPointXY pointXY( double x, double y );

    /**
      * Creates QgsPoint in QML
      */
    Q_INVOKABLE static QgsPoint point( double x, double y, double z = std::numeric_limits<double>::quiet_NaN(), double m = std::numeric_limits<double>::quiet_NaN() );

    /**
     * Creates empty geometry
     */
    Q_INVOKABLE static QgsGeometry emptyGeometry();

    /**
     * Creates empty feature
     */
    Q_INVOKABLE static QgsFeature emptyFeature();

    /**
     * Returns true for null or empty geometry
     */
    Q_INVOKABLE static bool isEmptyGeometry( const QgsGeometry &geometry );

    /**
      * Converts QGeoCoordinate to QgsPoint
      */
    Q_INVOKABLE static QgsPoint coordinateToPoint( const QGeoCoordinate &coor );

    /**
      * Transforms point between different crs
      * Return empty QgsPointXY if the transformation could not be applied or srcPoint is empty
      */
    Q_INVOKABLE static QgsPointXY transformPointXY( const QgsCoordinateReferenceSystem &srcCrs,
        const QgsCoordinateReferenceSystem &destCrs,
        const QgsCoordinateTransformContext &context,
        const QgsPointXY &srcPoint );

    /**
      * Transforms point between different crs
      * Return empty QgsPoint if the transformation could not be applied or srcPoint is empty
      */
    Q_INVOKABLE static QgsPoint transformPoint( const QgsCoordinateReferenceSystem &srcCrs,
        const QgsCoordinateReferenceSystem &destCrs,
        const QgsCoordinateTransformContext &context,
        const QgsPoint &srcPoint );

    /**
      * Transforms point between CRS and screen pixels
      * Return empty QgsPoint if the transformation could not be applied or srcPoint is empty
      */
    Q_INVOKABLE static QPointF transformPointToScreenCoordinates(
      const QgsCoordinateReferenceSystem &srcCrs,
      InputMapSettings *mapSettings,
      const QgsPoint &srcPoint );

    /**
      * Calculates the distance in meter representing baseLengthPixels pixels on the screen based on the current map settings.
      */
    Q_INVOKABLE static double screenUnitsToMeters( InputMapSettings *mapSettings, int baseLengthPixels );

    // Converts map coordinate in map's X/Y to GPS coordinate
    Q_INVOKABLE static QgsPoint mapPointToGps( QPointF mapPosition, InputMapSettings *mapSettings );

    /**
      * Returns whether file on path exists
      */
    Q_INVOKABLE static bool fileExists( const QString &path );

    /**
     * Returns working path to load the image from QML.
     * @param path Path of an image - either relative or absolute
     * @param homePath Project path
     * @param config Field widget's config
     * @param pair FeatureLayerPair - needed for expression evaluation
     * @param activeProject QgsProject - needed for expression evaluation
     * @return Path to the image
     */
    Q_INVOKABLE static QString resolvePath( const QString &path, const QString &homePath, const QVariantMap &config, const FeatureLayerPair &pair, QgsProject *activeProject );

    /**
     * This evaluates the "default path" with the following order:
     * 1. evaluate default path expression if defined,
     * 2. use default path value if not empty,
     * 3. use project home folder
     */
    Q_INVOKABLE static QString resolveTargetDir( const QString &homePath, const QVariantMap &config, const FeatureLayerPair &pair, QgsProject *activeProject );

    /**
     * Function used for resolving path of an image for a field with ExternalResource widget type.
     * Returns prefix which has to be added to the field's value to obtain working path to load the image from QML.
     * @param relativeStorageMode: 0 - Relative path disabled; 1 - Relative path to project;
     * 2 - Relative path to defaultRoot defined in the config - Default path field in the widget configuration form
     * @param homePath Project path
     * @param targetDir Default path in the widget configuration
     * @return Returns either homePath, targetDir or empty QString according relativeStorageMode that is configurable in field widget's config
     */
    Q_INVOKABLE static QString resolvePrefixForRelativePath( int relativeStorageMode, const QString &homePath, const QString &targetDir );

    /**
     * Returns absolute path of the file for given path and its prefix. If prefixPath is empty,
     * returns given path.
     * \param path (Relative) path to file,
     * \param prefixPath Empty or prefix for given path to abtain absolute path.
     */
    Q_INVOKABLE static QString getAbsolutePath( const QString &path, const QString &prefixPath );

    /**
     * Returns relative path of the file to given prefixPath. If prefixPath does not match a path parameter,
     * returns an empty string. If a path starts with "file://", this prefix is ignored.
     * \param path Absolute path to file
     * \param prefixPath
     */
    Q_INVOKABLE static QString getRelativePath( const QString &path, const QString &prefixPath );

    /**
      * Log message in QgsMessageLog
      */
    Q_INVOKABLE static void logMessage( const QString &message,
                                        const QString &tag = QString( "" ),
                                        Qgis::MessageLevel level = Qgis::Warning );

    /**
     * Logs message via CoreUtils to diagnostic log
     */
    Q_INVOKABLE static void log( const QString &context, const QString &message );

    /**
      * Returns QUrl to image from library's /images folder.
      */
    Q_INVOKABLE static const QUrl getThemeIcon( const QString &name );

    //! Returns icon url for layer based on its geometry
    Q_INVOKABLE static QUrl loadIconFromLayer( QgsMapLayer *layer );

    //! Returns icon url for feature based on its geometry
    Q_INVOKABLE static QUrl loadIconFromFeature( QgsFeature feature );

    /**
      * Returns url to field editor component for a feature form.
      * If the widgetName does not match any supported widget, text edit is returned.
      * \param widgetName name of the attribute field widget
      * \param config map coming from QGIS describing this field
      * \param field qgsfield instance of this field
      */
    Q_INVOKABLE static const QUrl getFormEditorType( const QString &widgetNameIn, const QVariantMap &config = QVariantMap(), const QgsField &field = QgsField(), const QgsRelation &relation = QgsRelation(), const QString &editorTitle = QString() );

    /**
     * \copydoc QgsCoordinateFormatter::format()
     */
    Q_INVOKABLE static QString formatPoint(
      const QgsPoint &point,
      QgsCoordinateFormatter::Format format = QgsCoordinateFormatter::FormatPair,
      int decimals = 3,
      QgsCoordinateFormatter::FormatFlags flags = QgsCoordinateFormatter::FlagDegreesUseStringSuffix );

    /**
      * Converts distance to human readable distance
      *
      * This is useful for scalebar texts or output of the GPS accuracy
      *
      * The resulting units are determined automatically,
      * based on requested system of measurement.
      * e.g. 1222.234 m is converted to 1.2 km
      *
      * \param distance distance in units
      * \param units units of dist
      * \param decimals decimal to use
      * \param destSystem system of measurement of the result
      * \returns string represetation of dist in desired destSystem. For distance less than 0, 0 is returned.
      */
    Q_INVOKABLE static QString formatDistance( double distance,
        Qgis::DistanceUnit units,
        int decimals,
        Qgis::SystemOfMeasurement destSystem = Qgis::SystemOfMeasurement::Metric );

    /**
      * Deletes file from a given path.
      *
      * \param filePath Absolute path to file
      * \returns bool TRUE, if removal was successful, otherwise FALSE.
      */
    Q_INVOKABLE static bool removeFile( const QString &filePath );

    /**
      * Converts distance to human readable distance in destination system of measurement
      *
      * \sa Utils::formatDistance()
      *
      * \param srcDistance distance in units
      * \param srcUnits units of dist
      * \param destSystem system of measurement of the result
      * \param destDistance output: distance if desired system of measurement
      * \param destUnits output: unit of destDistance
      */
    static void humanReadableDistance( double srcDistance,
                                       Qgis::DistanceUnit srcUnits,
                                       Qgis::SystemOfMeasurement destSystem,
                                       double &destDistance,
                                       Qgis::DistanceUnit &destUnits );

    //! Returns a string with information about screen size and resolution - useful for debugging
    QString dumpScreenInfo() const;

    /**
     * Evaluates expression.
     * \param pair Used to define a context scope.
     * \param activeProject Used to define a context scope.
     * \param expression
     * \return Evaluated expression
     */
    Q_INVOKABLE static QString evaluateExpression( const FeatureLayerPair &pair, QgsProject *activeProject, const QString &expression );

    /**
    * Returns the QVariant typeName of a \a field.
    * This is a stable identifier (compared to the provider field name).
    * \param field QgsField
    */
    Q_INVOKABLE static QString fieldType( const QgsField &field );

    /**
    * Returns field format's name for given string representing field format defined in QgsDateTimeFieldFormatter.
    * \param fieldFormat string representing formats from QgsDateTimeFieldFormatter.
    */
    Q_INVOKABLE static QString dateTimeFieldFormat( const QString &fieldFormat );

    /**
     * Returns if provided Id is valid ( >= 0 )
     */
    Q_INVOKABLE static bool isFeatureIdValid( qint64 featureId );

    /**
     * Returns widget setup according the field type - supports only basic types.
     * Note that external widget cannot be guessed from type since its the very same as text.
     * @param field QgsField
     * @return QgsEditorWidgetSetup for given field.
     */
    static const QgsEditorWidgetSetup getEditorWidgetSetup( const QgsField &field );
    static const QgsEditorWidgetSetup getEditorWidgetSetup( const QgsField &field, const QString &widgetType, const QVariantMap &additionalArgs = QVariantMap() );

    // Returns geometry type represented as string (point/linestring/polygon/nullGeo); returns empty string if geometry is unknown or layer is invalid.
    Q_INVOKABLE static QString geometryFromLayer( QgsVectorLayer *layer );
    Q_INVOKABLE static bool isPointLayer( QgsVectorLayer *layer );
    Q_INVOKABLE static bool isLineLayer( QgsVectorLayer *layer );
    Q_INVOKABLE static bool isPolygonLayer( QgsVectorLayer *layer );
    Q_INVOKABLE static bool isNoGeometryLayer( QgsVectorLayer *layer );
    Q_INVOKABLE static bool isMultiPartLayer( QgsVectorLayer *layer );
    Q_INVOKABLE static bool isSpatialLayer( QgsVectorLayer *layer );

    // Returns a point geometry from point feature
    Q_INVOKABLE static QgsPointXY extractPointFromFeature( const FeatureLayerPair &feature );

    // Returns an extent for stakeout based on distance between gps position and target feature
    Q_INVOKABLE QgsRectangle stakeoutPathExtent( MapPosition *mapPosition, const FeatureLayerPair &targetFeature, InputMapSettings *mapSettings, double mapExtentOffset );

    //! Returns geometry created out of the two points and converts it to map canvas screen pixels.
    Q_INVOKABLE static QgsGeometry stakeoutGeometry( const QgsPoint &mapPosition, const FeatureLayerPair &target, InputMapSettings *mapSettings );

    // Translates distance to target point into scale factor that should be used for map canvas during stakeout
    qreal distanceToScale( qreal distance );

    // Returns the distance from \a gpsPos to the feature \a pair
    Q_INVOKABLE qreal distanceBetweenGpsAndFeature( QgsPoint gpsPosition, const FeatureLayerPair &targetFeature, InputMapSettings *mapSettings );

    // Returns an angle between current gps position and feature
    Q_INVOKABLE qreal angleBetweenGpsAndFeature( QgsPoint gpsPosition, const FeatureLayerPair &targetFeature, InputMapSettings *mapSettings );

    // Returns the title of the feature
    Q_INVOKABLE static QString featureTitle( const FeatureLayerPair &pair, QgsProject *project );

    //! Creates featureLayerPair from geometry and layer, evaluates its expressions and returns it.
    Q_INVOKABLE static FeatureLayerPair createFeatureLayerPair( QgsVectorLayer *layer = nullptr, const QgsGeometry &geometry = QgsGeometry(), VariablesManager *variablesmanager = nullptr, QgsExpressionContextScope *additionalScope = nullptr );

    Q_INVOKABLE static void createEditBuffer( QgsVectorLayer *layer );

    /**
     *  Changes featureLayerPair's geometry to passed geometry
     *  Geometry must be in the same CRS as the layer
     *  Change is written directly inside the layer edit buffer, but the buffer is not commited!
     */
    Q_INVOKABLE static FeatureLayerPair changeFeaturePairGeometry( FeatureLayerPair featurePair, const QgsGeometry &geometry );

    // Calculates real screen DPR based on DPI
    static qreal calculateScreenDpr();

    // Calculates ratio between real DPR calculated by us with DPR calculated by QT that is later used in qml sizing
    static qreal calculateDpRatio();

    // Compares two variables and returns true if they are equal. For floating point values it uses provided epsilon for comparison
    static bool equals( const QPointF &a, const QPointF &b, double epsilon = 0.001 );
    // Convinient comparison function with possibility to pass custom epsilon value (not possible in QGIS API)
    static bool equals( const QgsPointXY &a, const QgsPointXY &b, double epsilon = 0.001 );
    static bool equals( const QgsPoint &a, const QgsPoint &b, double epsilon = 0.001 );

    // Returns whether geometry of the feature is an actual Point feature (Used because some )
    Q_INVOKABLE static bool isPointLayerFeature( const FeatureLayerPair &pair );

    /**
     *  Changes visible extent of mapsettings based on settings in QGIS project.
     */
    Q_INVOKABLE void zoomToProject( QgsProject *qgsProject, InputMapSettings *mapSettings );

    /**
     * Rescales image according to the project photo quality setting.
     */
    Q_INVOKABLE static bool rescaleImage( const QString &path, QgsProject *activeProject );

    /**
     * Creates an empty geometry matching layer WKB type
     */
    Q_INVOKABLE static QgsGeometry createGeometryForLayer( QgsVectorLayer *layer );

    Q_INVOKABLE static QString invalidGeometryWarning( QgsVectorLayer *layer );

    /**
     * Updates existing layer feature with new feature data.
     */
    Q_INVOKABLE static void updateFeature( const FeatureLayerPair &pair );

    // Returns default path to images in this system
    Q_INVOKABLE static QString imageGalleryLocation();

    /**
     * Parses position updates in format "<long> <lat> <alt> <time_in_secs>\n<long>..." into list of QgsPoints
     */
    static QList<QgsPoint> parsePositionUpdates( const QString &data );

    /**
     * Returns string containing attribution information for a given layer
     */
    Q_INVOKABLE static QString layerAttribution( QgsMapLayer *layer );

    /**
     * Returns QGIS profiler data
     */
    static QVector<QString> qgisProfilerLog();

    /**
     * Calculates the Euclidean distance between two pixel points
     */
    static double pixelDistanceBetween( const QPointF &p1, const QPointF &p2 );

  public slots:
    void onQgsLogMessageReceived( const QString &message, const QString &tag, Qgis::MessageLevel level );

  private:

    static void formatToMetricDistance( double srcDistance,
                                        Qgis::DistanceUnit srcUnits,
                                        double &destDistance,
                                        Qgis::DistanceUnit &destUnits );

    static void formatToImperialDistance( double srcDistance,
                                          Qgis::DistanceUnit srcUnits,
                                          double &destDistance,
                                          Qgis::DistanceUnit &destUnits );

    static void formatToUSCSDistance( double srcDistance,
                                      Qgis::DistanceUnit srcUnits,
                                      double &destDistance,
                                      Qgis::DistanceUnit &destUnits );

    // on iOS the names from gallery pickers are like
    // file:assets-library://asset/asset.PNG%3Fid=A53AB989-6354-433A-9CB9-958179B7C14D&ext=PNG
    // we need to change it to something more readable
    static QString sanitizeName( const QString &path );

    static double ratherZeroThanNaN( double d );

    //! Returns icon url from QgsWkbType geometry
    static QUrl iconFromGeometry( const Qgis::GeometryType &geometry );

    AndroidUtils *mAndroidUtils = nullptr; // not owned

    const QString LOCAL_FILE_PREFIX = QStringLiteral( "project://" );
};

#endif // INPUTUTILS_H
