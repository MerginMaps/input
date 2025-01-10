/***************************************************************************
 attributedata.h
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

#ifndef ATTRIBUTEDATA_H
#define ATTRIBUTEDATA_H


#include <QString>
#include <QVariant>
#include <QVector>
#include <QUuid>
#include <QVariantMap>
#include <QObject>
#include <memory>

#include "inputconfig.h"
#include "qgsfieldconstraints.h"
#include "qgseditorwidgetsetup.h"
#include "qgsexpression.h"
#include "qgsrelation.h"
#include "qgsfield.h"

#include "fieldvalidator.h"

class FormItem
{
    Q_GADGET

  public:
    enum FormItemType
    {
      Invalid = 1,
      Container,
      Relation,
      Field,
      Spacer,
      RichText
    };
    Q_ENUMS( FormItemType )

    FormItem(
      const QUuid &id,
      const QgsField &field,
      const QString &groupName,
      int parentTabId,
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
    );

    static FormItem *createFieldItem(
      const QUuid &id,
      const QgsField &field,
      const QString &groupName,
      int parentTabId,
      const QString &name,
      const QgsExpression &nameExpression,
      bool showName,
      bool isEditable,
      const QgsExpression &editableExpression,
      const QgsEditorWidgetSetup &editorWidgetSetup,
      int fieldIndex,
      const QgsExpression &visibilityExpression
    );

    static FormItem *createRelationItem(
      const QUuid &id,
      const QString &groupName,
      int parentTabId,
      const QString &name,
      bool showName,
      const QgsExpression &visibilityExpression,
      const QgsRelation &relation
    );

    static FormItem *createSpacerItem(
      const QUuid &id,
      const QString &groupName,
      int parentTabId,
      const QString &name,
      bool isHLine,
      const QgsExpression &visibilityExpression
    );

    static FormItem *createRichTextItem(
      const QUuid &id,
      const QString &groupName,
      int parentTabId,
      const QString &name,
      bool showName,
      const QString &text,
      bool isHtml,
      const QgsExpression &visibilityExpression
    );

    FormItem::FormItemType type() const;

    QString name() const;
    void setName( QString name );

    QString editorWidgetType() const;
    QVariantMap editorWidgetConfig() const;
    int fieldIndex() const;

    QString validationMessage() const;
    void setValidationMessage( QString message );

    FieldValidator::ValidationStatus validationStatus() const;
    void setValidationStatus( FieldValidator::ValidationStatus status );

    bool isEditable() const;
    void setIsEditable( bool editable );

    bool isVisible() const;
    void setVisible( bool visible );

    QUuid id() const;

    int parentTabId() const;

    QgsExpression visibilityExpression() const;
    QgsExpression nameExpression() const;
    QgsExpression editableExpression() const;

    bool visible() const;

    QgsField field() const;

    QString groupName() const;

    QVariant originalValue() const;
    void setOriginalValue( const QVariant &originalValue );

    QVariant rawValue() const;
    void setRawValue( const QVariant &rawValue );

    QgsRelation relation() const;
    QString fieldError() const;

    bool showName() const;

  private:

    const QUuid mId;
    const QgsField mField;
    const QString mGroupName; //empty for no group, group/tab name if widget is in container
    const int mParentTabId;
    const FormItem::FormItemType mType;
    const bool mShowName = true; // "Show label" in Widget Display group in QGIS widget settings
    const QgsEditorWidgetSetup mEditorWidgetSetup;
    const int mFieldIndex;
    const QgsExpression mVisibilityExpression;
    const QgsExpression mNameExpression; // Expression to define field’s display name (alias)
    const QgsExpression mEditableExpression; // Expression to determine whether the field is editable

    QString mName;
    QString mValidationMessage;
    FieldValidator::ValidationStatus mValidationStatus = FieldValidator::Valid;
    bool mIsEditable = true;
    bool mVisible = false;
    QVariant mOriginalValue; // original unmodified value
    QVariant mRawValue;

    const QgsRelation mRelation; // Only used for FormItemType::Relation
};

class  TabItem
{
  public:
    TabItem( const int &id,
             const QString &name,
             const QVector<QUuid> &formItems,
             const QgsExpression &visibilityExpression
           );

    QString name() const;
    const QVector<QUuid> formItems() const;

    bool isVisible() const;
    void setVisible( bool visible );

    int tabIndex() const;

    QgsExpression visibilityExpression() const;

  private:
    const int mTabIndex;
    const QString mName;
    const QVector<QUuid> mFormItems;
    const QgsExpression mVisibilityExpression;
    bool mVisible = false;
};

#endif // ATTRIBUTEDATA_H
