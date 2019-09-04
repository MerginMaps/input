ios {
    # https://doc.qt.io/qt-5.9/platform-notes-ios.html

    crsFiles.files = $$files(android/assets/qgis-data/resources/*.db)
    crsFiles.path = qgis-data/resources
    QMAKE_BUNDLE_DATA += crsFiles

    # app icon
    appIcons.files = $$files($$PWD/ios/appicon/*.png)
    QMAKE_BUNDLE_DATA += appIcons

    # launch screen
    app_launch_images.files = $$PWD/ios/launchscreen/Launch.xib $$files($$PWD/ios/launchscreen/*.png)
    QMAKE_BUNDLE_DATA += app_launch_images
}
