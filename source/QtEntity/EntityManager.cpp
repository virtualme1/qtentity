/*
Copyright (c) 2013 Martin Scheffler
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <QtEntity/EntityManager>

#include <QtEntity/EntitySystem>
#include <QDebug>

namespace QtEntity
{

	EntityManager::EntityManager(QObject* parent)
		: QObject(parent)
        , _entityCounter(1)
	{
	}


	EntityManager::~EntityManager()
	{
	}


    EntityId EntityManager::createEntityId()
    {
        EntityId eid = _entityCounter.fetchAndAddRelaxed(1);
        return eid;
    }

    
    void EntityManager::destroyEntity(EntityId id)
    {
        for(auto i = _systems.begin(); i != _systems.end(); ++i)
        {
            i->second->destroyComponent(id);
        }
    }


    void EntityManager::addSystem(int mid, EntitySystem* es)
    {
        _systems[mid] = es;
    }


    bool EntityManager::hasSystem(int ctype)
    {
        return _systems.find(ctype) != _systems.end();
    }


    bool EntityManager::removeSystem(EntitySystem* es)
    {
        // assert that system is in both maps
        auto j = _systems.find(es->componentType());
        Q_ASSERT(j != _systems.end());
        Q_ASSERT(es == j->second);
        _systems.erase(j);
        es->setParent(nullptr);
        return true;
    }


    EntitySystem* EntityManager::system(const QString& classname) const
    {
        int metatype = QMetaType::type(classname.toLocal8Bit());
        return system(metatype);
    }


    EntitySystem* EntityManager::system(int metatypeid) const
    {        
        auto i = _systems.find(metatypeid);
        if(i == _systems.end()) return nullptr;
        Q_ASSERT(i->second->componentType() == metatypeid);
        return i->second;
    }


    void* EntityManager::component(EntityId id, int tid) const
    {
        EntitySystem* s = this->system(tid);
        return (s == nullptr) ? nullptr : s->component(id);
    }


    void* EntityManager::createComponent(EntityId id, int cid, const QVariantMap& properties)
    {
        EntitySystem* s = this->system(cid);

        if(s == nullptr) return nullptr;

        if(s->component(id) != nullptr)
        {
            qDebug() << "Component already exists! ComponentType:" << s->componentName() << " EntityId " << id;
            return nullptr;
        }

        try
        {
            return s->createComponent(id, properties);
        }
        catch(std::bad_alloc&)
        {
            qCritical() << "Could not create component, bad allocation!";
            return nullptr;
        }
    }


    bool EntityManager::destroyComponent(EntityId id, int cid)
    {
        EntitySystem* s = this->system(cid);
        if(s == nullptr) return false;
        return s->destroyComponent(id);
    }

}
