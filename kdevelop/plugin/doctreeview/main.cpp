#include <kinstance.h>
#include "main.h"
#include "doctreeview.h"


extern "C" {

    void *init_libkdevdoctreeview()
    {
        return new DocTreeFactory;
    }
    
};


DocTreeFactory::DocTreeFactory(QObject *parent, const char *name)
    : KLibFactory(parent, name)
{
    instance();
}


DocTreeFactory::~DocTreeFactory()
{
    delete s_instance;
    s_instance = 0;
}


QObject *DocTreeFactory::create(QObject *parent, const char *name,
                                const char *classname, const QStringList &args)
{
    if (parent && !parent->isWidgetType()) {
        qDebug("Parent of print dialog is not a widget");
        return 0;
    }
    QWidget *parentWidget = (QWidget *) parent;
    
    // Depending on classname, this should method should also
    // be able to create the config dialog

    qDebug("Building DocTreeView");
    
    QObject *obj = new DocTreeView(parentWidget, name);
    emit objectCreated(obj);
    return obj;
}


KInstance *DocTreeFactory::s_instance = 0;
KInstance *DocTreeFactory::instance()
{
    if (!s_instance)
        s_instance = new KInstance("doctreeview");

    return s_instance;
}
