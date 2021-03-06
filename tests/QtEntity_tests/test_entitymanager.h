#include <QtTest/QtTest>
#include <QtCore/QObject>
#include <QtEntity/EntityManager>
#include <QtEntity/SimpleEntitySystem>
#include "common.h"

using namespace QtEntity;


class EntityManagerTest: public QObject
{
    Q_OBJECT
private slots:

    void getEntitySystemByComponent()
    {
        EntityManager em;

        EntitySystem* esnothing = em.system(qMetaTypeId<Testing>());
        QVERIFY(esnothing == nullptr);

        auto es = new TestingSystem(&em);
        EntitySystem* es2 = em.system(qMetaTypeId<Testing>());
        QVERIFY(es == es2);
    }  

    void removeSystem()
    {
        EntityManager em;

        auto es = new TestingSystem(&em);
        em.removeSystem(es);
        EntitySystem* es2 = em.system(qMetaTypeId<Testing>());
        QVERIFY(nullptr == es2);
        delete es;
    }


    void getComponent()
    {
        EntityManager em;
        new TestingSystem(&em);
        Testing *comp, *comp2;
        em.createComponent(1, comp);

        bool success = em.component(1, comp2);
        QVERIFY(success);
        QVERIFY(comp == comp2);
    }

    void getComponent2()
    {
        EntityManager em;
        new TestingSystem(&em);
        Testing *comp, *comp2;
        em.createComponent(1, comp);

        comp2 = em.component<Testing>(1);
        QVERIFY(comp == comp2);
    }

    void createComponent2()
    {
        EntityManager em;
        new TestingSystem(&em);
        auto c1 = em.createComponent<Testing>(1);

        Testing* c2;
        bool success = em.component(1, c2);
        QVERIFY(success);
        QVERIFY(c1 == c2);
    }

    void destroyComponent()
    {
        EntityManager em;
        new TestingSystem(&em);
        Testing *comp, *comp2;
        em.createComponent(1, comp);
        em.createComponent(2, comp2);
        em.destroyComponent<Testing>(1);

        bool success = em.component(1, comp);
        QVERIFY(!success);
        success = em.component(2, comp2);
        QVERIFY(success);
    }


    void destroyEntity()
    {
        EntityManager em;
        new TestingSystem(&em);
        Testing *comp;
        em.createComponent(1, comp);
        em.destroyEntity(1);
        bool success = em.component(1, comp);
        QVERIFY(!success);
        
    }


    void createEntityId()
    {
        EntityManager em;
        EntityId eid = em.createEntityId();
        QVERIFY(1 == eid);
        EntityId eid2 = em.createEntityId();
        QVERIFY(2 == eid2);
    }


};
