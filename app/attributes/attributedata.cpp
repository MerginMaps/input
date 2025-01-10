/***************************************************************************
 attributedata.cpp
  --------------------------------------
  Date                 : 22.4.2021
  Copyright            : (C) 2021 by Peter Petrik
  Email                : zilolv@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "attributedata.h"

FormItem::FormItem( const QUuid &id,
                    const QgsField &field,
                    const QString &groupName,
                    const int parentTabId,
                    FormItem::FormItemType type,
                    const QString &name,
                    const QgsExpression &nameExpression,
                    bool showName,
                    bool isEditable,
                    const QgsExpression &editableExpression,
                    const QgsEditorWidgetSetup &editorWidgetSetup,
                    int fieldIndex,
                    const QgsExpression &visibilityExpression,
                    const QgsRelation &relation
                  )
  : mId( id )
  , mField( field )
  , mGroupName( groupName )
  , mParentTabId( parentTabId )
  , mType( type )
  , mName( name )
  , mNameExpression( nameExpression )
  , mShowName( showName )
  , mIsEditable( isEditable )
  , mEditableExpression( editableExpression )
  , mEditorWidgetSetup( editorWidgetSetup )
  , mFieldIndex( fieldIndex )
  , mVisibilityExpression( visibilityExpression )
  , mRelation( relation ) // no relation for type field
{
}

FormItem *FormItem::createFieldItem( const QUuid &id,
                                     const QgsField &field,
                                     const QString &groupName,
                                     int parentTabId,
                                     const QString &name, const QgsExpression &nameExpression, bool showName,
                                     bool isEditable, const QgsExpression &editableExpression,
                                     const QgsEditorWidgetSetup
                                     &editorWidgetSetup,
                                     int fieldIndex,
                                     const QgsExpression &visibilityExpression
                                   )
{
  return new FormItem(
           id,
           field,
           groupName,
           parentTabId,
           FormItem::Field,
           name,
           nameExpression,
           showName,
           isEditable,
           editableExpression,
           editorWidgetSetup,
           fieldIndex,
           visibilityExpression,
           QgsRelation()
         );
}

FormItem *FormItem::createRelationItem( const QUuid &id,
                                        const QString &groupName,
                                        int parentTabId,
                                        const QString &name,
                                        bool showName,
                                        const QgsExpression &visibilityExpression,
                                        const QgsRelation &relation
                                      )
{
  FormItem *item = new FormItem(
    id,
    QgsField(),
    groupName,
    parentTabId,
    FormItem::Relation,
    name,
    QgsExpression(),
    showName,
    true,
    QgsExpression(),
    QgsEditorWidgetSetup(),
    -1,
    visibilityExpression,
    relation
  );
  return item;
}

FormItem *FormItem::createSpacerItem(
  const QUuid &id,
  const QString &groupName,
  int parentTabId,
  const QString &name,
  bool isHLine,
  const QgsExpression &visibilityExpression
)
{
  QVariantMap map;
  map["IsHLine"] = isHLine;
  map["ConfigType"] = "merginmaps-custom-config";
  QgsEditorWidgetSetup config( "spacer", map );

  return new FormItem(
           id,
           QgsField(),
           groupName,
           parentTabId,
           FormItem::Spacer,
           name,
           QgsExpression(),
           false, // label is never shown for spacer
           false,
           QgsExpression(),
           config,
           -1,
           visibilityExpression,
           QgsRelation()
         );
}

FormItem *FormItem::createRichTextItem(
  const QUuid &id,
  const QString &groupName,
  int parentTabId,
  const QString &name,
  bool showName,
  const QString &text,
  bool isHtml,
  const QgsExpression &visibilityExpression
)
{
  QVariantMap map;
  map["UseHtml"] = isHtml;
  map["Definition"] = text;
  map["ConfigType"] = "merginmaps-custom-config";

  QgsEditorWidgetSetup config( "richtext", map );

  FormItem *fi = new FormItem(
    id,
    QgsField(),
    groupName,
    parentTabId,
    FormItem::RichText,
    name,
    QgsExpression(),
    showName,
    false,
    QgsExpression(),
    config,
    -1,
    visibilityExpression,
    QgsRelation()
  );

  fi->setRawValue( text );
  return fi;
}

FormItem::FormItemType FormItem::type() const
{
  return mType;
}

QString FormItem::name() const
{
  return mName;
}

void FormItem::setName( QString name )
{
  mName = name;
}

bool FormItem::isEditable() const
{
  return mIsEditable;
}

QString FormItem::editorWidgetType() const
{
  if ( mType == FormItem::Relation )
    return QStringLiteral( "relation" );

  return mEditorWidgetSetup.type();
}

QVariantMap FormItem::editorWidgetConfig() const
{
  return mEditorWidgetSetup.config();
}

int FormItem::fieldIndex() const
{
  return mFieldIndex;
}

QString FormItem::validationMessage() const
{
  return mValidationMessage;
}

void FormItem::setValidationMessage( QString message )
{
  mValidationMessage = message;
}

FieldValidator::ValidationStatus FormItem::validationStatus() const
{
  return mValidationStatus;
}

void FormItem::setValidationStatus( FieldValidator::ValidationStatus status )
{
  mValidationStatus = status;
}

bool FormItem::isVisible() const
{
  return mVisible;
}

void FormItem::setVisible( bool visible )
{
  mVisible = visible;
}

void FormItem::setIsEditable( bool editable )
{
  mIsEditable = editable;
}

QUuid FormItem::id() const
{
  return mId;
}

int FormItem::parentTabId() const
{
  return mParentTabId;
}

QgsExpression FormItem::visibilityExpression() const
{
  return mVisibilityExpression;
}

QgsExpression FormItem::nameExpression() const
{
  return mNameExpression;
}

QgsExpression FormItem::editableExpression() const
{
  return mEditableExpression;
}

bool FormItem::visible() const
{
  return mVisible;
}

QgsField FormItem::field() const
{
  return mField;
}

QString FormItem::groupName() const
{
  return mGroupName;
}

QVariant FormItem::originalValue() const
{
  return mOriginalValue;
}

void FormItem::setOriginalValue( const QVariant &originalValue )
{
  mOriginalValue = originalValue;
}

QgsRelation FormItem::relation() const
{
  return mRelation;
}

bool FormItem::showName() const
{
  return mShowName;
}

QVariant FormItem::rawValue() const
{
  return mRawValue;
}

void FormItem::setRawValue( const QVariant &rawValue )
{
  mRawValue = rawValue;
}


TabItem::TabItem( const int &id,
                  const QString &name,
                  const QVector<QUuid> &formItems,
                  const QgsExpression &visibilityExpression )
  : mTabIndex( id )
  , mName( name )
  , mFormItems( formItems )
  , mVisibilityExpression( visibilityExpression )
{
}

QString TabItem::name() const
{
  return mName;
}

bool TabItem::isVisible() const
{
  return mVisible;
}

void TabItem::setVisible( bool visible )
{
  mVisible = visible;
}

int TabItem::tabIndex() const
{
  return mTabIndex;
}

QgsExpression TabItem::visibilityExpression() const
{
  return mVisibilityExpression;
}

const QVector<QUuid> TabItem::formItems() const
{
  return mFormItems;
}
