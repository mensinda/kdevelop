/***************************************************************************
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KDEVPLATFORM_DVCS_PLUGIN_H
#define KDEVPLATFORM_DVCS_PLUGIN_H

#include <QObject>
#include <QUrl>

#include <interfaces/iplugin.h>

#include "dvcsevent.h"
#include <vcs/vcsexport.h>
#include <vcs/interfaces/idistributedversioncontrol.h>
#include <vcs/interfaces/ibranchingversioncontrol.h>

class QMenu;

namespace KDevelop
{
class DVcsJob;

/**
 * DistributedVersionControlPlugin is a base class for git/hg/bzr plugins. This class implements
 * KDevelop::IBasicVersionControl, KDevelop::IDistributedVersionControl and KDevelop::IPlugin (contextMenuExtension).
 * DistributedVersionControlPlugin class uses IDVCSexecutor to get all jobs
 * from real DVCS plugins like Git. It is based on KDevelop's CVS plugin (also looks like svn plugin is it's relative too).
 * @note Create only special items in contextMenuExtension, all standard menu items are created in vcscommon plugin!
 */
class KDEVPLATFORMVCS_EXPORT DistributedVersionControlPlugin : public IPlugin, public IDistributedVersionControl, public IBranchingVersionControl
{
    Q_OBJECT
    Q_INTERFACES(KDevelop::IBasicVersionControl KDevelop::IDistributedVersionControl KDevelop::IBranchingVersionControl)
public:

    DistributedVersionControlPlugin(QObject *parent, const QString& componentName);
    ~DistributedVersionControlPlugin() override;

    // Begin: KDevelop::IBasicVersionControl

    /** Used in KDevelop's appwizardplugin (creates import widget) */
    VcsImportMetadataWidget* createImportMetadataWidget(QWidget* parent) override;

    // From KDevelop::IPlugin
    /** Creates context menu
     * @note Create only special items here (like checkout), all standard menu items are created in vcscommon plugin!
     */
    ContextMenuExtension contextMenuExtension(Context* context, QWidget* parent) override;

    /**
      * Parses the output generated by a <tt>dvcs log</tt> command and
      * fills the given QList with all commits infos found in the given output.
      * @param job The finished job of a <tt>dvcs log</tt> call
      * @param revisions Will be filled with all revision infos found in @p jobOutput
      * TODO: Change it to pass the results in @c job->getResults()
      */
    virtual void parseLogOutput(const DVcsJob * job,
                                QVector<KDevelop::DVcsEvent>& revisions) const = 0;
    
    /** Returns the list of all commits (in all branches).
     * @see CommitView and CommitViewDelegate to see how this list is used.
     */
    virtual QVector<KDevelop::DVcsEvent> allCommits(const QString& repo) = 0;

    /**
     * When a plugin wants to add elements to the vcs menu, this method can be
     * overridden.
     */
    virtual void additionalMenuEntries(QMenu* menu, const QList<QUrl>& urls);
public Q_SLOTS:
    //slots for context menu
    void ctxBranchManager();

protected:
    /** Checks if dirPath is located in DVCS repository */
    virtual bool isValidDirectory(const QUrl &dirPath) = 0;

private:
    const QScopedPointer<class DistributedVersionControlPluginPrivate> d;
};

}

#endif
