/* This file is part of KDevelop
    Copyright 2010 Aleix Pol Gonzalez <aleixpol@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "projectchangesmodel.h"

#include "debug.h"

#include <KLocalizedString>

#include <vcs/interfaces/ibasicversioncontrol.h>
#include <interfaces/ibranchingversioncontrol.h>
#include <interfaces/iplugin.h>
#include <interfaces/iproject.h>
#include <interfaces/icore.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <vcs/vcsstatusinfo.h>
#include <vcs/vcsjob.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <project/projectmodel.h>
#include <util/path.h>

#include <QDir>
#include <QIcon>

Q_DECLARE_METATYPE(KDevelop::IProject*)

using namespace KDevelop;

ProjectChangesModel::ProjectChangesModel(QObject* parent)
    : VcsFileChangesModel(parent)
{
    foreach(IProject* p, ICore::self()->projectController()->projects())
        addProject(p);
    
    connect(ICore::self()->projectController(), &IProjectController::projectOpened,
                                              this, &ProjectChangesModel::addProject);
    connect(ICore::self()->projectController(), &IProjectController::projectClosing,
                                              this, &ProjectChangesModel::removeProject);
    
    connect(ICore::self()->documentController(), &IDocumentController::documentSaved,
                                                this, &ProjectChangesModel::documentSaved);
    connect(ICore::self()->projectController()->projectModel(), &ProjectModel::rowsInserted,
                                                this, &ProjectChangesModel::itemsAdded);
    
    connect(ICore::self()->runController(), &IRunController::jobUnregistered, this, &ProjectChangesModel::jobUnregistered);
}

ProjectChangesModel::~ProjectChangesModel()
{}

void ProjectChangesModel::addProject(IProject* p)
{
    QStandardItem* it = new QStandardItem(p->name());
    it->setData(p->name(), ProjectChangesModel::ProjectNameRole);
    IPlugin* plugin = p->versionControlPlugin();
    if(plugin) {
        auto* vcs = plugin->extension<IBasicVersionControl>();

        auto info = ICore::self()->pluginController()->pluginInfo(plugin);

        it->setIcon(QIcon::fromTheme(info.iconName()));
        it->setToolTip(vcs->name());

        auto* branchingExtension = plugin->extension<KDevelop::IBranchingVersionControl>();
        if(branchingExtension) {
            const auto pathUrl = p->path().toUrl();
            branchingExtension->registerRepositoryForCurrentBranchChanges(pathUrl);
            // can't use new signal slot syntax here, IBranchingVersionControl is not a QObject
            connect(plugin, SIGNAL(repositoryBranchChanged(QUrl)), this, SLOT(repositoryBranchChanged(QUrl)));
            repositoryBranchChanged(pathUrl);
        } else
            reload(QList<IProject*>() << p);
    } else {
        it->setEnabled(false);
    }
    
    appendRow(it);
}

void ProjectChangesModel::removeProject(IProject* p)
{
    QStandardItem* it=projectItem(p);
    
    removeRow(it->row());
}

QStandardItem* findItemChild(QStandardItem* parent, const QVariant& value, int role = Qt::DisplayRole)
{
    for(int i=0; i<parent->rowCount(); i++) {
        QStandardItem* curr=parent->child(i);
        
        if(curr->data(role) == value)
            return curr;
    }
    return nullptr;
}

QStandardItem* ProjectChangesModel::projectItem(IProject* p) const
{
    return findItemChild(invisibleRootItem(), p->name(), ProjectChangesModel::ProjectNameRole);
}

void ProjectChangesModel::updateState(IProject* p, const KDevelop::VcsStatusInfo& status)
{
    QStandardItem* pItem = projectItem(p);
    Q_ASSERT(pItem);
    
    VcsFileChangesModel::updateState(pItem, status);
}

void ProjectChangesModel::changes(IProject* project, const QList<QUrl>& urls, IBasicVersionControl::RecursionMode mode)
{
    IPlugin* vcsplugin=project->versionControlPlugin();
    IBasicVersionControl* vcs = vcsplugin ? vcsplugin->extension<IBasicVersionControl>() : nullptr;
    
    if(vcs && vcs->isVersionControlled(urls.first())) { //TODO: filter?
        VcsJob* job=vcs->status(urls, mode);
        job->setProperty("urls", qVariantFromValue<QList<QUrl>>(urls));
        job->setProperty("mode", qVariantFromValue<int>(mode));
        job->setProperty("project", qVariantFromValue(project));
        connect(job, &VcsJob::finished, this, &ProjectChangesModel::statusReady);
        
        ICore::self()->runController()->registerJob(job);
    }
}

void ProjectChangesModel::statusReady(KJob* job)
{
    auto* status=static_cast<VcsJob*>(job);

    const QList<QVariant> states = status->fetchResults().toList();
    auto* project = job->property("project").value<KDevelop::IProject*>();
    if(!project)
        return;

    QSet<QUrl> foundUrls;
    foundUrls.reserve(states.size());
    for (const QVariant& state : states) {
        const VcsStatusInfo st = state.value<VcsStatusInfo>();
        foundUrls += st.url();

        updateState(project, st);
    }

    QStandardItem* itProject = projectItem(project);
    if (!itProject) {
        qCDebug(PROJECT) << "Project no longer listed in model:" << project->name() << "- skipping update";
        return;
    }

    IBasicVersionControl::RecursionMode mode = IBasicVersionControl::RecursionMode(job->property("mode").toInt());
    const QSet<QUrl> uncertainUrls = urls(itProject).toSet().subtract(foundUrls);
    const QList<QUrl> sourceUrls = job->property("urls").value<QList<QUrl>>();
    for (const QUrl& url : sourceUrls) {
        if(url.isLocalFile() && QDir(url.toLocalFile()).exists()) {
            for (const QUrl& currentUrl : uncertainUrls) {
                if((mode == IBasicVersionControl::NonRecursive && currentUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash) == url.adjusted(QUrl::StripTrailingSlash))
                    || (mode == IBasicVersionControl::Recursive && url.isParentOf(currentUrl))
                ) {
                    removeUrl(currentUrl);
                }
            }
        }
    }
}

void ProjectChangesModel::documentSaved(KDevelop::IDocument* document)
{
    reload({document->url()});
}

void ProjectChangesModel::itemsAdded(const QModelIndex& parent, int start, int end)
{
    ProjectModel* model=ICore::self()->projectController()->projectModel();
    ProjectBaseItem* item=model->itemFromIndex(parent);

    if(!item)
        return;

    IProject* project=item->project();
    
    if(!project)
        return;
    
    QList<QUrl> urls;
    
    for(int i=start; i<end; i++) {
        QModelIndex idx=parent.model()->index(i, 0, parent);
        item=model->itemFromIndex(idx);
        
        if(item->type()==ProjectBaseItem::File || item->type()==ProjectBaseItem::Folder || item->type()==ProjectBaseItem::BuildFolder)
            urls += item->path().toUrl();
    }
        
    if(!urls.isEmpty())
        changes(project, urls, KDevelop::IBasicVersionControl::NonRecursive);
}

void ProjectChangesModel::reload(const QList<IProject*>& projects)
{
    for (IProject* project : projects) {
        changes(project, {project->path().toUrl()}, KDevelop::IBasicVersionControl::Recursive);
    }
}

void ProjectChangesModel::reload(const QList<QUrl>& urls)
{
    for (const QUrl& url : urls) {
        IProject* project=ICore::self()->projectController()->findProjectForUrl(url);
        
        if (project) {
            // FIXME: merge multiple urls of the same project
            changes(project, {url}, KDevelop::IBasicVersionControl::NonRecursive);
        }
    }
}

void ProjectChangesModel::reloadAll()
{
    QList< IProject* > projects = ICore::self()->projectController()->projects();
    reload(projects);
}

void ProjectChangesModel::jobUnregistered(KJob* job)
{
    static const QList<VcsJob::JobType> readOnly = QList<VcsJob::JobType>{
        KDevelop::VcsJob::Add,
        KDevelop::VcsJob::Remove,
        KDevelop::VcsJob::Pull,
        KDevelop::VcsJob::Commit,
        KDevelop::VcsJob::Move,
        KDevelop::VcsJob::Copy,
        KDevelop::VcsJob::Revert,
    };

    auto* vcsjob=dynamic_cast<VcsJob*>(job);
    if(vcsjob && readOnly.contains(vcsjob->type())) {
        reloadAll();
    }
}

void ProjectChangesModel::repositoryBranchChanged(const QUrl& url)
{
    IProject* project = ICore::self()->projectController()->findProjectForUrl(url);
    if(project) {
        IPlugin* v = project->versionControlPlugin();
        Q_ASSERT(v);
        auto* branching = v->extension<IBranchingVersionControl>();
        Q_ASSERT(branching);
        VcsJob* job = branching->currentBranch(url);
        connect(job, &VcsJob::resultsReady, this, &ProjectChangesModel::branchNameReady);
        job->setProperty("project", QVariant::fromValue<QObject*>(project));
        ICore::self()->runController()->registerJob(job);
    }
}

void ProjectChangesModel::branchNameReady(VcsJob* job)
{
    auto* project = qobject_cast<IProject*>(job->property("project").value<QObject*>());
    if(job->status()==VcsJob::JobSucceeded) {
        QString name = job->fetchResults().toString();
        QString branchName = name.isEmpty() ? i18n("no branch") : name;
        projectItem(project)->setText(i18nc("project name (branch name)", "%1 (%2)", project->name(), branchName));
    } else {
        projectItem(project)->setText(project->name());
    }

    reload(QList<IProject*>() << project);
}
