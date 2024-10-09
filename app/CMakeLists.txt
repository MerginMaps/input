# ########################################################################################
# SOURCES of Mergin Maps Target
# ########################################################################################

set(MM_SRCS
    attributes/attributecontroller.cpp
    attributes/attributedata.cpp
    attributes/attributeformmodel.cpp
    attributes/attributeformproxymodel.cpp
    attributes/attributepreviewcontroller.cpp
    attributes/attributetabmodel.cpp
    attributes/attributetabproxymodel.cpp
    attributes/fieldvalidator.cpp
    attributes/rememberattributescontroller.cpp
    layer/layerdetaildata.cpp
    layer/layerdetaillegendimageprovider.cpp
    layer/layertreeflatmodel.cpp
    layer/layertreeflatmodelpixmapprovider.cpp
    layer/layertreeflatsortfiltermodel.cpp
    layer/layertreemodel.cpp
    layer/layertreemodelpixmapprovider.cpp
    layer/layertreesortfiltermodel.cpp
    map/inputcoordinatetransformer.cpp
    map/inputmapcanvasmap.cpp
    map/inputmapsettings.cpp
    map/inputmaptransform.cpp
    maptools/abstractmaptool.cpp
    maptools/recordingmaptool.cpp
    maptools/splittingmaptool.cpp
    ios/iosimagepicker.cpp
    ios/iosutils.cpp
    position/providers/abstractpositionprovider.cpp
    position/providers/internalpositionprovider.cpp
    position/providers/positionprovidersmodel.cpp
    position/providers/simulatedpositionprovider.cpp
    position/tracking/abstracttrackingbackend.cpp
    position/tracking/internaltrackingbackend.cpp
    position/tracking/positiontrackinghighlight.cpp
    position/tracking/positiontrackingmanager.cpp
    position/geoposition.cpp
    position/mapposition.cpp
    position/positiondirection.cpp
    position/positionkit.cpp
    activelayer.cpp
    activeproject.cpp
    androidutils.cpp
    appsettings.cpp
    autosynccontroller.cpp
    bluetoothdiscoverymodel.cpp
    qrcodedecoder.cpp
    changelogmodel.cpp
    compass.cpp
    featurelayerpair.cpp
    featuresmodel.cpp
    featuresproxymodel.cpp
    fieldsmodel.cpp
    guidelinecontroller.cpp
    identifykit.cpp
    imageutils.cpp
    inputexpressionfunctions.cpp
    inputhelp.cpp
    inputprojutils.cpp
    inpututils.cpp
    inpututils_p.cpp
    invitationsmodel.cpp
    invitationsproxymodel.cpp
    layersmodel.cpp
    layersproxymodel.cpp
    main.cpp
    mapthemesmodel.cpp
    notificationmodel.cpp
    projectsmodel.cpp
    projectsproxymodel.cpp
    projectwizard.cpp
    relationfeaturesmodel.cpp
    relationreferencefeaturesmodel.cpp
    scalebarkit.cpp
    snaputils.cpp
    streamingintervaltype.cpp
    synchronizationerror.cpp
    synchronizationmanager.cpp
    valuerelationfeaturesmodel.cpp
    variablesmanager.cpp
    workspacesmodel.cpp
    workspacesproxymodel.cpp
)

set(MM_HDRS
    attributes/attributecontroller.h
    attributes/attributedata.h
    attributes/attributeformmodel.h
    attributes/attributeformproxymodel.h
    attributes/attributepreviewcontroller.h
    attributes/attributetabmodel.h
    attributes/attributetabproxymodel.h
    attributes/fieldvalidator.h
    attributes/rememberattributescontroller.h
    layer/layerdetaildata.h
    layer/layerdetaillegendimageprovider.h
    layer/layertreeflatmodel.h
    layer/layertreeflatmodelpixmapprovider.h
    layer/layertreeflatsortfiltermodel.h
    layer/layertreemodel.h
    layer/layertreemodelpixmapprovider.h
    layer/layertreesortfiltermodel.h
    map/inputcoordinatetransformer.h
    map/inputmapcanvasmap.h
    map/inputmapsettings.h
    map/inputmaptransform.h
    maptools/abstractmaptool.h
    maptools/recordingmaptool.h
    maptools/splittingmaptool.h
    ios/iosimagepicker.h
    ios/iosutils.h
    position/providers/abstractpositionprovider.h
    position/providers/internalpositionprovider.h
    position/providers/positionprovidersmodel.h
    position/providers/simulatedpositionprovider.h
    position/tracking/abstracttrackingbackend.h
    position/tracking/internaltrackingbackend.h
    position/tracking/positiontrackinghighlight.h
    position/tracking/positiontrackingmanager.h
    position/geoposition.h
    position/mapposition.h
    position/positiondirection.h
    position/positionkit.h
    activelayer.h
    activeproject.h
    androidutils.h
    appsettings.h
    autosynccontroller.h
    bluetoothdiscoverymodel.h
    qrcodedecoder.h
    changelogmodel.h
    compass.h
    enumhelper.h
    featurelayerpair.h
    featuresmodel.h
    featuresproxymodel.h
    fieldsmodel.h
    guidelinecontroller.h
    identifykit.h
    imageutils.h
    inputexpressionfunctions.h
    inputhelp.h
    inputprojutils.h
    inpututils.h
    inpututils_p.h
    invitationsmodel.h
    invitationsproxymodel.h
    layersmodel.h
    layersproxymodel.h
    mapthemesmodel.h
    notificationmodel.h
    projectsmodel.h
    projectsproxymodel.h
    projectwizard.h
    relationfeaturesmodel.h
    relationreferencefeaturesmodel.h
    scalebarkit.h
    snaputils.h
    streamingintervaltype.h
    synchronizationerror.h
    synchronizationmanager.h
    synchronizationoptions.h
    valuerelationfeaturesmodel.h
    variablesmanager.h
    workspacesmodel.h
    workspacesproxymodel.h
    mmstyle.h
)

if (HAVE_BLUETOOTH)
  set(MM_SRCS ${MM_SRCS} position/providers/bluetoothpositionprovider.cpp)

  set(MM_HDRS ${MM_HDRS} position/providers/bluetoothpositionprovider.h)
endif ()

if (ENABLE_TESTS)
  set(MM_SRCS
      ${MM_SRCS}
      test/inputtests.cpp
      test/testattributecontroller.cpp
      test/testattributepreviewcontroller.cpp
      test/testcoreutils.cpp
      test/testformeditors.cpp
      test/testidentifykit.cpp
      test/testimageutils.cpp
      test/testlayertree.cpp
      test/testlinks.cpp
      test/testmaptools.cpp
      test/testmerginapi.cpp
      test/testmodels.cpp
      test/testposition.cpp
      test/testrememberattributescontroller.cpp
      test/testscalebarkit.cpp
      test/testutils.cpp
      test/testutilsfunctions.cpp
      test/testvariablesmanager.cpp
      test/testactiveproject.cpp
      test/testprojectchecksumcache.cpp
  )

  set(MM_HDRS
      ${MM_HDRS}
      test/inputtests.h
      test/testattributecontroller.h
      test/testattributepreviewcontroller.h
      test/testcoreutils.h
      test/testformeditors.h
      test/testidentifykit.h
      test/testimageutils.h
      test/testlayertree.h
      test/testlinks.h
      test/testmaptools.h
      test/testmerginapi.h
      test/testmodels.h
      test/testposition.h
      test/testrememberattributescontroller.h
      test/testscalebarkit.h
      test/testutils.h
      test/testutilsfunctions.h
      test/testvariablesmanager.h
      test/testactiveproject.h
      test/testprojectchecksumcache.h
  )

  if (NOT USE_MM_SERVER_API_KEY)
    set_property(
      SOURCE test/testmerginapi.cpp PROPERTY COMPILE_DEFINITIONS USE_MERGIN_DUMMY_API_KEY
    )
  endif ()
endif ()

if (IOS)
  set(MM_HDRS
      ${MM_HDRS}
      ios/iosinterface.h
      ios/iosviewdelegate.h
      position/tracking/iostrackingbackend.h
      position/tracking/iostrackingbackendimpl.h
  )

  set(MM_SRCS
      ${MM_SRCS}
      ios/iosinterface.mm
      ios/iosviewdelegate.mm
      ios/iosimagepicker.mm
      ios/iosutils.mm
      position/tracking/iostrackingbackend.cpp
      position/tracking/iostrackingbackend.mm
      position/tracking/iostrackingbackendimpl.mm
  )
endif ()

if (ANDROID)
  set(MM_HDRS ${MM_HDRS} position/tracking/androidtrackingbackend.h
              position/providers/androidpositionprovider.h
  )

  set(MM_SRCS ${MM_SRCS} position/tracking/androidtrackingbackend.cpp
              position/providers/androidpositionprovider.cpp
  )
endif ()

qt_add_resources(
  MM_SRCS
  icons/icons.qrc
  images/qgis/qgisicons.qrc
  images/images.qrc
  i18n/input_i18n.qrc
  fonts/fonts.qrc
)

# ########################################################################################
# Mergin Maps Target
# ########################################################################################

qt_add_executable(
  Input
  ${MM_SRCS}
  ${MM_HDRS}
  $<TARGET_OBJECTS:mm_core>
  main.cpp
)

target_include_directories(
  Input
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
         ${CMAKE_CURRENT_SOURCE_DIR}/attributes
         ${CMAKE_CURRENT_SOURCE_DIR}/map
         ${CMAKE_CURRENT_SOURCE_DIR}/layer
         ${CMAKE_CURRENT_SOURCE_DIR}/maptools
         ${CMAKE_CURRENT_SOURCE_DIR}/position
)

if (ANDROID)
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml)
    file(REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml)
  endif ()
  configure_file(
    ${INPUT_CMAKE_TEMPLATES_PATH}/AndroidManifest.xml.in
    ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml @ONLY
  )

  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle)
    file(REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle)
  endif ()
  configure_file(
    ${INPUT_CMAKE_TEMPLATES_PATH}/build.gradle.in
    ${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle @ONLY
  )

  set_target_properties(
    Input
    PROPERTIES QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
               QT_ANDROID_TARGET_SDK_VERSION ${INPUT_ANDROID_TARGET_SDK_VERSION}
               QT_ANDROID_MIN_SDK_VERSION ${INPUT_ANDROID_MIN_SDK_VERSION}
               QT_ANDROID_VERSION_CODE ${INPUT_VERSION_CODE}
               QT_ANDROID_VERSION_NAME ${INPUT_VERSION}
  )
endif ()

if (MACOS)
  set_target_properties(
    Input
    PROPERTIES MACOSX_BUNDLE TRUE
               MACOSX_RPATH TRUE
               MACOSX_BUNDLE_GUI_IDENTIFIER LutraConsultingLtd.Input
               MACOSX_BUNDLE_INFO_PLIST ${INPUT_CMAKE_TEMPLATES_PATH}/macOsInfo.plist.in
               MACOSX_BUNDLE_SHORT_VERSION_STRING ${INPUT_VERSION_CODE}
               XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@loader_path/Libraries"
               XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME TRUE
  )
endif ()

if (IOS)
  # qgis resources
  set(IOS_CRS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android/assets/qgis-data/resources)
  target_sources(Input PRIVATE ${IOS_CRS_DIR})
  set_source_files_properties(
    ${IOS_CRS_DIR} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/qgis-data
  )

  # proj
  set(IOS_PROJ_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android/assets/qgis-data/proj)
  target_sources(Input PRIVATE ${IOS_PROJ_DIR})
  set_source_files_properties(
    ${IOS_PROJ_DIR} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/qgis-data
  )

  # app icon
  set(IOS_ASSET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ios/Images.xcassets)
  target_sources(Input PRIVATE ${IOS_ASSET_DIR})
  set_source_files_properties(
    ${IOS_ASSET_DIR} PROPERTIES MACOSX_PACKAGE_LOCATION Resources
  )

  # launch screen
  set_target_properties(
    Input
    PROPERTIES QT_IOS_LAUNCH_SCREEN
               ${CMAKE_CURRENT_SOURCE_DIR}/ios/launchscreen/MMLaunchScreen.storyboard
  )

  set_target_properties(
    Input
    PROPERTIES MACOSX_BUNDLE TRUE
               MACOSX_BUNDLE_INFO_PLIST ${INPUT_CMAKE_TEMPLATES_PATH}/iOSInfo.plist.in
               MACOSX_BUNDLE_GUI_IDENTIFIER "LutraConsultingLtd.Input"
               MACOSX_BUNDLE_SHORT_VERSION_STRING ${INPUT_VERSION}
               MACOSX_BUNDLE_BUNDLE_VERSION ${INPUT_VERSION_CODE}
               XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "79QMH2QRAH"
               XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
  )

  if (IOS_USE_PRODUCTION_SIGNING)

    # use production signing profile
    set_target_properties(
      Input
      PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER
                 "LutraConsultingLtd.Input.AppStore"
                 XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY
                 "iPhone Distribution: LUTRA CONSULTING LIMITED (79QMH2QRAH)"
                 XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
    )
    message(STATUS "Using manual code sign with production certificate")

  else ()

    # use development profiles - automatically manage signing
    set_target_properties(Input PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
    message(STATUS "Using automatic code sign")
  endif ()
endif ()

# ########################################################################################
# LINKING Mergin Maps Target
# ########################################################################################

# at least on Linux qgis_core needs to be linked before Qt libs, otherwise one gets "DSO
# missing from command line" linker error about missing symbol from qt6::core5compat
# library - probably if our code does not use anything from that library, it is not added
# (linking order matters)
target_link_libraries(Input PUBLIC QGIS::Core)

target_link_libraries(
  Input
  PUBLIC Qt6::Quick
         Qt6::Qml
         Qt6::Xml
         Qt6::Concurrent
         Qt6::Positioning
         Qt6::Sensors
         Qt6::QuickControls2
         Qt6::Network
         Qt6::Svg
         Qt6::Sql
         Qt6::Core
         Qt6::Core5Compat
)

if (HAVE_BLUETOOTH)
  target_link_libraries(Input PUBLIC Qt6::Bluetooth)
endif ()

target_link_libraries(Input PUBLIC Qt6::Multimedia Qt6::Widgets)
if (NOT IOS)
  target_link_libraries(Input PUBLIC Qt6::PrintSupport)
endif ()

if (ANDROID)
  target_link_libraries(Input PUBLIC Qt6::CorePrivate)
endif ()

target_link_libraries(
  Input
  PUBLIC mm_core
         Geodiff::Geodiff
         Proj::Proj
         ZXing::ZXing
)

target_link_libraries(
  Input
  PUBLIC QGIS::authmethod_basic
         QGIS::authmethod_esritoken
         QGIS::authmethod_identcert
         QGIS::authmethod_oauth2
         QGIS::authmethod_pkcs12
         QGIS::authmethod_pkipaths
         QGIS::provider_arcgisfeatureserver
         QGIS::provider_arcgismapserver
         QGIS::provider_delimitedtext
         QGIS::provider_spatialite
         QGIS::provider_virtuallayer
         QGIS::provider_wcs
         QGIS::provider_wfs
         QGIS::provider_wms
         QGIS::provider_postgres
)

if (LNX)
  # needs to be added again because of the cycling dependencies between qgis_core and
  # providers and auth methods
  target_link_libraries(Input PUBLIC QGIS::Core)
endif ()

target_link_libraries(
  Input
  PUBLIC Qtkeychain::Qtkeychain
         Qca::Qca
         Gdal::Gdal
         Pq::Pq
         Pq::Pgcommon
         Pq::Pgport
         Spatialite::Spatialite
)

if (NOT LNX)
  target_link_libraries(Input PUBLIC Charset::Charset Iconv::Iconv)
endif ()

target_link_libraries(Input PUBLIC Xml2::Xml2)

target_link_libraries(
  Input
  PUBLIC Spatialindex::Spatialindex
         Geos::Geos-c
         Geos::Geos
         Protobuf::Protobuf-lite
         Expat::Expat
         Freexl::Freexl
         Exiv2::Exiv2
         Exiv2::Exiv2-xmp
         Tiff::Tiff
         Sqlite3::Sqlite3
         MiniZip::MiniZip
         ZLIB::ZLIB
         Zip::Zip
         WebP::WebP
         WebP::Sharpyuv
         Curl::Curl
         Jpeg::Jpeg
         Geotiff::Geotiff
         Lzma::Lzma
         Zstd::Zstd
         Bz2::Bz2
)

if (IOS OR MACOS)
  target_link_libraries(Input PUBLIC Tasn1::Tasn1)
endif ()

target_link_libraries(Input PUBLIC OpenSSL::OpenSSL OpenSSL::Crypto)
if (ANDROID)
  set_property(
    TARGET Input PROPERTY QT_ANDROID_EXTRA_LIBS ${KDAB_OpenSSL_CRYPTO_LIBRARY}
                          ${KDAB_OpenSSL_LIBRARY}
  )
endif ()

if (MACOS OR IOS)
  target_link_libraries(
    Input PUBLIC AppleFrameworks::Security AppleFrameworks::CoreFoundation
                 AppleFrameworks::SystemConfiguration
  )
endif ()

if (IOS)
  target_link_libraries(Input PUBLIC AppleFrameworks::CoreLocation)
endif ()

if (ENABLE_TESTS)
  target_link_libraries(Input PUBLIC Qt6::Test)
endif ()

# ########################################################################################
# QML
# ########################################################################################

target_compile_definitions(
  Input PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)

add_subdirectory(qml)

# register Singleton TODO REMOVE!
set_source_files_properties(qml/InputStyle.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# make sure the QML files are not prefixed with "qml/" in the qmldir
set(MM_QML_SRCS)
foreach (src ${MM_QML})
  set(MM_QML_SRCS ${MM_QML_SRCS} qml/${src})
  set_source_files_properties(qml/${src} PROPERTIES QT_RESOURCE_ALIAS ${src})
endforeach ()

# For some reason, this call cannot be in qml/CMakeLists.txt
qt_add_qml_module(
  Input
  URI MMInput
  VERSION 1.0
  QML_FILES ${MM_QML_SRCS}
  RESOURCE_PREFIX /com.merginmaps/imports NO_GENERATE_QMLTYPES
)
