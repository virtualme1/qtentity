/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QTPROPERTYBROWSER_H
#define QTPROPERTYBROWSER_H
#include <QtPropertyBrowser/Export>
#include <QIcon>
#include <QWidget>
#include <QSet>
#include <QLineEdit>
#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif



typedef QLineEdit::EchoMode EchoMode;

class QtAbstractPropertyManager;
class QtPropertyPrivate;

class QTPROPERTYBROWSER_EXPORT QtProperty
{
public:
    virtual ~QtProperty();

    QList<QtProperty *> subProperties() const;

    QtAbstractPropertyManager *propertyManager() const;

    QString toolTip() const;
    QString statusTip() const;
    QString whatsThis() const;
    QString propertyName() const;
    bool isEnabled() const;
    bool isModified() const;

    bool hasValue() const;
    QIcon valueIcon() const;
    QString valueText() const;
    QString displayText() const;

    void setToolTip(const QString &text);
    void setStatusTip(const QString &text);
    void setWhatsThis(const QString &text);
    void setPropertyName(const QString &text);
    void setEnabled(bool enable);
    void setModified(bool modified);

    void addSubProperty(QtProperty *property);
    void insertSubProperty(QtProperty *property, QtProperty *afterProperty);
    void removeSubProperty(QtProperty *property);
protected:
    explicit QtProperty(QtAbstractPropertyManager *manager);
    void propertyChanged();
private:
    friend class QtAbstractPropertyManager;
    QtPropertyPrivate *d_ptr;
};

class QtAbstractPropertyManagerPrivate;

class QTPROPERTYBROWSER_EXPORT QtAbstractPropertyManager : public QObject
{
    Q_OBJECT
public:

    explicit QtAbstractPropertyManager(QObject *parent = 0);
    ~QtAbstractPropertyManager();

    QSet<QtProperty *> properties() const;
    void clear() const;

    QtProperty *addProperty(const QString &name = QString());
signals:

    void propertyInserted(QtProperty *property,
                QtProperty *parent, QtProperty *after);
    void propertyChanged(QtProperty *property);
    void propertyRemoved(QtProperty *property, QtProperty *parent);
    void propertyDestroyed(QtProperty *property);
protected:
    virtual bool hasValue(const QtProperty *property) const;
    virtual QIcon valueIcon(const QtProperty *property) const;
    virtual QString valueText(const QtProperty *property) const;
    virtual QString displayText(const QtProperty *property) const;
    virtual EchoMode echoMode(const QtProperty *) const;
    virtual void initializeProperty(QtProperty *property) = 0;
    virtual void uninitializeProperty(QtProperty *property);
    virtual QtProperty *createProperty();
private:
    friend class QtProperty;
    QtAbstractPropertyManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtAbstractPropertyManager)
    Q_DISABLE_COPY(QtAbstractPropertyManager)
};

class QTPROPERTYBROWSER_EXPORT QtAbstractEditorFactoryBase : public QObject
{
    Q_OBJECT
public:
    virtual QWidget *createEditor(QtProperty *property, QWidget *parent) = 0;
protected:
    explicit QtAbstractEditorFactoryBase(QObject *parent = 0)
        : QObject(parent) {}

    virtual void breakConnection(QtAbstractPropertyManager *manager) = 0;
protected slots:
    virtual void managerDestroyed(QObject *manager) = 0;

    friend class QtAbstractPropertyBrowser;
};

template <class PropertyManager>
class QtAbstractEditorFactory : public QtAbstractEditorFactoryBase
{
public:
    explicit QtAbstractEditorFactory(QObject *parent) : QtAbstractEditorFactoryBase(parent) {}
    QWidget *createEditor(QtProperty *property, QWidget *parent)
    {
        QSetIterator<PropertyManager *> it(m_managers);
        while (it.hasNext()) {
            PropertyManager *manager = it.next();
            if (manager == property->propertyManager()) {
                return createEditor(manager, property, parent);
            }
        }
        return 0;
    }
    void addPropertyManager(PropertyManager *manager)
    {
        if (m_managers.contains(manager))
            return;
        m_managers.insert(manager);
        connectPropertyManager(manager);
        connect(manager, SIGNAL(destroyed(QObject *)),
                    this, SLOT(managerDestroyed(QObject *)));
    }
    void removePropertyManager(PropertyManager *manager)
    {
        if (!m_managers.contains(manager))
            return;
        disconnect(manager, SIGNAL(destroyed(QObject *)),
                    this, SLOT(managerDestroyed(QObject *)));
        disconnectPropertyManager(manager);
        m_managers.remove(manager);
    }
    QSet<PropertyManager *> propertyManagers() const
    {
        return m_managers;
    }
    PropertyManager *propertyManager(QtProperty *property) const
    {
        QtAbstractPropertyManager *manager = property->propertyManager();
        QSetIterator<PropertyManager *> itManager(m_managers);
        while (itManager.hasNext()) {
            PropertyManager *m = itManager.next();
            if (m == manager) {
                return m;
            }
        }
        return 0;
    }
protected:
    virtual void connectPropertyManager(PropertyManager *manager) = 0;
    virtual QWidget *createEditor(PropertyManager *manager, QtProperty *property,
                QWidget *parent) = 0;
    virtual void disconnectPropertyManager(PropertyManager *manager) = 0;
    void managerDestroyed(QObject *manager)
    {
        QSetIterator<PropertyManager *> it(m_managers);
        while (it.hasNext()) {
            PropertyManager *m = it.next();
            if (m == manager) {
                m_managers.remove(m);
                return;
            }
        }
    }
private:
    void breakConnection(QtAbstractPropertyManager *manager)
    {
        QSetIterator<PropertyManager *> it(m_managers);
        while (it.hasNext()) {
            PropertyManager *m = it.next();
            if (m == manager) {
                removePropertyManager(m);
                return;
            }
        }
    }
private:
    QSet<PropertyManager *> m_managers;
    friend class QtAbstractPropertyEditor;
};

class QtAbstractPropertyBrowser;
class QtBrowserItemPrivate;

class QTPROPERTYBROWSER_EXPORT QtBrowserItem
{
public:
    QtProperty *property() const;
    QtBrowserItem *parent() const;
    QList<QtBrowserItem *> children() const;
    QtAbstractPropertyBrowser *browser() const;
private:
    explicit QtBrowserItem(QtAbstractPropertyBrowser *browser, QtProperty *property, QtBrowserItem *parent);
    ~QtBrowserItem();
    QtBrowserItemPrivate *d_ptr;
    friend class QtAbstractPropertyBrowserPrivate;
};

class QtAbstractPropertyBrowserPrivate;

class QTPROPERTYBROWSER_EXPORT QtAbstractPropertyBrowser : public QWidget
{
    Q_OBJECT
public:

    explicit QtAbstractPropertyBrowser(QWidget *parent = 0);
    ~QtAbstractPropertyBrowser();

    QList<QtProperty *> properties() const;
    QList<QtBrowserItem *> items(QtProperty *property) const;
    QtBrowserItem *topLevelItem(QtProperty *property) const;
    QList<QtBrowserItem *> topLevelItems() const;
    void clear();

    template <class PropertyManager>
    void setFactoryForManager(PropertyManager *manager,
                    QtAbstractEditorFactory<PropertyManager> *factory) {
        QtAbstractPropertyManager *abstractManager = manager;
        QtAbstractEditorFactoryBase *abstractFactory = factory;

        if (addFactory(abstractManager, abstractFactory))
            factory->addPropertyManager(manager);
    }

    void unsetFactoryForManager(QtAbstractPropertyManager *manager);

    QtBrowserItem *currentItem() const;
    void setCurrentItem(QtBrowserItem *);

signals:
    void currentItemChanged(QtBrowserItem *);

public slots:

    QtBrowserItem *addProperty(QtProperty *property);
    QtBrowserItem *insertProperty(QtProperty *property, QtProperty *afterProperty);
    void removeProperty(QtProperty *property);

protected:

    virtual void itemInserted(QtBrowserItem *item, QtBrowserItem *afterItem) = 0;
    virtual void itemRemoved(QtBrowserItem *item) = 0;
    // can be tooltip, statustip, whatsthis, name, icon, text.
    virtual void itemChanged(QtBrowserItem *item) = 0;

    virtual QWidget *createEditor(QtProperty *property, QWidget *parent);
private:

    bool addFactory(QtAbstractPropertyManager *abstractManager,
                QtAbstractEditorFactoryBase *abstractFactory);

    QtAbstractPropertyBrowserPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtAbstractPropertyBrowser)
    Q_DISABLE_COPY(QtAbstractPropertyBrowser)
    Q_PRIVATE_SLOT(d_func(), void slotPropertyInserted(QtProperty *,
                            QtProperty *, QtProperty *))
    Q_PRIVATE_SLOT(d_func(), void slotPropertyRemoved(QtProperty *,
                            QtProperty *))
    Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
    Q_PRIVATE_SLOT(d_func(), void slotPropertyDataChanged(QtProperty *))

};


class QtPropertyPrivate
{
public:
    QtPropertyPrivate(QtAbstractPropertyManager *manager)
        : m_enabled(true),
          m_modified(false),
          m_manager(manager) {}
    QtProperty *q_ptr;

    QSet<QtProperty *> m_parentItems;
    QList<QtProperty *> m_subItems;

    QString m_toolTip;
    QString m_statusTip;
    QString m_whatsThis;
    QString m_name;
    bool m_enabled;
    bool m_modified;

    QtAbstractPropertyManager * const m_manager;
};



class QtAbstractPropertyManagerPrivate
{
    QtAbstractPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtAbstractPropertyManager)
public:
    void propertyDestroyed(QtProperty *property);
    void propertyChanged(QtProperty *property) const;
    void propertyRemoved(QtProperty *property,
                QtProperty *parentProperty) const;
    void propertyInserted(QtProperty *property, QtProperty *parentProperty,
                QtProperty *afterProperty) const;

    QSet<QtProperty *> m_properties;
};


typedef QMap<QtAbstractPropertyBrowser *, QMap<QtAbstractPropertyManager *,
                            QtAbstractEditorFactoryBase *> > Map1;
typedef QMap<QtAbstractPropertyManager *, QMap<QtAbstractEditorFactoryBase *,
                            QList<QtAbstractPropertyBrowser *> > > Map2;


class QtAbstractPropertyBrowserPrivate
{
    QtAbstractPropertyBrowser *q_ptr;
    Q_DECLARE_PUBLIC(QtAbstractPropertyBrowser)
public:
    QtAbstractPropertyBrowserPrivate();

    void insertSubTree(QtProperty *property,
            QtProperty *parentProperty);
    void removeSubTree(QtProperty *property,
            QtProperty *parentProperty);
    void createBrowserIndexes(QtProperty *property, QtProperty *parentProperty, QtProperty *afterProperty);
    void removeBrowserIndexes(QtProperty *property, QtProperty *parentProperty);
    QtBrowserItem *createBrowserIndex(QtProperty *property, QtBrowserItem *parentIndex, QtBrowserItem *afterIndex);
    void removeBrowserIndex(QtBrowserItem *index);
    void clearIndex(QtBrowserItem *index);

    void slotPropertyInserted(QtProperty *property,
            QtProperty *parentProperty, QtProperty *afterProperty);
    void slotPropertyRemoved(QtProperty *property, QtProperty *parentProperty);
    void slotPropertyDestroyed(QtProperty *property);
    void slotPropertyDataChanged(QtProperty *property);

    QList<QtProperty *> m_subItems;
    QMap<QtAbstractPropertyManager *, QList<QtProperty *> > m_managerToProperties;
    QMap<QtProperty *, QList<QtProperty *> > m_propertyToParents;

    QMap<QtProperty *, QtBrowserItem *> m_topLevelPropertyToIndex;
    QList<QtBrowserItem *> m_topLevelIndexes;
    QMap<QtProperty *, QList<QtBrowserItem *> > m_propertyToIndexes;

    QtBrowserItem *m_currentItem;
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#endif // QTPROPERTYBROWSER_H
