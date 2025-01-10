/***************************************************************************
 attributecontroller.cpp
  --------------------------------------
  Date                 : 20.4.2021
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

#include "attributecontroller.h"
#include "attributeformmodel.h"
#include "attributetabmodel.h"
#include "fieldvalidator.h"

#include <QDebug>
#include <QSet>

#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgsattributeeditorfield.h"
#include "qgsattributeeditorrelation.h"
#include "qgsattributeeditorcontainer.h"
#include "qgsvectorlayerutils.h"
#include "qgsattributeeditorspacerelement.h"
#include "qgsattributeeditortextelement.h"
#include "qgsattributeeditorhtmlelement.h"
#include "qgsvectorlayereditbuffer.h"
#include "qgsexpressioncontextutils.h"
#include "qgsrelation.h"
#include "inpututils.h"
#include "coreutils.h"

AttributeController::AttributeController( QObject *parent )
  : QObject( parent )
  , mAttributeTabProxyModel( new AttributeTabProxyModel() )
{
}

void AttributeController::reset()
{
  setFeatureLayerPair( FeatureLayerPair() );
}

AttributeController::~AttributeController() = default;


FeatureLayerPair AttributeController::featureLayerPair() const
{
  return mFeatureLayerPair;
}

void AttributeController::setFeatureLayerPair( const FeatureLayerPair &pair )
{
  if ( mFeatureLayerPair != pair )
  {
    // block signals:
    // everything should be invalidated by
    // featureLayerPairChanged & attributeTabProxyModelChanged signals
    blockSignals( true );

    bool hasLayerChanged = mFeatureLayerPair.layer() != pair.layer();
    // Set new active pair
    mFeatureLayerPair = pair;
    if ( hasLayerChanged )
    {
      // layer changed!
      updateOnLayerChange();
    }

    // feature changed!
    updateOnFeatureChange();

    // Done, emit signals
    blockSignals( false );
    if ( hasLayerChanged )
    {
      emit attributeTabProxyModelChanged();
      emit hasTabsChanged();
    }
    emit featureLayerPairChanged();
    emit hasAnyChangesChanged();
    emit hasValidationErrorsChanged();
  }
}

QgsAttributeEditorContainer *AttributeController::autoLayoutTabContainer() const  //#spellok
{
  QgsVectorLayer *layer = mFeatureLayerPair.layer();
  Q_ASSERT( layer );

  std::unique_ptr<QgsAttributeEditorContainer> root( new QgsAttributeEditorContainer( QLatin1String( "AutoLayoutRoot" ), nullptr ) );
  root->setIsGroupBox( false ); //tab!

  const QgsFields fields = layer->fields();
  for ( int i = 0; i < fields.size(); ++i )
  {
    QgsAttributeEditorField *field = new QgsAttributeEditorField( fields.at( i ).name(), i, root.get() );
    root->addChildElement( field );
  }
  return root.release();
}

QgsEditorWidgetSetup AttributeController::getEditorWidgetSetup( QgsVectorLayer *layer, int fieldIndex ) const
{
  QgsEditorWidgetSetup setup = layer->editorWidgetSetup( fieldIndex );
  if ( setup.type().isEmpty() )
  {
    QgsField field = layer->fields().at( fieldIndex );
    return InputUtils::getEditorWidgetSetup( field );
  }
  return setup;
}

void AttributeController::prefillRelationReferenceField()
{
  if ( !mParentController || !mLinkedRelation.isValid() )
    return;

  const QList<QgsRelation::FieldPair> fieldPairs = mLinkedRelation.fieldPairs();
  for ( const QgsRelation::FieldPair &fieldPair : fieldPairs )
  {
    QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
    while ( formItemsIterator != mFormItems.end() )
    {
      std::shared_ptr<FormItem> itemData = formItemsIterator.value();
      if ( itemData->field().name() == fieldPair.referencingField() )
      {
        QVariant fk = mParentController->featureLayerPair().feature().attribute( fieldPair.referencedField() );
        setFormValue( itemData->id(), fk );
        break;
      }
      ++formItemsIterator;
    }
  }
}

bool AttributeController::allowTabs( QgsAttributeEditorContainer *container )
{
  for ( QgsAttributeEditorElement *element : container->children() )
  {
    if ( element->type() == Qgis::AttributeEditorType::Container )
    {
      QgsAttributeEditorContainer *elemContainer = static_cast<QgsAttributeEditorContainer *>( element );
      if ( elemContainer->isGroupBox() )
        return false;
    }
    else
      return false;
  }

  return !container->children().isEmpty();
}

VariablesManager *AttributeController::variablesManager() const
{
  return mVariablesManager;
}

void AttributeController::setVariablesManager( VariablesManager *variablesManager )
{
  if ( mVariablesManager != variablesManager )
  {
    mVariablesManager = variablesManager;
    emit variablesManagerChanged();
  }
}

RememberAttributesController *AttributeController::rememberAttributesController() const
{
  return mRememberAttributesController;
}

void AttributeController::setRememberAttributesController( RememberAttributesController *rememberAttributes )
{
  if ( mRememberAttributesController != rememberAttributes )
  {
    mRememberAttributesController = rememberAttributes;
    if ( mRememberAttributesController && mFeatureLayerPair.layer() )
      mRememberAttributesController->storeLayerFields( mFeatureLayerPair.layer() );
    emit rememberAttributesChanged();
  }
}

void AttributeController::flatten(
  QgsAttributeEditorContainer *container,
  int parentTabRow,
  const QString &parentVisibilityExpressions,
  QVector<QUuid> &items
)
{
  QgsVectorLayer *layer = mFeatureLayerPair.layer();
  Q_ASSERT( layer );

  for ( QgsAttributeEditorElement *element : container->children() )
  {
    switch ( element->type() )
    {
      case Qgis::AttributeEditorType::Container:
      {
        QString visibilityExpression = parentVisibilityExpressions;
        QgsAttributeEditorContainer *container = static_cast<QgsAttributeEditorContainer *>( element );
        if ( container->visibilityExpression().enabled() )
        {
          if ( visibilityExpression.isNull() )
            visibilityExpression = container->visibilityExpression().data().expression();
          else
            visibilityExpression += " AND " + container->visibilityExpression().data().expression();
        }

        flatten( container, parentTabRow, visibilityExpression, items );
        break;
      }

      case Qgis::AttributeEditorType::Field:
      {
        QUuid fieldUuid = QUuid::createUuid();

        QgsAttributeEditorField *editorField = static_cast<QgsAttributeEditorField *>( element );
        int fieldIndex = editorField->idx();
        if ( fieldIndex < 0 || fieldIndex >= layer->fields().size() )
        {
          CoreUtils::log( "Forms", QStringLiteral( "Invalid fieldIndex for editorField %1 in layer %2" ).arg( editorField->name(), layer->name() ) );
          continue;
        }
        QgsField field = layer->fields().at( fieldIndex );
        QStringList expressions;
        QString expression = field.constraints().constraintExpression();

        QgsEditFormConfig editFormConfig = layer->editFormConfig();
        QString fieldName = field.name();
        QgsPropertyCollection fieldProperties = editFormConfig.dataDefinedFieldProperties( fieldName );

        // Retrieving field name expression
        QgsProperty nameProperty = fieldProperties.property( QgsEditFormConfig::DataDefinedProperty::Alias );
        QgsExpression nameExpression; // empty if users set to hide the field label

        if ( !editorField->showLabel() )
        {
          QgsExpression nameExpression = QgsExpression( nameProperty.expressionString() );
        }

        // Retrieving field editability expression
        QgsProperty editableProperty = fieldProperties.property( QgsEditFormConfig::DataDefinedProperty::Editable );
        bool isReadOnly = ( layer->editFormConfig().readOnly( fieldIndex ) ) ||
                          ( !field.defaultValueDefinition().expression().isEmpty() && field.defaultValueDefinition().applyOnUpdate() );
        QgsExpression isEditableExpression; // empty if the field is read-only

        if ( !isReadOnly )
        {
          QgsProperty editableProperty = fieldProperties.property( QgsEditFormConfig::DataDefinedProperty::Editable );
          isEditableExpression = QgsExpression( editableProperty.expressionString() );
        }

        if ( !expression.isEmpty() )
        {
          expressions << field.constraints().constraintExpression();
        }

        const QString groupName = container->isGroupBox() ? container->name() : QString();
        std::shared_ptr<FormItem> formItemData =
          std::shared_ptr<FormItem>(
            FormItem::createFieldItem(
              fieldUuid,
              field,
              groupName,
              parentTabRow,
              layer->attributeDisplayName( fieldIndex ),
              nameExpression,
              editorField->showLabel(),
              !isReadOnly,
              isEditableExpression,
              getEditorWidgetSetup( layer, fieldIndex ),
              fieldIndex,
              parentVisibilityExpressions // field doesn't have visibility expression itself
            )
          );

        mFormItems[formItemData->id()] = formItemData;


        items.append( fieldUuid );
        break;
      }

      case Qgis::AttributeEditorType::Relation:
      {
        QUuid widgetUuid = QUuid::createUuid();

        QgsAttributeEditorRelation *relationField = static_cast<QgsAttributeEditorRelation *>( element );
        QgsRelation associatedRelation = relationField->relation();

        bool isValid = associatedRelation.isValid();
        if ( !isValid )
        {
          CoreUtils::log( "Relations", QStringLiteral( "Ignoring invalid relation in layer %1" ).arg( layer->name() ) );
          continue;
        }

        bool isNmRelation = layer->editFormConfig().widgetConfig( associatedRelation.id() )[QStringLiteral( "nm-rel" )].toBool();
        if ( isNmRelation )
        {
          CoreUtils::log( "Relations", QStringLiteral( "Nm relations are not supported in layer %1" ).arg( layer->name() ) );
          continue;
        }

        const QString groupName = container->isGroupBox() ? container->name() : QString();
        QString label = relationField->label();
        if ( label.isEmpty() )
        {
          label = associatedRelation.name();
          if ( label.isEmpty() ) // relation name can be empty
          {
            label = associatedRelation.referencingLayer()->name();
          }
        }

        std::shared_ptr<FormItem> formItemData =
          std::shared_ptr<FormItem>(
            FormItem::createRelationItem(
              widgetUuid,
              groupName,
              parentTabRow,
              label,
              relationField->showLabel(),
              parentVisibilityExpressions, // relation field doesn't have visibility expression itself
              associatedRelation
            )
          );

        mFormItems[formItemData->id()] = formItemData;
        items.append( widgetUuid );

        break;
      }
      case Qgis::AttributeEditorType::SpacerElement:
      {
        QUuid fieldUuid = QUuid::createUuid();

        QgsAttributeEditorSpacerElement *spacerElement = static_cast<QgsAttributeEditorSpacerElement *>( element );

        const QString groupName = container->isGroupBox() ? container->name() : QString();
        std::shared_ptr<FormItem> formItemData =
          std::shared_ptr<FormItem>(
            FormItem::createSpacerItem(
              fieldUuid,
              groupName,
              parentTabRow,
              spacerElement->name(),
              spacerElement->drawLine(),
              parentVisibilityExpressions // spacer doesn't have visibility expression itself
            )
          );

        mFormItems[formItemData->id()] = formItemData;


        items.append( fieldUuid );
        break;
      }
      case Qgis::AttributeEditorType::TextElement:
      {
        QUuid fieldUuid = QUuid::createUuid();

        QgsAttributeEditorTextElement *textElement = static_cast<QgsAttributeEditorTextElement *>( element );


        const QString groupName = container->isGroupBox() ? container->name() : QString();
        std::shared_ptr<FormItem> formItemData =
          std::shared_ptr<FormItem>(
            FormItem::createRichTextItem(
              fieldUuid,
              groupName,
              parentTabRow,
              textElement->name(),
              textElement->showLabel(),
              textElement->text(),
              false,
              parentVisibilityExpressions // text doesn't have visibility expression itself
            )
          );

        mFormItems[formItemData->id()] = formItemData;

        items.append( fieldUuid );
        break;
      }
      case Qgis::AttributeEditorType::HtmlElement:
      {
        QUuid fieldUuid = QUuid::createUuid();

        QgsAttributeEditorHtmlElement *htmlElement = static_cast<QgsAttributeEditorHtmlElement *>( element );


        const QString groupName = container->isGroupBox() ? container->name() : QString();
        std::shared_ptr<FormItem> formItemData =
          std::shared_ptr<FormItem>(
            FormItem::createRichTextItem(
              fieldUuid,
              groupName,
              parentTabRow,
              htmlElement->name(),
              htmlElement->showLabel(),
              htmlElement->htmlCode(),
              true,
              parentVisibilityExpressions // text doesn't have visibility expression itself
            )
          );

        mFormItems[formItemData->id()] = formItemData;

        items.append( fieldUuid );
        break;
      }

      default:
        // Invalid, Action, QmlElement
        // are not supported at the moment
        break;
    }
  }
}

void AttributeController::createTab( QgsAttributeEditorContainer *container )
{
  Q_ASSERT( container );

  int tabRow = mTabItems.size();

  QgsExpression expr;
  if ( container->visibilityExpression().enabled() )
  {
    expr = container->visibilityExpression().data();
  }

  QVector<QUuid> formItemsUuids;
  flatten( container, tabRow, QString(), formItemsUuids );

  std::shared_ptr<TabItem> tabItem( new TabItem( tabRow,
                                    container->name(),
                                    formItemsUuids,
                                    expr )
                                  );

  mTabItems.push_back( tabItem );
}

void AttributeController::clearAll()
{
  mAttributeFormProxyModelForTabItem.clear();
  mAttributeTabProxyModel.reset( new AttributeTabProxyModel() );
  setHasValidationErrors( false );
  mFormItems.clear();
  mTabItems.clear();
  mExpressionFieldsOutsideForm.clear();
  mVirtualFieldsOutsideForm.clear();
  mHasTabs = false;
}

void AttributeController::updateOnLayerChange()
{
  clearAll();
  QgsVectorLayer *layer = mFeatureLayerPair.layer();
  if ( !layer )
    return;

  // 1) DATA
  if ( layer->editFormConfig().layout() == Qgis::AttributeFormLayout::DragAndDrop )
  {
    QgsAttributeEditorContainer *root = layer->editFormConfig().invisibleRootContainer();
    if ( root->columnCount() > 1 )
    {
      qDebug() << "root tab in manual config has multiple columns. not supported on mobile devices!";
      root->setColumnCount( 1 );
    }

    mHasTabs = allowTabs( root );
    if ( mHasTabs )
    {
      for ( QgsAttributeEditorElement *element : root->children() )
      {
        if ( element->type() == Qgis::AttributeEditorType::Container )
        {
          QgsAttributeEditorContainer *container = static_cast<QgsAttributeEditorContainer *>( element );
          if ( container->columnCount() > 1 )
          {
            qDebug() << "tab " << container->name() << " in manual config has multiple columns. not supported on mobile devices!";
            container->setColumnCount( 1 );
          }
          createTab( container );
        }
      }
    }
    else
    {
      createTab( root );
    }
  }
  else
  {
    // Auto-Generated Layout
    // We create fake root tab
    QgsAttributeEditorContainer *tab = autoLayoutTabContainer();

    // We need to look for relations and include them into form,
    // in auto-generated layout they are not included in form config
    discoverRelations( tab );

    createTab( tab );
  }

  if ( mRememberAttributesController )
    mRememberAttributesController->storeLayerFields( layer );


  // 2) MODELS
  // for all other models, ownership is managed by Qt parent system
  AttributeTabModel *tabModel = new AttributeTabModel( mAttributeTabProxyModel.get(), this, mTabItems.size() );
  mAttributeTabProxyModel->setSourceModel( tabModel );
  mAttributeFormProxyModelForTabItem.resize( mTabItems.size() );
  QVector<std::shared_ptr<TabItem>>::iterator tabItemsIterator = mTabItems.begin();
  while ( tabItemsIterator != mTabItems.end() )
  {
    std::shared_ptr<TabItem> item = *tabItemsIterator;
    const QVector<QUuid> &formItems = item->formItems();

    AttributeFormProxyModel *proxyFormModel = new AttributeFormProxyModel( mAttributeTabProxyModel.get() );
    AttributeFormModel *formModel = new AttributeFormModel( mAttributeTabProxyModel.get(), this, formItems );
    proxyFormModel->setSourceModel( formModel );
    mAttributeFormProxyModelForTabItem[item->tabIndex()] = proxyFormModel;
    ++tabItemsIterator;
  }

  // collect fields which have default value expression and are not in the form
  QSet<int> fieldIndexes;
  QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
  while ( formItemsIterator != mFormItems.end() )
  {
    std::shared_ptr<FormItem> item = formItemsIterator.value();
    if ( item->type() == FormItem::Field )
    {
      fieldIndexes << item->fieldIndex();
    }

    ++formItemsIterator;
  }

  for ( int i = 0; i < mFeatureLayerPair.layer()->fields().count(); i++ )
  {
    if ( !fieldIndexes.contains( i ) )
    {
      QgsField f = mFeatureLayerPair.layer()->fields().at( i );
      const QgsDefaultValue defaultDefinition = f.defaultValueDefinition();

      if ( !defaultDefinition.expression().isEmpty() )
      {
        mExpressionFieldsOutsideForm << i;
      }

      if ( mFeatureLayerPair.layer()->fields().fieldOrigin( i ) == QgsFields::OriginExpression )
      {
        mVirtualFieldsOutsideForm << i;
      }
    }
  }

}

void AttributeController::updateOnFeatureChange()
{
  if ( !mFeatureLayerPair.layer() )
    return;

  const QgsFeature feature = mFeatureLayerPair.feature();

  QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
  while ( formItemsIterator != mFormItems.end() )
  {
    std::shared_ptr<FormItem> itemData = formItemsIterator.value();
    if ( itemData->type() == FormItem::Field )
    {
      int fieldIndex = itemData->fieldIndex();
      const QVariant newVal = feature.attribute( fieldIndex );
      mFormItems[itemData->id()]->setOriginalValue( newVal );
      mFormItems[itemData->id()]->setRawValue( newVal ); // we need to set raw value as well, as we use it in form now
      if ( mRememberAttributesController && isNewFeature() ) // this is a new feature
      {
        QVariant rememberedValue;
        bool shouldUseRememberedValue = mRememberAttributesController->rememberedValue(
                                          mFeatureLayerPair.layer(),
                                          fieldIndex,
                                          rememberedValue
                                        );
        if ( shouldUseRememberedValue )
        {
          mFeatureLayerPair.featureRef().setAttribute( fieldIndex, rememberedValue );
          itemData->setRawValue( rememberedValue );
        }
      }
    }
    ++formItemsIterator;
  }

  bool formValueChange = false;
  // if feature geometry was changed we also need recalculate defaults
  // as some attributes may contain expressions which use feature geometry
  if ( mFeatureLayerPair.layer()->isEditable() )
  {
    formValueChange = mFeatureLayerPair.layer()->editBuffer()->changedGeometries().contains( feature.id() );
  }
  recalculateDerivedItems( formValueChange, isNewFeature() );
}

bool AttributeController::isNewFeature() const
{
  QgsFeatureId id = mFeatureLayerPair.feature().id();
  return FID_IS_NEW( id ) || FID_IS_NULL( id );
}

void AttributeController::acquireId()
{
  if ( !mFeatureLayerPair.layer() )
    return;

  QgsFeature feat = mFeatureLayerPair.feature();
  bool featureIsNotYetAdded = FID_IS_NULL( feat.id() );

  startEditing();

  if ( featureIsNotYetAdded )
  {
    if ( !mFeatureLayerPair.layer()->addFeature( feat ) )
    {
      CoreUtils::log( QStringLiteral( "Attribute Controller" ), QStringLiteral( "Feature could not be added" ) );

    }
  }
  else
  {
    if ( !mFeatureLayerPair.layer()->updateFeature( feat ) )
    {
      CoreUtils::log( QStringLiteral( "Attribute Controller" ), QStringLiteral( "Cannot update feature" ) );
    }
  }


  connect( mFeatureLayerPair.layer(), &QgsVectorLayer::featureAdded, this, &AttributeController::onFeatureAdded );

  if ( !commit() )
  {
    emit commitFailed();
  }

  //
  // We need to trigger the recalucation of the form because QgsVectorLayer::featureAdded
  // is called before commit is finished. This can cause issues with evaluation of validations
  // when old feature with FID_IS_NEW is not yet removed from the layer and the new feature
  // with valid fid is already added (unique constraints are failing as it detects two features
  // with the same values). We thus manually trigger the recalculation here.
  //
  recalculateDerivedItems();

  disconnect( mFeatureLayerPair.layer(), &QgsVectorLayer::featureAdded, this, &AttributeController::onFeatureAdded );
}

void AttributeController::evaluateExpressionAndUpdateValue( QSet<QUuid> &changedFormItems,
    const QString &expressionString, QgsExpressionContext &expressionContext, int fieldIndex, const QgsField &field, std::shared_ptr<FormItem> formItem )
{
  QgsExpression exp( expressionString );

  exp.prepare( &expressionContext );
  if ( exp.hasParserError() )
  {
    QString msg( QStringLiteral( "Expression for %1:%2 has parser error: %3" ).arg(
                   mFeatureLayerPair.layer()->name(),
                   field.name(),
                   exp.parserErrorString() ) );
    CoreUtils::log( QStringLiteral( "Attribute Controller" ), msg );
    return;
  }

  QVariant value = exp.evaluate( &expressionContext );

  if ( exp.hasEvalError() )
  {
    QString msg( QStringLiteral( "Expression for %1:%2 has evaluation error: %3" ).arg(
                   mFeatureLayerPair.layer()->name(),
                   field.name(),
                   exp.evalErrorString() ) );
    CoreUtils::log( QStringLiteral( "Attribute Controller" ), msg );
    return;
  }

  QVariant val( value );
  if ( !field.convertCompatible( val ) )
  {
    QString msg( QStringLiteral( "Value \"%1\" %4 could not be converted to a compatible value for field %2(%3)." ).arg(
                   value.toString(),
                   field.name(),
                   field.typeName(),
                   value.isNull() ? "NULL" : "NOT NULL" )
               );
    CoreUtils::log( QStringLiteral( "Attribute Controller" ), msg );
    return;
  }

  QVariant oldVal = mFeatureLayerPair.feature().attribute( fieldIndex );

  if ( val != oldVal )
  {
    mFeatureLayerPair.featureRef().setAttribute( fieldIndex, val );

    // We need to update form items and emit signals for fields included in the form
    if ( formItem )
    {
      formItem->setRawValue( val );
      emit formDataChanged( formItem->id(), { AttributeFormModel::RawValue, AttributeFormModel::RawValueIsNull } );
      changedFormItems.insert( formItem->id() );
    }

    // Update also expression context after an attribute change
    expressionContext.setFeature( featureLayerPair().featureRef() );
  }
}

void AttributeController::recalculateDefaultValues(
  QSet<QUuid> &changedFormItems,
  QgsExpressionContext &expressionContext,
  bool isFormValueChange,
  bool isFirstUpdateOfNewFeature
)
{
  // update default values for fields which are not in the form
  for ( const int idx : mExpressionFieldsOutsideForm )
  {
    QgsField f = mFeatureLayerPair.layer()->fields().at( idx );
    const QgsDefaultValue defaultDefinition = f.defaultValueDefinition();

    bool shouldApplyDefaultValue =
      !defaultDefinition.expression().isEmpty() &&
      ( isFirstUpdateOfNewFeature || ( isFormValueChange && defaultDefinition.applyOnUpdate() ) );

    if ( shouldApplyDefaultValue )
    {
      evaluateExpressionAndUpdateValue( changedFormItems,
                                        defaultDefinition.expression(),
                                        expressionContext,
                                        idx,
                                        f,
                                        nullptr );
    }
  }

  // evaluate default values for fields in the form
  QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
  while ( formItemsIterator != mFormItems.end() )
  {
    std::shared_ptr<FormItem> item = formItemsIterator.value();

    const QgsField field = item->field();
    const QgsDefaultValue defaultDefinition = field.defaultValueDefinition();

    bool shouldApplyDefaultValue =
      !defaultDefinition.expression().isEmpty() &&
      ( isFirstUpdateOfNewFeature || ( isFormValueChange && defaultDefinition.applyOnUpdate() ) );

    if ( shouldApplyDefaultValue )
    {
      evaluateExpressionAndUpdateValue( changedFormItems,
                                        defaultDefinition.expression(),
                                        expressionContext,
                                        item->fieldIndex(),
                                        field,
                                        item );
    }

    if ( isFirstUpdateOfNewFeature )
    {
      // check if item is a slider, if so and it does not have default value,
      // set it's initial value from NULL to zero or minimum
      bool isSlider = item->editorWidgetType() == QStringLiteral( "Range" ) &&
                      item->editorWidgetConfig().value( QStringLiteral( "Style" ) ) == QStringLiteral( "Slider" );
      bool hasDefaultValueDefinition = !defaultDefinition.expression().isEmpty();

      if ( isSlider && !hasDefaultValueDefinition )
      {
        double min = item->editorWidgetConfig().value( "Min" ).toDouble();
        double max = item->editorWidgetConfig().value( "Max" ).toDouble();
        double valueToSet = 0;

        if ( !( min <= valueToSet && valueToSet <= max ) )
        {
          valueToSet = qMin( min, max );
        }

        mFeatureLayerPair.featureRef().setAttribute( item->fieldIndex(), valueToSet );
        item->setRawValue( valueToSet );
        emit formDataChanged( item->id(), { AttributeFormModel::RawValue } );
        changedFormItems.insert( item->id() );
      }
    }
    ++formItemsIterator;
  }
}

void AttributeController::recalculateVirtualFields( QSet<QUuid> &changedFormItems, QgsExpressionContext &expressionContext )
{
  // update default values for fields which are not in the form
  for ( const int idx : mVirtualFieldsOutsideForm )
  {
    QgsField f = mFeatureLayerPair.layer()->fields().at( idx );
    QString expressionString = mFeatureLayerPair.layer()->expressionField( idx );
    evaluateExpressionAndUpdateValue( changedFormItems,
                                      expressionString,
                                      expressionContext,
                                      idx,
                                      f,
                                      nullptr );
  }

  // evaluate virtual fields in the form
  QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
  while ( formItemsIterator != mFormItems.end() )
  {
    std::shared_ptr<FormItem> item = formItemsIterator.value();

    const QgsField field = item->field();

    if ( mFeatureLayerPair.layer()->fields().fieldOrigin( item->fieldIndex() ) == QgsFields::OriginExpression )
    {
      QString expressionString = mFeatureLayerPair.layer()->expressionField( item->fieldIndex() );
      evaluateExpressionAndUpdateValue( changedFormItems,
                                        expressionString,
                                        expressionContext,
                                        item->fieldIndex(),
                                        field,
                                        item );
    }
    ++formItemsIterator;
  }
}

void AttributeController::recalculateDerivedItems( bool isFormValueChange, bool isFirstUpdateOfNewFeature )
{
  QSet<QUuid> changedFormItems;

  QgsVectorLayer *layer = mFeatureLayerPair.layer();
  if ( !layer || !layer->isValid() )
    return;

  if ( !mFeatureLayerPair.feature().isValid() )
    return;

  // Create context
  QgsFields fields = mFeatureLayerPair.feature().fields();
  QgsExpressionContext expressionContext = layer->createExpressionContext();
  expressionContext << QgsExpressionContextUtils::formScope( mFeatureLayerPair.feature() );
  if ( mVariablesManager )
    expressionContext << mVariablesManager->positionScope();

  expressionContext.setFields( fields );
  expressionContext.setFeature( featureLayerPair().featureRef() );

  // Evaluate virtual fields
  recalculateVirtualFields( changedFormItems, expressionContext );

  // Evaluate default values
  recalculateDefaultValues( changedFormItems, expressionContext, isFormValueChange, isFirstUpdateOfNewFeature );


  // Evaluate HTML and Text element expressions
  recalculateRichTextWidgets( changedFormItems, expressionContext );

  // Evaluate tab items visiblity
  {
    QVector<std::shared_ptr<TabItem>>::iterator tabItemsIterator = mTabItems.begin();
    while ( tabItemsIterator != mTabItems.end() )
    {
      std::shared_ptr<TabItem> item = *tabItemsIterator;
      QgsExpression exp = item->visibilityExpression();
      exp.prepare( &expressionContext );
      bool visible = true;
      if ( exp.isValid() )
      {
        visible = exp.evaluate( &expressionContext ).toBool();
      }

      if ( item->isVisible() != visible )
      {
        item->setVisible( visible );
        emit tabDataChanged( item->tabIndex() );
      }
      ++tabItemsIterator;
    }
  }

  // Evaluate form items visibility
  {
    QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
    while ( formItemsIterator != mFormItems.end() )
    {
      std::shared_ptr<FormItem> item = formItemsIterator.value();
      bool visible = true;
      if ( item->editorWidgetType() == QLatin1String( "Hidden" ) )
      {
        visible = false;
      }
      else
      {
        QgsExpression exp = item->visibilityExpression();
        exp.prepare( &expressionContext );

        if ( exp.isValid() )
          visible = exp.evaluate( &expressionContext ).toInt();
      }

      if ( item->visible() != visible )
      {
        item->setVisible( visible );
        changedFormItems << item->id();
      }
      ++formItemsIterator;
    }
  }

  // Evaluate if form items are editable
  {
    QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
    while ( formItemsIterator != mFormItems.end() )
    {
      std::shared_ptr<FormItem> item = formItemsIterator.value();
      QgsExpression exp = item->editableExpression();

      if ( !exp.expression().isEmpty() )
      {
        bool editable = item->isEditable();
        exp.prepare( &expressionContext );

        if ( exp.isValid() )
        {
          editable = exp.evaluate( &expressionContext ).toBool();
        }

        if ( item->isEditable() != editable )
        {
          item->setIsEditable( editable );
          changedFormItems << item->id();
        }
      }

      ++formItemsIterator;
    }
  }

  // Evaluate form items name
  {
    QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
    while ( formItemsIterator != mFormItems.end() )
    {
      std::shared_ptr<FormItem> item = formItemsIterator.value();
      QgsExpression exp = item->nameExpression();

      if ( !exp.expression().isEmpty() )
      {
        QString name = item->name();
        exp.prepare( &expressionContext );

        if ( exp.isValid() )
        {
          name = exp.evaluate( &expressionContext ).toString();
        }

        if ( item->name() != name )
        {
          item->setName( name );
          changedFormItems << item->id();
        }
      }

      ++formItemsIterator;
    }
  }

  // Evaluate form items value state - hard/soft constraints, value validity
  {
    bool containsValidationError = false;
    {
      QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
      while ( formItemsIterator != mFormItems.end() )
      {
        std::shared_ptr<FormItem> item = formItemsIterator.value();
        if ( item->type() == FormItem::Field )
        {
          QString validationMessage;
          FieldValidator::ValidationStatus validationStatus;

          validationStatus = FieldValidator::validate( featureLayerPair(), *item, validationMessage );

          if ( validationStatus == FieldValidator::Error )
          {
            containsValidationError = true;
          }

          if ( validationMessage != item->validationMessage() )
          {
            item->setValidationStatus( validationStatus );
            item->setValidationMessage( validationMessage );
            changedFormItems.insert( item->id() );
          }
        }
        ++formItemsIterator;
      }
    }
    setHasValidationErrors( containsValidationError );
  }

  // Check if we have any changes
  bool anyChanges = isNewFeature();
  if ( !anyChanges )
  {
    QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
    while ( formItemsIterator != mFormItems.end() )
    {
      std::shared_ptr<FormItem> item = formItemsIterator.value();
      if ( item->type() == FormItem::Field )
      {
        if ( item->originalValue() != mFeatureLayerPair.feature().attribute( item->fieldIndex() ) )
        {
          anyChanges = true;
          break;
        }
      }

      ++formItemsIterator;
    }
  }
  setHasAnyChanges( anyChanges );

  // Emit all signals
  QSet<QUuid>::const_iterator i = changedFormItems.constBegin();
  while ( i != changedFormItems.constEnd() )
  {
    emit formDataChanged( *i );
    ++i;
  }

  if ( isFormValueChange )
    emit formRecalculated();
}

void AttributeController::recalculateRichTextWidgets( QSet<QUuid> &changedFormItems, QgsExpressionContext &context )
{
  QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
  while ( formItemsIterator != mFormItems.end() )
  {
    std::shared_ptr<FormItem> itemData = formItemsIterator.value();
    if ( itemData->type() == FormItem::RichText )
    {
      QString newValue;
      QString definition = itemData->editorWidgetConfig().value( QStringLiteral( "Definition" ) ).toString();
      bool isHTML = itemData->editorWidgetConfig().value( QStringLiteral( "UseHtml" ) ).toBool();
      if ( isHTML )
      {
        // evaluate texts like: <script>document.write(expression.evaluate("\TextField\""));</script>

        // QML Text does not support document.write, so just remove it
        const thread_local QRegularExpression sRegEx1( "<script>\\s*document\\.write\\(\\s*(.*)\\s*\\)\\s*;\\s*</script>", QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption );
        QRegularExpressionMatch match1 = sRegEx1.match( definition );
        while ( match1.hasMatch() )
        {
          QString expression = match1.captured( 1 );
          definition = QStringLiteral( "<span>%1</span>" ).arg( definition.mid( 0, match1.capturedStart( 0 ) ) + expression + definition.mid( match1.capturedEnd( 0 ) ) );
          match1 = sRegEx1.match( definition );
        }

        // Not evaluate expression with the engine
        const thread_local QRegularExpression sRegEx( "expression\\.evaluate\\(\\s*\\\"(.*?[^\\\\])\\\"\\s*\\)", QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption );
        QRegularExpressionMatch match = sRegEx.match( definition );
        while ( match.hasMatch() )
        {
          QString expression = match.captured( 1 );
          expression = expression.replace( QStringLiteral( "\\\"" ), QStringLiteral( "\"" ) );

          QgsExpression exp = QgsExpression( expression );
          exp.prepare( &context );
          QString resultString = exp.evaluate( &context ).toString();
          definition = definition.mid( 0, match.capturedStart( 0 ) ) + resultString + definition.mid( match.capturedEnd( 0 ) );
          match = sRegEx.match( definition );
        }
        newValue = definition;
      }
      else
      {
        newValue = QgsExpression::replaceExpressionText( definition, &context );
      }
      if ( itemData->rawValue() != newValue )
      {
        changedFormItems.insert( itemData->id() );
        itemData->setRawValue( newValue );
      }
    }
    ++formItemsIterator;
  }

}

bool AttributeController::hasValidationErrors() const
{
  return mHasValidationErrors;
}

bool AttributeController::hasTabs() const
{
  return mHasTabs;
}

AttributeTabProxyModel *AttributeController::attributeTabProxyModel() const
{
  Q_ASSERT( mAttributeTabProxyModel );
  return mAttributeTabProxyModel.get();
}

int AttributeController::tabCount() const
{
  return mTabItems.size();
}

AttributeFormProxyModel *AttributeController::attributeFormProxyModelForTab( int tabRow ) const
{
  if ( isValidTabId( tabRow ) )
  {
    return mAttributeFormProxyModelForTabItem[tabRow];
  }
  else
  {
    return nullptr;
  }
}

bool AttributeController::deleteFeature()
{
  if ( !mFeatureLayerPair.layer() )
    return false;

  if ( !startEditing() )
  {
    return false;
  }

  bool featureIsNotYetAdded = FID_IS_NULL( mFeatureLayerPair.feature().id() );
  bool isDeleted = true;
  bool rv = true;

  if ( featureIsNotYetAdded )
  {
    rv = mFeatureLayerPair.layer()->rollBack();
  }
  else
  {
    isDeleted = mFeatureLayerPair.layer()->deleteFeature( mFeatureLayerPair.feature().id() );
    rv = commit();
  }

  if ( !isDeleted || !rv )
  {
    CoreUtils::log( QStringLiteral( "Attribute Controller" ), QStringLiteral( "Cannot delete feature" ) );
    emit commitFailed();
  }
  else
  {
    mFeatureLayerPair = FeatureLayerPair();
    emit featureLayerPairChanged();
    emit changesCommited();
  }

  return rv;
}

bool AttributeController::rollback()
{
  if ( !mFeatureLayerPair.layer() )
    return false;

  if ( !mFeatureLayerPair.layer()->isEditable() )
  {
    return false;
  }

  if ( !mFeatureLayerPair.layer()->rollBack() )
  {
    CoreUtils::log( QStringLiteral( "Attribute Controller" ), QStringLiteral( "Could not rollback the changes in form" ) );
  }

  mFeatureLayerPair.layer()->triggerRepaint();
  return true;
}

bool AttributeController::save()
{
  if ( !mFeatureLayerPair.layer() )
    return false;

  renamePhotos();

  if ( !startEditing() )
  {
    return false;
  }

  bool rv = true;

  QgsFeature feat = mFeatureLayerPair.feature();

  bool featureIsNotYetAdded = FID_IS_NULL( feat.id() );

  if ( featureIsNotYetAdded )
  {
    if ( !mFeatureLayerPair.layer()->addFeature( feat ) )
    {
      CoreUtils::log( QStringLiteral( "Attribute Controller" ), QStringLiteral( "Feature could not be added" ) );
    }
    connect( mFeatureLayerPair.layer(), &QgsVectorLayer::featureAdded, this, &AttributeController::onFeatureAdded );
  }
  else
  {
    // update it instead of adding
    if ( !mFeatureLayerPair.layer()->updateFeature( feat, true ) )
    {
      CoreUtils::log( QStringLiteral( "Attribute Controller" ), QStringLiteral( "Cannot update feature" ) );
    }
  }

  bool featureIsNew = isNewFeature();

  // This calls lower-level I/O functions which shouldn't be used
  // in a Q_INVOKABLE because they can make the UI unresponsive.
  rv = commit();

  if ( rv )
  {
    emit changesCommited();
  }
  else
  {
    emit commitFailed();
  }

  if ( featureIsNotYetAdded )
  {
    disconnect( mFeatureLayerPair.layer(), &QgsVectorLayer::featureAdded, this, &AttributeController::onFeatureAdded );
  }

  // Store the feature attributes for future use
  if ( featureIsNew && mRememberAttributesController )
  {
    mRememberAttributesController->storeFeature( mFeatureLayerPair );
  }

  return rv;
}

bool AttributeController::startEditing()
{
  Q_ASSERT( mFeatureLayerPair.layer() );

  // Already an edit session active
  if ( mFeatureLayerPair.layer()->editBuffer() )
    return true;

  if ( !mFeatureLayerPair.layer()->startEditing() )
  {
    CoreUtils::log( QStringLiteral( "Attribute Controller" ), QStringLiteral( "Cannot start editing" ) );
    return false;
  }
  else
  {
    return true;
  }
}

bool AttributeController::commit()
{
  Q_ASSERT( mFeatureLayerPair.layer() );


  if ( !mFeatureLayerPair.layer()->commitChanges() )
  {
    CoreUtils::log( QStringLiteral( "CommitChanges" ),
                    QStringLiteral( "Failed to commit changes:\n%1" )
                    .arg( mFeatureLayerPair.layer()->commitErrors().join( QLatin1Char( '\n' ) ) ) );
    mFeatureLayerPair.layer()->rollBack();
    return false;
  }
  else
  {
    mFeatureLayerPair.layer()->triggerRepaint();
    return true;
  }
}

bool AttributeController::hasAnyChanges() const
{
  return mHasAnyChanges;
}

void AttributeController::setHasAnyChanges( bool hasChanges )
{
  if ( hasChanges != mHasAnyChanges )
  {
    mHasAnyChanges = hasChanges;
    emit hasAnyChangesChanged();
  }
}

void AttributeController::setHasValidationErrors( bool hasErrors )
{
  if ( mHasValidationErrors != hasErrors )
  {
    mHasValidationErrors = hasErrors;
    emit hasValidationErrorsChanged();
  }
}

void AttributeController::discoverRelations( QgsAttributeEditorContainer *container )
{
  QgsRelationManager *rManager = QgsProject::instance()->relationManager();

  if ( !rManager || !mFeatureLayerPair.layer() )
    return;

  // find relation references (add relation reference widgets)
  const QList<QgsRelation> childRelations = rManager->referencingRelations( mFeatureLayerPair.layer() );

  for ( const QgsRelation &relation : childRelations )
  {
    for ( QgsAttributeEditorElement *child : container->children() )
    {
      if ( child->name() == relation.fieldPairs().at( 0 ).first ) // we are using only one fieldPair!
      {
        QgsAttributeEditorField *editorField = static_cast<QgsAttributeEditorField *>( child );

        if ( !editorField )
          continue;

        int fieldIdx = editorField->idx();
        QgsField field = mFeatureLayerPair.layer()->fields().at( fieldIdx );

        QVariantMap config = mFeatureLayerPair.layer()->editorWidgetSetup( fieldIdx ).config();

        // relation reference fields in autogenerated fields might have an empty config, we need to set needed properties here
        if ( config.isEmpty() )
        {
          QVariantMap neededArgs =
          {
            { QStringLiteral( "ReferencedLayerId" ), relation.referencedLayerId() },
            { QStringLiteral( "Relation" ), relation.id() }
          };

          const QgsEditorWidgetSetup newWidgetSetup = InputUtils::getEditorWidgetSetup( field, QStringLiteral( "RelationReference" ), neededArgs );
          mFeatureLayerPair.layer()->setEditorWidgetSetup( fieldIdx, newWidgetSetup );

          CoreUtils::log( QStringLiteral( "DiscoverRelations" ), QStringLiteral( "Changed field %1 to RelationReference" ).arg( field.name() ) );
        }
        else
        {
          config.insert( QStringLiteral( "Relation" ), relation.id() );

          const QgsEditorWidgetSetup newWidgetSetup = QgsEditorWidgetSetup( QStringLiteral( "RelationReference" ), config );
          mFeatureLayerPair.layer()->setEditorWidgetSetup( fieldIdx, newWidgetSetup );
        }
      }
    }
  }

  // find relations (add relation widgets)
  const QList<QgsRelation> referencingRelations = rManager->referencedRelations( mFeatureLayerPair.layer() );

  for ( const QgsRelation &relation : referencingRelations )
  {
    std::unique_ptr<QgsAttributeEditorRelation> relationEditor = std::unique_ptr<QgsAttributeEditorRelation>( new QgsAttributeEditorRelation( relation, container ) );
    if ( relationEditor->label().isEmpty() && relation.name().isEmpty() )
    {
      // relation does not have a name nor field have label, set label based on child layer
      QString label = relation.referencingLayer()->name();
      relationEditor->setLabel( label );
    }
    container->addChildElement( relationEditor.release() );
  }
}

bool AttributeController::isValidFormId( const QUuid &id ) const
{
  return mFormItems.contains( id );
}

bool AttributeController::isValidTabId( int id ) const
{
  return ( id >= 0 ) && ( id < tabCount() );
}

const FormItem *AttributeController::formItem( const QUuid &id ) const
{
  if ( isValidFormId( id ) )
    return mFormItems[id].get();
  else
    return nullptr;
}

const TabItem *AttributeController::tabItem( int tabRow ) const
{
  if ( isValidTabId( tabRow ) )
    return mTabItems[tabRow].get();
  else
    return nullptr;
}

bool AttributeController::setFormShouldRememberValue( const QUuid &id, bool shouldRememberValue )
{
  if ( !mRememberAttributesController )
    return true; //noop

  if ( isValidFormId( id ) )
  {
    std::shared_ptr<FormItem> data = mFormItems[id];
    bool changed = mRememberAttributesController->setShouldRememberValue( mFeatureLayerPair.layer(), data->fieldIndex(), shouldRememberValue );
    if ( changed )
    {
      emit formDataChanged( id ); // It _should_ be enough to emit only the RememberValue role here, not all of them
    }
    return true;
  }
  else
  {
    return false;
  }
}

bool AttributeController::formShouldRememberValue( int fieldIndex ) const
{
  const QgsFeature feat = mFeatureLayerPair.feature();
  const QgsVectorLayer *layer = mFeatureLayerPair.layer();

  if ( !mRememberAttributesController )
    return false;

  return mRememberAttributesController->shouldRememberValue( layer, fieldIndex );
}

bool AttributeController::setFormValue( const QUuid &id, QVariant value )
{
  if ( isValidFormId( id ) )
  {
    std::shared_ptr<FormItem> item = mFormItems[id];
    QgsField field = item->field();
    QVariant val( value );

    item->setRawValue( val );
    emit formDataChanged( item->id(), { AttributeFormModel::RawValue } );

    if ( !field.convertCompatible( val ) )
    {
      QString msg( QStringLiteral( "Value \"%1\" %4 could not be converted to a compatible value for field %2(%3)." ).arg( value.toString(), field.name(), field.typeName(), value.isNull() ? "NULL" : "NOT NULL" ) );
      CoreUtils::log( QStringLiteral( "Attribute Controller" ), msg );
    }
    else
    {
      mFeatureLayerPair.featureRef().setAttribute( item->fieldIndex(), val );
      emit formDataChanged( item->id(), { AttributeFormModel::AttributeValue, AttributeFormModel::RawValueIsNull } );
    }
    recalculateDerivedItems( true, false );
    return true;
  }
  else
  {
    return false;
  }
}

QVariant AttributeController::formValue( int fieldIndex ) const
{
  const QgsFeature feat = mFeatureLayerPair.feature();
  if ( !feat.isValid() ||
       fieldIndex < 0 ||
       fieldIndex >= feat.attributeCount()
     )
    return QVariant();

  return mFeatureLayerPair.feature().attribute( fieldIndex );
}

AttributeController *AttributeController::parentController() const
{
  return mParentController;
}

void AttributeController::setParentController( AttributeController *newParentController )
{
  if ( !newParentController || mParentController == newParentController )
    return;

  mParentController = newParentController;
  emit parentControllerChanged();

  prefillRelationReferenceField();

  if ( mParentController )
  {
    // When parent's feature Id is changed, we want to update the relation reference field
    connect( mParentController, &AttributeController::featureIdChanged, this, &AttributeController::prefillRelationReferenceField );
  }
}

const QgsRelation &AttributeController::linkedRelation() const
{
  return mLinkedRelation;
}

void AttributeController::setLinkedRelation( const QgsRelation &newLinkedRelation )
{
  if ( mLinkedRelation.id() == newLinkedRelation.id() )
    return;

  mLinkedRelation = newLinkedRelation;
  emit linkedRelationChanged();

  prefillRelationReferenceField();
}

void AttributeController::onFeatureAdded( QgsFeatureId newFeatureId )
{
  QgsFeature f = mFeatureLayerPair.layer()->getFeature( newFeatureId );
  setFeatureLayerPair( FeatureLayerPair( f, mFeatureLayerPair.layer() ) );
  emit featureIdChanged();
}

void AttributeController::renamePhotos()
{
  const QStringList photoNameFormat = QgsProject::instance()->entryList( QStringLiteral( "Mergin" ), QStringLiteral( "PhotoNaming/%1" ).arg( mFeatureLayerPair.layer()->id() ) );
  if ( photoNameFormat.isEmpty() )
  {
    return;
  }

  QgsExpressionContext expressionContext = mFeatureLayerPair.layer()->createExpressionContext();
  expressionContext << QgsExpressionContextUtils::formScope( mFeatureLayerPair.feature() );
  if ( mVariablesManager )
    expressionContext << mVariablesManager->positionScope();

  expressionContext.setFields( mFeatureLayerPair.feature().fields() );
  expressionContext.setFeature( mFeatureLayerPair.featureRef() );

  // check for new photos
  QMap<QUuid, std::shared_ptr<FormItem>>::iterator formItemsIterator = mFormItems.begin();
  while ( formItemsIterator != mFormItems.end() )
  {
    std::shared_ptr<FormItem> item = formItemsIterator.value();
    if ( item->type() == FormItem::Field && item->editorWidgetType() == QStringLiteral( "ExternalResource" ) )
    {
      QVariantMap config = item->editorWidgetConfig();
      const QgsField field = item->field();
      if ( !photoNameFormat.contains( field.name() ) )
      {
        ++formItemsIterator;
        continue;
      }

      if ( item->originalValue() != mFeatureLayerPair.feature().attribute( item->fieldIndex() ) )
      {
        QString expString = QgsProject::instance()->readEntry( QStringLiteral( "Mergin" ), QStringLiteral( "PhotoNaming/%1/%2" ).arg( mFeatureLayerPair.layer()->id() ).arg( field.name() ) );
        QgsExpression exp( expString );
        exp.prepare( &expressionContext );
        if ( exp.hasParserError() )
        {
          CoreUtils::log( QStringLiteral( "Photo name format" ), QStringLiteral( "Expression for %1:%2 has parser error: %3" ).arg( mFeatureLayerPair.layer()->name() ).arg( field.name() ).arg( exp.parserErrorString() ) );
          ++formItemsIterator;
          continue;
        }

        QVariant value = exp.evaluate( &expressionContext );
        if ( exp.hasEvalError() )
        {
          CoreUtils::log( QStringLiteral( "Photo name format" ), QStringLiteral( "Expression for %1:%2 has evaluation error: %3" ).arg( mFeatureLayerPair.layer()->name() ).arg( field.name() ).arg( exp.evalErrorString() ) );
          ++formItemsIterator;
          continue;
        }

        QVariant val( value );
        if ( !field.convertCompatible( val ) )
        {
          CoreUtils::log( QStringLiteral( "Photo name format" ), QStringLiteral( "Value \"%1\" %4 could not be converted to a compatible value for field %2 (%3)." ).arg( value.toString() ).arg( field.name() ).arg( field.typeName() ).arg( value.isNull() ? "NULL" : "NOT NULL" ) );
          ++formItemsIterator;
          continue;
        }

        const QString targetDir = InputUtils::resolveTargetDir( QgsProject::instance()->homePath(), config, mFeatureLayerPair, QgsProject::instance() );
        const QString prefix = InputUtils::resolvePrefixForRelativePath( config[ QStringLiteral( "RelativeStorage" ) ].toInt(), QgsProject::instance()->homePath(), targetDir );
        const QString src = InputUtils::getAbsolutePath( mFeatureLayerPair.feature().attribute( item->fieldIndex() ).toString(), prefix );
        QFileInfo fi( src );
        QString newName = QStringLiteral( "%1.%2" ).arg( val.toString() ).arg( fi.completeSuffix() );

        // Remove leading slashes from newName following issue #3415
        const QRegularExpression leadingSlashes( "^/+" );
        newName.remove( leadingSlashes );

        const QString dst = InputUtils::getAbsolutePath( newName, prefix );
        if ( InputUtils::renameFile( src, dst ) )
        {
          mFeatureLayerPair.featureRef().setAttribute( item->fieldIndex(), newName );
          expressionContext.setFeature( featureLayerPair().featureRef() );
        }
        else
        {
          CoreUtils::log( QStringLiteral( "Photo name format" ), QStringLiteral( "Could not rename file from %1 to %2" ).arg( src ).arg( dst ) );
        }
      }
    }

    ++formItemsIterator;
  }
}
