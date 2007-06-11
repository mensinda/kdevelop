/* KDevelop Standard OutputView
 *
 * Copyright 2006-2007 Andreas Pakulat <apaku@gmx.de>
 * Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "standardoutputview.h"
#include "outputwidget.h"

#include <QtCore/QStringList>

#include <QtDesigner/QExtensionFactory>
#include <QtCore/QAbstractItemModel>

#include <QtGui/QAction>
#include <icore.h>
#include <iuicontroller.h>

#include <kgenericfactory.h>
#include <klocale.h>
#include <kdebug.h>
#include <kactioncollection.h>

typedef KGenericFactory<StandardOutputView> StandardOutputViewFactory ;
K_EXPORT_COMPONENT_FACTORY( kdevstandardoutputview,
                            StandardOutputViewFactory( "kdevstandardoutputview" ) )

class StandardOutputViewViewFactory : public KDevelop::IToolViewFactory{
public:
    StandardOutputViewViewFactory(StandardOutputView *part): m_part(part) {}
    virtual QWidget* create(QWidget *parent = 0)
    {
        Q_UNUSED(parent)
        OutputWidget* l = new OutputWidget( parent, m_part);
        QObject::connect( l, SIGNAL( viewRemoved( const QString& ) ),
                 m_part, SIGNAL( viewRemoved( const QString &) ) );
        QObject::connect( m_part, SIGNAL( viewRemoved( const QString& ) ),
                 l, SLOT( removeView( const QString &) ) );
        QObject::connect( l, SIGNAL( activated(const QModelIndex&) ),
                 m_part, SIGNAL(activated(const QModelIndex&)) );
        QObject::connect( l, SIGNAL( activePageChanged(const QString&) ),
                 m_part, SLOT(activePageChanged(const QString&)) );
        return l;
    }
    virtual Qt::DockWidgetArea defaultPosition(const QString &/*areaName*/)
    {
        return Qt::BottomDockWidgetArea;
    }
private:
    StandardOutputView *m_part;
};

class StandardOutputViewPrivate
{
public:
    StandardOutputViewViewFactory* m_factory;
    QMap<QString, QAbstractItemModel* > m_models;
    QMap<QString, QString> m_titles;
    QList<unsigned int> m_ids;
    QMap<QString, KDevelop::IOutputView::CloseBehaviour> m_behaviours;
    QString m_activePageId;
};

StandardOutputView::StandardOutputView(QObject *parent, const QStringList &)
    : KDevelop::IPlugin(StandardOutputViewFactory::componentData(), parent),
      d(new StandardOutputViewPrivate)
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IOutputView )
    d->m_factory = new StandardOutputViewViewFactory( this );
    core()->uiController()->addToolView( "Output View", d->m_factory );

    setXMLFile("kdevstandardoutputview.rc");

}

StandardOutputView::~StandardOutputView()
{
    delete d;
}

QString StandardOutputView::registerView( const QString& title,
                                          KDevelop::IOutputView::CloseBehaviour behaviour )
{
    unsigned int newid;
    if( d->m_ids.isEmpty() )
        newid = 0;
    else
        newid = d->m_ids.last()+1;
    kDebug(9004) << "Registering view" << title << " with behaviour: " << behaviour << endl;
    QString idstr = QString::number(newid);
    d->m_ids << newid;
    d->m_titles[idstr] = title;
    d->m_models[idstr] = 0;
    d->m_behaviours[idstr] = behaviour;
    return idstr;
}

KDevelop::IOutputView::CloseBehaviour StandardOutputView::closeBehaviour( const QString& id )
{

    if( d->m_titles.contains( id ) )
    {
        return d->m_behaviours[id];
    }
    return KDevelop::IOutputView::AllowUserClose;
}

void StandardOutputView::setModel( const QString& id, QAbstractItemModel* model )
{
    unsigned int viewid = id.toUInt();
    if( d->m_ids.contains( viewid ) )
    {
        d->m_models[id] = model;
        emit modelChanged( id );
    }else
        kDebug(9004) << "AAAH id is not known:" << id << "|" << viewid<< endl;
}

QString StandardOutputView::currentId()
{
    return d->m_activePageId;
}

QStringList StandardOutputView::registeredViews()
{
    QStringList ret;
    foreach(unsigned int id, d->m_ids)
    {
        ret << QString::number(id);
    }
    return ret;
}

QAbstractItemModel* StandardOutputView::registeredModel( const QString& id )
{
    if( d->m_models.contains( id ) )
    {
        return d->m_models[id];
    }
    return 0;
}

QString StandardOutputView::registeredTitle( const QString& id )
{
    if( d->m_titles.contains( id ) )
    {
        return d->m_titles[id];
    }
    return QString();
}

void StandardOutputView::activePageChanged( const QString &id )
{
    d->m_activePageId = id;
}

#include "standardoutputview.moc"
// kate: space-indent on; indent-width 4; tab-width: 4; replace-tabs on; auto-insert-doxygen on
