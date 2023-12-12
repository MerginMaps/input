/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MMSTYLE_H
#define MMSTYLE_H

#include <QObject>
#include <QFont>
#include <QColor>
#include <QUrl>

class MMStyle: public QObject
{
    Q_OBJECT

    // Fonts - how to use
    // standard - font: __style.p5
    // extended - font.pixelSize: __style.p5.pixelSize; font.italic: true

    // Fonts - Heading
    Q_PROPERTY( QFont h1 READ h1 CONSTANT )
    Q_PROPERTY( QFont h2 READ h2 CONSTANT )
    Q_PROPERTY( QFont h3 READ h3 CONSTANT )

    // Fonts - Title
    Q_PROPERTY( QFont t1 READ t1 CONSTANT )
    Q_PROPERTY( QFont t2 READ t2 CONSTANT )
    Q_PROPERTY( QFont t3 READ t3 CONSTANT )
    Q_PROPERTY( QFont t4 READ t4 CONSTANT )
    Q_PROPERTY( QFont t5 READ t5 CONSTANT )

    // Fonts - Paragraph
    Q_PROPERTY( QFont p1 READ p1 CONSTANT )
    Q_PROPERTY( QFont p2 READ p2 CONSTANT )
    Q_PROPERTY( QFont p3 READ p3 CONSTANT )
    Q_PROPERTY( QFont p4 READ p4 CONSTANT )
    Q_PROPERTY( QFont p5 READ p5 CONSTANT )
    Q_PROPERTY( QFont p6 READ p6 CONSTANT )
    Q_PROPERTY( QFont p7 READ p7 CONSTANT )

    // Colors - primary palette
    Q_PROPERTY( QColor grassColor READ grassColor CONSTANT )
    Q_PROPERTY( QColor forestColor READ forestColor CONSTANT )
    Q_PROPERTY( QColor nightColor READ nightColor CONSTANT )
    Q_PROPERTY( QColor whiteColor READ whiteColor CONSTANT )
    Q_PROPERTY( QColor transparentColor READ transparentColor CONSTANT )

    // Colors - secondary palette
    Q_PROPERTY( QColor lightGreenColor READ lightGreenColor CONSTANT )
    Q_PROPERTY( QColor mediumGreenColor READ mediumGreenColor CONSTANT )
    Q_PROPERTY( QColor grayColor READ grayColor CONSTANT )

    // Colors - additional colors
    Q_PROPERTY( QColor sandColor READ sandColor CONSTANT )
    Q_PROPERTY( QColor sunsetColor READ sunsetColor CONSTANT )
    Q_PROPERTY( QColor sunColor READ sunColor CONSTANT )
    Q_PROPERTY( QColor earthColor READ earthColor CONSTANT )
    Q_PROPERTY( QColor roseColor READ roseColor CONSTANT )
    Q_PROPERTY( QColor skyColor READ skyColor CONSTANT )
    Q_PROPERTY( QColor grapeColor READ grapeColor CONSTANT )
    Q_PROPERTY( QColor deepOceanColor READ deepOceanColor CONSTANT )
    Q_PROPERTY( QColor purpleColor READ purpleColor CONSTANT )
    Q_PROPERTY( QColor fieldColor READ fieldColor CONSTANT )

    // Colors - sentiment colors
    Q_PROPERTY( QColor positiveColor READ positiveColor CONSTANT )
    Q_PROPERTY( QColor warningColor READ warningColor CONSTANT )
    Q_PROPERTY( QColor negativeColor READ negativeColor CONSTANT )
    Q_PROPERTY( QColor informativeColor READ informativeColor CONSTANT )

    // Colors - others
    Q_PROPERTY( QColor nightAlphaColor READ nightAlphaColor CONSTANT ) // placeholder input color
    Q_PROPERTY( QColor errorBgInputColor READ errorBgInputColor CONSTANT ) // error bg input color
    Q_PROPERTY( QColor shadowColor READ shadowColor CONSTANT )

    // Icons
    Q_PROPERTY( QUrl archaeologyIcon READ archaeologyIcon CONSTANT )
    Q_PROPERTY( QUrl arrowDownIcon READ arrowDownIcon CONSTANT )
    Q_PROPERTY( QUrl arrowLinkRightIcon READ arrowLinkRightIcon CONSTANT )
    Q_PROPERTY( QUrl arrowUpIcon READ arrowUpIcon CONSTANT )
    Q_PROPERTY( QUrl backIcon READ backIcon CONSTANT )
    Q_PROPERTY( QUrl briefcaseIcon READ briefcaseIcon CONSTANT )
    Q_PROPERTY( QUrl bubbleIcon READ bubbleIcon CONSTANT )
    Q_PROPERTY( QUrl calendarIcon READ calendarIcon CONSTANT )
    Q_PROPERTY( QUrl checkmarkIcon READ checkmarkIcon CONSTANT )
    Q_PROPERTY( QUrl closeButtonIcon READ closeButtonIcon CONSTANT )
    Q_PROPERTY( QUrl closeIcon READ closeIcon CONSTANT )
    Q_PROPERTY( QUrl deleteIcon READ deleteIcon CONSTANT )
    Q_PROPERTY( QUrl doneIcon READ doneIcon CONSTANT )
    Q_PROPERTY( QUrl downloadIcon READ downloadIcon CONSTANT )
    Q_PROPERTY( QUrl editIcon READ editIcon CONSTANT )
    Q_PROPERTY( QUrl electricityIcon READ electricityIcon CONSTANT )
    Q_PROPERTY( QUrl engineeringIcon READ engineeringIcon CONSTANT )
    Q_PROPERTY( QUrl environmentalIcon READ environmentalIcon CONSTANT )
    Q_PROPERTY( QUrl errorIcon READ errorIcon CONSTANT )
    Q_PROPERTY( QUrl facebookIcon READ facebookIcon CONSTANT )
    Q_PROPERTY( QUrl hideIcon READ hideIcon CONSTANT )
    Q_PROPERTY( QUrl infoIcon READ infoIcon CONSTANT )
    Q_PROPERTY( QUrl linkedinIcon READ linkedinIcon CONSTANT )
    Q_PROPERTY( QUrl mastodonIcon READ mastodonIcon CONSTANT )
    Q_PROPERTY( QUrl moreIcon READ moreIcon CONSTANT )
    Q_PROPERTY( QUrl morePhotosIcon READ morePhotosIcon CONSTANT )
    Q_PROPERTY( QUrl mouthIcon READ mouthIcon CONSTANT )
    Q_PROPERTY( QUrl naturalResourcesIcon READ naturalResourcesIcon CONSTANT )
    Q_PROPERTY( QUrl otherIcon READ otherIcon CONSTANT )
    Q_PROPERTY( QUrl othersIcon READ othersIcon CONSTANT )
    Q_PROPERTY( QUrl projectButtonMoreIcon READ projectButtonMoreIcon CONSTANT )
    Q_PROPERTY( QUrl qgisIcon READ qgisIcon CONSTANT )
    Q_PROPERTY( QUrl qrCodeIcon READ qrCodeIcon CONSTANT )
    Q_PROPERTY( QUrl redditIcon READ redditIcon CONSTANT )
    Q_PROPERTY( QUrl searchIcon READ searchIcon CONSTANT )
    Q_PROPERTY( QUrl showIcon READ showIcon CONSTANT )
    Q_PROPERTY( QUrl socialMediaIcon READ socialMediaIcon CONSTANT )
    Q_PROPERTY( QUrl stateAndLocalIcon READ stateAndLocalIcon CONSTANT )
    Q_PROPERTY( QUrl stopIcon READ stopIcon CONSTANT )
    Q_PROPERTY( QUrl subscriptionsIcon READ subscriptionsIcon CONSTANT )
    Q_PROPERTY( QUrl syncIcon READ syncIcon CONSTANT )
    Q_PROPERTY( QUrl teacherIcon READ teacherIcon CONSTANT )
    Q_PROPERTY( QUrl telecommunicationIcon READ telecommunicationIcon CONSTANT )
    Q_PROPERTY( QUrl termsIcon READ termsIcon CONSTANT )
    Q_PROPERTY( QUrl tractorIcon READ tractorIcon CONSTANT )
    Q_PROPERTY( QUrl transportationIcon READ transportationIcon CONSTANT )
    Q_PROPERTY( QUrl waitingIcon READ waitingIcon CONSTANT )
    Q_PROPERTY( QUrl waterResourcesIcon READ waterResourcesIcon CONSTANT )
    Q_PROPERTY( QUrl xMarkIcon READ xMarkIcon CONSTANT )
    Q_PROPERTY( QUrl xTwitterIcon READ xTwitterIcon CONSTANT )
    Q_PROPERTY( QUrl youtubeIcon READ youtubeIcon CONSTANT )
    Q_PROPERTY( QUrl globeIcon READ globeIcon CONSTANT )

    // Images
    Q_PROPERTY( QUrl acceptInvitationImage READ acceptInvitationImage CONSTANT )
    Q_PROPERTY( QUrl acceptInvitationLogoImage READ acceptInvitationLogoImage CONSTANT )
    Q_PROPERTY( QUrl reachedDataLimitImage READ reachedDataLimitImage CONSTANT )
    Q_PROPERTY( QUrl uploadImage READ uploadImage CONSTANT )

    // Map items
    Q_PROPERTY( double mapItemHeight READ mapItemHeight CONSTANT )

    // Toolbar
    Q_PROPERTY( double toolbarHeight READ toolbarHeight CONSTANT )
    Q_PROPERTY( double menuDrawerHeight READ menuDrawerHeight CONSTANT )

    // Other
    Q_PROPERTY( double inputRadius READ inputRadius CONSTANT )

  public:
    explicit MMStyle( qreal dp )
      : mDp( dp )
    {}
    ~MMStyle() = default;

    QFont h1() {return fontFactory( 48, true );}
    QFont h2() {return fontFactory( 36, true );}
    QFont h3() {return fontFactory( 24, true );}

    QFont t1() {return fontFactory( 18, true );}
    QFont t2() {return fontFactory( 16, true );}
    QFont t3() {return fontFactory( 14, true );}
    QFont t4() {return fontFactory( 12, true );}
    QFont t5() {return fontFactory( 10, true );}

    QFont p1() {return fontFactory( 32, false );}
    QFont p2() {return fontFactory( 24, false );}
    QFont p3() {return fontFactory( 20, false );}
    QFont p4() {return fontFactory( 16, false );}
    QFont p5() {return fontFactory( 14, false );}
    QFont p6() {return fontFactory( 12, false );}
    QFont p7() {return fontFactory( 10, false );}

    QColor grassColor() {return QColor::fromString( "#73D19C" );}
    QColor forestColor() {return QColor::fromString( "#004C45" );}
    QColor nightColor() {return QColor::fromString( "#12181F" );}
    QColor whiteColor() {return QColor::fromString( "#FFFFFF" );}
    QColor transparentColor() {return QColor::fromString( "transparent" );}

    QColor lightGreenColor() {return QColor::fromString( "#EFF5F3" );}
    QColor mediumGreenColor() {return QColor::fromString( "#B7CDC4" );}
    QColor grayColor() {return QColor::fromString( "#E2E2E2" );}

    QColor sandColor() {return QColor::fromString( "#FFF4E2" );}
    QColor sunsetColor() {return QColor::fromString( "#FFB673" );}
    QColor sunColor() {return QColor::fromString( "#F4CB46" );}
    QColor earthColor() {return QColor::fromString( "#4D2A24" );}
    QColor roseColor() {return QColor::fromString( "#FFBABC" );}
    QColor skyColor() {return QColor::fromString( "#A6CBF4" );}
    QColor grapeColor() {return QColor::fromString( "#5A2740" );}
    QColor deepOceanColor() {return QColor::fromString( "#1C324A" );}
    QColor purpleColor() {return QColor::fromString( "#CCBDF5" );}
    QColor fieldColor() {return QColor::fromString( "#9BD1A9" );}

    QColor positiveColor() {return QColor::fromString( "#C2FFA6" );}
    QColor warningColor() {return QColor::fromString( "#FFD6A6" );}
    QColor negativeColor() {return QColor::fromString( "#FFA6A6" );}
    QColor informativeColor() {return QColor::fromString( "#A6F4FF" );}

    QColor nightAlphaColor() {return QColor::fromString( "#AA12181F" );}
    QColor errorBgInputColor() {return QColor::fromString( "#FEFAF9" );}
    QColor shadowColor() {return QColor::fromString( "#66777777" );}

    QUrl arrowLinkRightIcon() {return QUrl( "qrc:/Arrow Link Right.svg" );}
    QUrl searchIcon() {return QUrl( "qrc:/Search.svg" );}
    QUrl backIcon() {return QUrl( "qrc:/Back.svg" );}
    QUrl bubbleIcon() {return QUrl( "qrc:/Bubble.svg" );}
    QUrl calendarIcon() {return QUrl( "qrc:/Calendar.svg" );}
    QUrl showIcon() {return QUrl( "qrc:/Show.svg" );}
    QUrl hideIcon() {return QUrl( "qrc:/Hide.svg" );}
    QUrl xMarkIcon() {return QUrl( "qrc:/X Mark.svg" );}
    QUrl errorIcon() {return QUrl( "qrc:/Error.svg" );}
    QUrl arrowUpIcon() {return QUrl( "qrc:/Arrow Up.svg" );}
    QUrl arrowDownIcon() {return QUrl( "qrc:/Arrow Down.svg" );}
    QUrl qrCodeIcon() {return QUrl( "qrc:/QR Code.svg" );}
    QUrl checkmarkIcon() {return QUrl( "qrc:/Checkmark.svg" );}
    QUrl closeButtonIcon() {return QUrl( "qrc:/CloseButton.svg" );}
    QUrl closeIcon() {return QUrl( "qrc:/Close.svg" );}
    QUrl waitingIcon() {return QUrl( "qrc:/Waiting.svg" );}
    QUrl deleteIcon() {return QUrl( "qrc:/Delete.svg" );}
    QUrl doneIcon() {return QUrl( "qrc:/Done.svg" );}
    QUrl editIcon() {return QUrl( "qrc:/Edit.svg" );}
    QUrl moreIcon() {return QUrl( "qrc:/More.svg" );}
    QUrl projectButtonMoreIcon() {return QUrl( "qrc:/ProjectButtonMore.svg" );}
    QUrl stopIcon() {return QUrl( "qrc:/Stop.svg" );}
    QUrl syncIcon() {return QUrl( "qrc:/Sync.svg" );}
    QUrl infoIcon() {return QUrl( "qrc:/Info.svg" );}
    QUrl downloadIcon() {return QUrl( "qrc:/Download.svg" );}
    QUrl morePhotosIcon() {return QUrl( "qrc:/MorePhotos.svg" );}
    QUrl archaeologyIcon() {return QUrl( "qrc:/Archaeology.svg" );}
    QUrl briefcaseIcon() {return QUrl( "qrc:/Briefcase.svg" );}
    QUrl electricityIcon() {return QUrl( "qrc:/Electricity.svg" );}
    QUrl engineeringIcon() {return QUrl( "qrc:/Engineering.svg" );}
    QUrl environmentalIcon() {return QUrl( "qrc:/Environmental.svg" );}
    QUrl facebookIcon() {return QUrl( "qrc:/Facebook.svg" );}
    QUrl linkedinIcon() {return QUrl( "qrc:/Linkedin.svg" );}
    QUrl mastodonIcon() {return QUrl( "qrc:/Mastodon.svg" );}
    QUrl mouthIcon() {return QUrl( "qrc:/Mouth.svg" );}
    QUrl naturalResourcesIcon() {return QUrl( "qrc:/Natural Resources.svg" );}
    QUrl otherIcon() {return QUrl( "qrc:/Other.svg" );}
    QUrl othersIcon() {return QUrl( "qrc:/Others.svg" );}
    QUrl qgisIcon() {return QUrl( "qrc:/QGIS.svg" );}
    QUrl redditIcon() {return QUrl( "qrc:/Reddit.svg" );}
    QUrl socialMediaIcon() {return QUrl( "qrc:/Social media.svg" );}
    QUrl stateAndLocalIcon() {return QUrl( "qrc:/State and Local.svg" );}
    QUrl subscriptionsIcon() {return QUrl( "qrc:/Subscriptions.svg" );}
    QUrl teacherIcon() {return QUrl( "qrc:/Teacher.svg" );}
    QUrl telecommunicationIcon() {return QUrl( "qrc:/Telecommunication.svg" );}
    QUrl termsIcon() {return QUrl( "qrc:/Terms.svg" );}
    QUrl tractorIcon() {return QUrl( "qrc:/Tractor.svg" );}
    QUrl transportationIcon() {return QUrl( "qrc:/Transportation.svg" );}
    QUrl waterResourcesIcon() {return QUrl( "qrc:/Water resources.svg" );}
    QUrl xTwitterIcon() {return QUrl( "qrc:/X Twitter.svg" );}
    QUrl youtubeIcon() {return QUrl( "qrc:/Youtube.svg" );}
    QUrl globeIcon() {return QUrl( "qrc:/Globe.svg" );}

    QUrl acceptInvitationLogoImage() {return QUrl( "qrc:/AcceptInvitationLogoImage.svg" ); }
    QUrl acceptInvitationImage() {return QUrl( "qrc:/AcceptInvitationImage.svg" ); }
    QUrl uploadImage() {return QUrl( "qrc:/UploadImage.svg" );}
    QUrl reachedDataLimitImage() {return QUrl( "qrc:/ReachedDataLimitImage.svg" );}

    double mapItemHeight() {return 50 * mDp;}

    double toolbarHeight() {return 89 * mDp;}
    double menuDrawerHeight() {return 67 * mDp;}

    double inputRadius() {return 12 * mDp;}

  signals:
    void styleChanged();

  private:
    QFont fontFactory( int pixelSize, bool bold )
    {
      QFont f;
      f.setBold( bold );
      f.setPixelSize( pixelSize * mDp );
      return f;
    }

    qreal mDp;

};

#endif // MMSTYLE_H
