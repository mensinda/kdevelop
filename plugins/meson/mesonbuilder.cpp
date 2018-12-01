/* This file is part of KDevelop
    Copyright 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
    Copyright 2018 Daniel Mensinger <daniel@mensinger-ka.de>

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

#include "mesonbuilder.h"
#include "mesonconfig.h"
#include "mesonjob.h"
#include "mesonjobprune.h"
#include <QDir>
#include <QFileInfo>
#include <debug.h>
#include <executecompositejob.h>
#include <interfaces/icore.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/iproject.h>
#include <klocalizedstring.h>
#include <outputview/outputexecutejob.h>
#include <project/projectmodel.h>
#include <util/path.h>

using namespace KDevelop;

class ErrorJob : public OutputJob
{
    Q_OBJECT
public:
    ErrorJob(QObject* parent, const QString& error)
        : OutputJob(parent)
        , m_error(error)
    {
    }

    void start() override
    {
        auto* output = new OutputModel(this);
        setModel(output);
        startOutput();

        output->appendLine(i18n("    *** MESON ERROR ***"));
        output->appendLine(QStringLiteral(""));
        QStringList lines = m_error.split(QChar((int)'\n'));
        output->appendLines(lines);

        setError(!m_error.isEmpty());
        setErrorText(m_error);
        emitResult();
    }

private:
    QString m_error;
};

MesonBuilder::MesonBuilder(QObject* parent)
    : QObject(parent)
{
    auto p = KDevelop::ICore::self()->pluginController()->pluginForExtension(
        QStringLiteral("org.kdevelop.IProjectBuilder"), QStringLiteral("KDevNinjaBuilder"));
    if (p) {
        m_ninjaBuilder = p->extension<KDevelop::IProjectBuilder>();
        if (m_ninjaBuilder) {
            connect(p, SIGNAL(built(KDevelop::ProjectBaseItem*)), this, SIGNAL(built(KDevelop::ProjectBaseItem*)));
            connect(p, SIGNAL(installed(KDevelop::ProjectBaseItem*)), this,
                    SIGNAL(installed(KDevelop::ProjectBaseItem*)));
            connect(p, SIGNAL(cleaned(KDevelop::ProjectBaseItem*)), this, SIGNAL(cleaned(KDevelop::ProjectBaseItem*)));
            connect(p, SIGNAL(failed(KDevelop::ProjectBaseItem*)), this, SIGNAL(failed(KDevelop::ProjectBaseItem*)));
        }
    }
}

MesonBuilder::DirectoryStatus MesonBuilder::evaluateBuildDirectory(const Path& path, QString const& backend)
{
    QString pathSTR = path.toLocalFile();
    if (pathSTR.isEmpty()) {
        return EMPTY_STRING;
    }

    QFileInfo info(pathSTR);
    if (!info.exists()) {
        return DOES_NOT_EXIST;
    }

    if (!info.isDir() || !info.isReadable() || !info.isWritable()) {
        return INVALID_BUILD_DIR;
    }

    QDir dir(path.toLocalFile());
    if (dir.isEmpty(QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries)) {
        return CLEAN;
    }

    QStringList requiredPaths = { QStringLiteral("meson-logs"), QStringLiteral("meson-private") };
    if (backend == QStringLiteral("ninja")) {
        requiredPaths += QStringLiteral("build.ninja");
    }

    // Check if this is a meson directory
    QStringList entries = dir.entryList();
    for (auto const& i : requiredPaths) {
        if (!entries.contains(i)) {
            return DIR_NOT_EMPTY;
        }
    }

    return MESON_CONFIGURED;
}

KJob* MesonBuilder::configure(IProject* project, const Meson::BuildDir& buildDir, DirectoryStatus status)
{
    Q_ASSERT(project);

    if (!buildDir.isValid()) {
        return new ErrorJob(this, i18n("The current build directory for %1 is invalid", project->name()));
    }

    if (status == ___UNDEFINED___) {
        status = evaluateBuildDirectory(buildDir.buildDir, buildDir.mesonBackend);
    }

    KJob* job = nullptr;

    switch (status) {
    case DOES_NOT_EXIST:
    case CLEAN:
        job = new MesonJob(buildDir, project, MesonJob::CONFIGURE, {}, this);
        connect(job, &KJob::result, this, [this, project]() { emit configured(project); });
        return job;
    case MESON_CONFIGURED:
        job = new MesonJob(buildDir, project, MesonJob::RE_CONFIGURE, {}, this);
        connect(job, &KJob::result, this, [this, project]() { emit configured(project); });
        return job;
    case DIR_NOT_EMPTY:
        return new ErrorJob(
            this,
            i18n("The directory '%1' is not empty and does not seam to be an already configured build directory",
                 buildDir.buildDir.toLocalFile()));
    case INVALID_BUILD_DIR:
        return new ErrorJob(
            this,
            i18n("The directory '%1' can not be used as a meson build directory", buildDir.buildDir.toLocalFile()));
    case EMPTY_STRING:
        return new ErrorJob(
            this, i18n("The current build configuration is broken, because the build directory is not specified"));
    default:
        // This code should NEVER be reached
        return new ErrorJob(this,
                            i18n("Congratulations: You have reached unreachable code!\n"
                                 "Please report a bug at https://bugs.kde.org/\n"
                                 "FILE: %1:%2",
                                 QStringLiteral(__FILE__), __LINE__));
    }
}

KJob* MesonBuilder::configure(KDevelop::IProject* project)
{
    Q_ASSERT(project);
    auto buildDir = Meson::currentBuildDir(project);
    return configure(project, buildDir);
}

KJob* MesonBuilder::configureIfRequired(KDevelop::IProject* project, KJob* realJob)
{
    Q_ASSERT(project);
    Meson::BuildDir buildDir = Meson::currentBuildDir(project);
    DirectoryStatus status = evaluateBuildDirectory(buildDir.buildDir, buildDir.mesonBackend);

    if (status == MESON_CONFIGURED) {
        return realJob;
    } else {
        QList<KJob*> jobs = {
            configure(project, buildDir, status), // First configure the build directory
            realJob // If this succeeds execute the real job
        };

        return new ExecuteCompositeJob(this, jobs);
    }

    return new ErrorJob(this,
                        i18n("Congratulations: You have reached unreachable code!\n"
                             "Please report a bug at https://bugs.kde.org/\n"
                             "FILE: %1:%2",
                             QStringLiteral(__FILE__), __LINE__));
}

KJob* MesonBuilder::build(KDevelop::ProjectBaseItem* item)
{
    Q_ASSERT(item);
    return configureIfRequired(item->project(), m_ninjaBuilder->build(item));
}

KJob* MesonBuilder::clean(KDevelop::ProjectBaseItem* item)
{
    Q_ASSERT(item);
    return configureIfRequired(item->project(), m_ninjaBuilder->clean(item));
}

KJob* MesonBuilder::install(KDevelop::ProjectBaseItem* item, const QUrl& installPath)
{
    Q_ASSERT(item);
    return configureIfRequired(item->project(), m_ninjaBuilder->install(item, installPath));
}

KJob* MesonBuilder::prune(KDevelop::IProject* project)
{
    Q_ASSERT(project);
    Meson::BuildDir buildDir = Meson::currentBuildDir(project);
    if (!buildDir.isValid()) {
        qCWarning(KDEV_Meson) << "The current build directory is invalid";
        return new ErrorJob(this, i18n("The current build directory for %1 is invalid", project->name()));
    }

    KJob* job = new MesonJobPrune(buildDir, this);
    connect(job, &KJob::result, this, [this, project]() { emit pruned(project); });
    return job;
}

QList<IProjectBuilder*> MesonBuilder::additionalBuilderPlugins(IProject*) const
{
    return { m_ninjaBuilder };
}

#include "mesonbuilder.moc"
