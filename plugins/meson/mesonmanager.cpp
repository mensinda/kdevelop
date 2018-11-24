/* This file is part of KDevelop
    Copyright 2017 Aleix Pol Gonzalez <aleixpol@kde.org>

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

#include "mesonmanager.h"
#include "mesonbuilder.h"
#include "mesonconfig.h"
#include "mesonimportjob.h"
#include <interfaces/iproject.h>
#include <project/projectmodel.h>
#include <util/executecompositejob.h>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <QFileDialog>

#include "debug.h"

using namespace KDevelop;

static const QString GENERATOR_NINJA = QStringLiteral("ninja");

K_PLUGIN_FACTORY_WITH_JSON(MesonSupportFactory, "kdevmesonmanager.json", registerPlugin<MesonManager>();)

MesonManager::MesonManager(QObject* parent, const QVariantList& args)
    : AbstractFileManagerPlugin(QStringLiteral("KDevMesonManager"), parent, args)
    , m_builder(new MesonBuilder(this))
{
}

MesonManager::~MesonManager()
{
    delete m_builder;
}

// *********************************
// * AbstractFileManagerPlugin API *
// *********************************

IProjectFileManager::Features MesonManager::features() const
{
    return IProjectFileManager::Files | IProjectFileManager::Folders | IProjectFileManager::Targets;
}

ProjectFolderItem* MesonManager::createFolderItem(IProject* project, const Path& path, ProjectBaseItem* parent)
{
    // TODO: Maybe use meson targets instead
    if (QFile::exists(path.toLocalFile() + QStringLiteral("/meson.build")))
        return new ProjectBuildFolderItem(project, path, parent);
    else
        return AbstractFileManagerPlugin::createFolderItem(project, path, parent);
}

// ***************************
// * IBuildSystemManager API *
// ***************************

KJob* MesonManager::createImportJob(ProjectFolderItem* item)
{
    auto project = item->project();
    auto job = new MesonImportJob(this, project, this);

    connect(job, &KJob::result, this, [this, job, project]() {
        if (job->error() != 0) {
            qCWarning(KDEV_Meson) << "couldn't load project successfully" << project->name();
            m_projects.remove(project);
        }
    });

    const QList<KJob*> jobs = {
        builder()->configure(project), // Make sure the project is configured
        job, // Import the compile_commands.json file
        AbstractFileManagerPlugin::createImportJob(item) // generate the file system listing
    };

    Q_ASSERT(!jobs.contains(nullptr));
    auto composite = new ExecuteCompositeJob(this, jobs);
    composite->setAbortOnError(false);
    return composite;
}

Path MesonManager::buildDirectory(ProjectBaseItem* item) const
{
    Q_ASSERT(item);
    Meson::BuildDir buildDir = Meson::currentBuildDir(item->project());
    return buildDir.buildDir;
}

IProjectBuilder* MesonManager::builder() const
{
    return m_builder;
}

Meson::BuildDir MesonManager::newBuildDirectory(IProject* project)
{
    // TODO: Make a real dialog
    Meson::BuildDir buildDir;
    const QString path = QFileDialog::getExistingDirectory(nullptr, i18n("Choose a build directory"));
    buildDir.buildDir = Path(path);
    buildDir.mesonBackend = GENERATOR_NINJA;

    Meson::MesonConfig mesonCfg = Meson::getMesonConfig(project);
    mesonCfg.addBuildDir(buildDir);
    Meson::writeMesonConfig(project, mesonCfg);

    return buildDir;
}

QVector<QString> MesonManager::supportedMesonBackends() const
{
    // TODO check for Windows / Unix. Add support for VS, XCode?
    return { GENERATOR_NINJA };
}

QString MesonManager::defaultMesonBackend() const
{
    return GENERATOR_NINJA;
}

void MesonManager::setProjectData(IProject* project, const QJsonObject& data)
{
    m_projects[project] = data;
}

#include "mesonmanager.moc"
