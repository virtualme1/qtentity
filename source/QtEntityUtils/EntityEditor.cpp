#include <QtEntityUtils/EntityEditor>
#include <QtEntityUtils/VariantFactory>
#include <QtEntityUtils/VariantManager>
#include <QtEntityUtils/PropertyObjectsEdit>
#include <QtPropertyBrowser/QtTreePropertyBrowser>
#include <QtEntity/EntityManager>
#include <QtEntity/MetaObjectRegistry>
#include <QDate>
#include <QLocale>
#include <QHBoxLayout>
#include <QMetaProperty>
#include <QUuid>

namespace QtEntityUtils
{
    EntityEditor::EntityEditor()
        : _entityId(0)
        , _propertyManager(new VariantManager(this))
        , _editor(new QtTreePropertyBrowser())
    {

        QtVariantEditorFactory* variantFactory = new VariantFactory();

        _editor->setFactoryForManager(_propertyManager, variantFactory);
        _editor->setPropertiesWithoutValueMarked(true);
        _editor->setRootIsDecorated(false);
        setLayout(new QHBoxLayout());
        layout()->addWidget(_editor);

        connect(_propertyManager, &VariantManager::valueChanged, this, &EntityEditor::propertyValueChanged);
    }


    void EntityEditor::displayEntity(QtEntity::EntityId id, const QVariant& data)
    {
        _entityId = id;
        clear();

        if(!data.canConvert<QVariantMap>()) return;
        
        QVariantMap components = data.value<QVariantMap>();
        for(auto i = components.begin(); i != components.end(); ++i)
        {
            QString componenttype = i.key();
            QtProperty* item = _propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), componenttype);
            _editor->addProperty(item);
            QVariant variant = i.value();
            if(!variant.canConvert<QVariantMap>()) continue;
            QVariantMap props = variant.value<QVariantMap>();
            for(auto j = props.begin(); j != props.end(); ++j)
            {
                QString propname = j.key();

                // skip property attributes
                if(propname.left(2) == "#|") continue;
                QVariant propval = j.value();

                int t = propval.userType();
                
                // qvariant has no float type :(

                if(t == (int)QMetaType::Float)
                {
                    t = QVariant::Double;
                }

                QtVariantProperty* propitem = _propertyManager->addProperty(t, propname);
				if(propitem)
				{
					propitem->setValue(propval);

					// fetch property attributes
					auto i = props.find(QString("#|%1").arg(propname));
					if(i != props.end())
					{
						QVariantMap attrs = i.value().value<QVariantMap>();
						for(auto j = attrs.begin(); j != attrs.end(); ++j)
						{
							propitem->setAttribute(j.key(), j.value());
						}
					}

					item->addSubProperty(propitem);
				}
				else
				{
					qDebug() << "Could not create property editor for property " << propname;
				}                
            }
        }
    }


    void EntityEditor::clear()
    {
        _propertyManager->clear();
        _editor->clear();
    }


    QVariant EntityEditor::fetchEntityData(const QtEntity::EntityManager& em, QtEntity::EntityId eid)
    {
        QVariantMap components;
        for(auto i = em.begin(); i != em.end(); ++i)
        {
            QtEntity::EntitySystem* es = *i;
            QObject* component = es->getComponent(eid);
            if(component == nullptr) continue;

            const QMetaObject& meta = es->componentMetaObject();

            // store property name-value pairs
            QVariantMap componentvals = PropertyObjectEditor::componentToVariantMap(*component);

            for(int j = 0; j < meta.propertyCount(); ++j)
            {
                QMetaProperty prop = meta.property(j);
				const char* name = prop.name();               

                // store property constraints. Prepend a "#|" to identify
                QVariantMap constraints = es->attributesForProperty(name);
                if(!constraints.empty())
                {
                    componentvals[QString("#|%1").arg(name)] = constraints;
                }

            }
            components[es->componentMetaObject().className()] = componentvals;

        }
        return components;
    }


    void EntityEditor::applyEntityData(QtEntity::EntityManager& em, QtEntity::EntityId eid, const QString& componenttype, const QString& propertyname, const QVariant& value)
    {
        QtEntity::EntitySystem* es = em.getSystemByComponentClassName(componenttype);
        if(es == nullptr)
        {
            qDebug() << "Could not apply entity data, no entity system of type " << componenttype;
            return;
        }
        
        QObject* component = es->getComponent(eid);
        if(!component) 
        {
            qDebug() << "Could not apply entity data, no component of type " << componenttype << " on entity " << eid;
            return;
        }

        const QMetaObject& meta = es->componentMetaObject();

        bool foundprop = false;
        QMetaProperty prop;
        for(int j = 0; j < meta.propertyCount(); ++j)
        {
            if(meta.property(j).name() == propertyname) 
            {
                foundprop = true;
                prop = meta.property(j);
                break;
            }
        }

        if(!foundprop)
        {
            qDebug() << "No property named " << propertyname << " on object of type " << meta.className();
            return;
        }

        if(prop.userType() == VariantManager::propertyObjectsId())
        {
            QVariantList vars = value.value<QVariantList>();
            QtEntity::PropertyObjects objs = prop.read(component).value<QtEntity::PropertyObjects>();
            QtEntity::PropertyObjects objsnew = PropertyObjectEditor::updatePropertyObjectsFromVariantList(objs, vars);
            prop.write(component, QVariant::fromValue(objsnew));
        }
        else
        {
            prop.write(component, value);
        }
    }


    QString getComponentNameForProperty(QtAbstractPropertyManager* manager, QtProperty *property)
    {
        QSet<QtProperty*> comps = manager->properties();
        foreach(auto comp, comps)
        {
            
			QString tmpname= comp->propertyName();
            foreach(auto prop, comp->subProperties())
            {
				Q_ASSERT(prop != NULL);
				QString tmpname2 = prop->propertyName();
                if(prop == property)
                {
                     return comp->propertyName();
                }
            }
        }
        return "";
    }


    void EntityEditor::propertyValueChanged(QtProperty *property, const QVariant &val)
    {
        QString componentName = getComponentNameForProperty(_propertyManager, property);
        if(!componentName.isEmpty())
        {
            emit entityDataChanged(_entityId, componentName, property->propertyName(), val);
        }
    }
}
