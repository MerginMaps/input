/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "merginuserinfo.h"
#include "inpututils.h"

MerginUserInfo::MerginUserInfo( QObject *parent )
  : QObject( parent )
{
  clear();
}

void MerginUserInfo::clear()
{
  mEmail = "";
  mPlanAlias = "";
  mPlanMerginId = "";
  mPlanProvider = "";
  mPlanProductId = "";
  mNextBillPrice = "";
  mSubscriptionStatus = MerginSubscriptionStatus::FreeSubscription;
  mDiskUsage = 0;
  mStorageLimit = 0;
  emit userInfoChanged();
}

void MerginUserInfo::setFromJson( QJsonObject docObj )
{
  mEmail = docObj.value( QStringLiteral( "email" ) ).toString();
  mNextBillPrice = docObj.value( QStringLiteral( "next_bill_price" ) ).toString();
  QString timestamp = docObj.value( QStringLiteral( "next_payment_date" ) ).toString();
  mSubscriptionTimestamp = InputUtils::localizedDateFromUTFString( timestamp );
  mDiskUsage = docObj.value( QStringLiteral( "disk_usage" ) ).toDouble();
  mStorageLimit = docObj.value( QStringLiteral( "storage" ) ).toDouble();
  mOwnsActiveSubscription = docObj.value( QStringLiteral( "is_paid_plan" ) ).toBool();

  QString status = docObj.value( QStringLiteral( "status" ) ).toString();
  if ( status == "active" )
  {
    if ( mOwnsActiveSubscription )
    {
      mSubscriptionStatus = MerginSubscriptionStatus::ValidSubscription;
    }
    else
    {
      mSubscriptionStatus = MerginSubscriptionStatus::FreeSubscription;
    }
  }
  else if ( status == "unsubscribed" )
  {
    mSubscriptionStatus = MerginSubscriptionStatus::SubscriptionUnsubscribed;
  }
  else if ( status == "past_due" )
  {
    mSubscriptionStatus = MerginSubscriptionStatus::SubscriptionInGracePeriod;
  }
  else
  {
    // internal error some new mergin api? what to do?
    mSubscriptionStatus = MerginSubscriptionStatus::FreeSubscription;
  }

  QJsonObject planObj = docObj.value( QStringLiteral( "plan" ) ).toObject();
  mPlanMerginId = planObj.value( QStringLiteral( "id" ) ).toString();
  mPlanAlias = planObj.value( QStringLiteral( "alias" ) ).toString();
  mPlanProvider = planObj.value( QStringLiteral( "type" ) ).toString();
  mPlanProductId = planObj.value( QStringLiteral( "product_id" ) ).toString();

  emit userInfoChanged();
}

bool MerginUserInfo::ownsActiveSubscription() const
{
  return mOwnsActiveSubscription;
}

void MerginUserInfo::setPaidPlan( bool paidPlan )
{
  mOwnsActiveSubscription = paidPlan;
  emit userInfoChanged();
}

QString MerginUserInfo::planMerginId() const
{
  return mPlanMerginId;
}

void MerginUserInfo::setPlanMerginId( const QString &planMerginId )
{
  mPlanMerginId = planMerginId;
}

QString MerginUserInfo::planProvider() const
{
  return mPlanProvider;
}

void MerginUserInfo::setPlanProvider( const QString &planProvider )
{
  mPlanProvider = planProvider;
}

QString MerginUserInfo::planProductId() const
{
  return mPlanProductId;
}

void MerginUserInfo::setPlanProductId( const QString &planProductId )
{
  mPlanProductId = planProductId;
}

QString MerginUserInfo::email() const
{
  return mEmail;
}

QString MerginUserInfo::planAlias() const
{
  return mPlanAlias;
}

QString MerginUserInfo::nextBillPrice() const
{
  return mNextBillPrice;
}

int MerginUserInfo::subscriptionStatus() const
{
  return mSubscriptionStatus;
}

double MerginUserInfo::diskUsage() const
{
  return mDiskUsage;
}

double MerginUserInfo::storageLimit() const
{
  return mStorageLimit;
}

void MerginUserInfo::setEmail( const QString &email )
{
  mEmail = email;
  emit userInfoChanged();
}

void MerginUserInfo::setPlanAlias( const QString &plan )
{
  mPlanAlias = plan;
  emit userInfoChanged();
}

void MerginUserInfo::setNextBillPrice( const QString &nextBillPrice )
{
  mNextBillPrice = nextBillPrice;
  emit userInfoChanged();
}

void MerginUserInfo::setSubscriptionStatus( const MerginSubscriptionStatus::SubscriptionStatus &subscriptionStatus )
{
  mSubscriptionStatus = subscriptionStatus;
  emit userInfoChanged();
}

void MerginUserInfo::setDiskUsage( double diskUsage )
{
  mDiskUsage = diskUsage;
  emit userInfoChanged();
}

void MerginUserInfo::setStorageLimit( double storageLimit )
{
  mStorageLimit = storageLimit;
  emit userInfoChanged();
}

void MerginUserInfo::setSubscriptionTimestamp( const QString &subscriptionTimestamp )
{
  mSubscriptionTimestamp = subscriptionTimestamp;
  emit userInfoChanged();
}

QString MerginUserInfo::subscriptionTimestamp() const
{
  return mSubscriptionTimestamp;
}
