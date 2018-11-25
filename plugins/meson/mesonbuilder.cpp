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

#include "mesonbuilder.h"
#include "mesonconfig.h"
#include <QDir>
#include <QFileInfo>
#include <debug.h>
#include <interfaces/icore.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/iproject.h>
#include <outputview/outputexecutejob.h>
#include <util/path.h>
#include <klocalizedstring.h>

using namespace KDevelop;

class ErrorJob : public KJob
{
    Q_OBJECT
public:
    ErrorJob(QObject* parent, const QString& error)
        : KJob(parent)
        , m_error(error)
    {
    }

    void start() override
    {
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

MesonBuilder::DirectoryStatus MesonBuilder::evaluateBuildDirectory(const Path& path, QString backend)
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
    if (dir.isEmpty(QDir::AllEntries)) {
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

KJob* MesonBuilder::configure(KDevelop::IProject* project)
{
    Q_ASSERT(project);
    Meson::BuildDir buildDir = Meson::currentBuildDir(project);
    if (!buildDir.isValid()) {
        qCWarning(KDEV_Meson) << "The current build directory is invalid";
        return new ErrorJob(this, i18n("The current build directory for %1 is invalid").arg(project->name()));
    }

    qCDebug(KDEV_Meson) << "MesonBuilder::configure() PATH=" << buildDir.buildDir;
    auto job = new KDevelop::OutputExecuteJob();
    *job << QStringLiteral("meson") << project->path().toLocalFile();
    job->setWorkingDirectory(buildDir.buildDir.toUrl());
    return job;
}

KJob* MesonBuilder::build(KDevelop::ProjectBaseItem* item)
{
    // TODO: probably want to make sure it's configured first
    return m_ninjaBuilder->install(item);
}

KJob* MesonBuilder::clean(KDevelop::ProjectBaseItem* item)
{
    return m_ninjaBuilder->clean(item);
}

KJob* MesonBuilder::install(KDevelop::ProjectBaseItem* dom, const QUrl& installPath)
{
    return m_ninjaBuilder->install(dom, installPath);
}

KJob* MesonBuilder::prune(KDevelop::IProject* project)
{
    qCDebug(KDEV_Meson) << "TODO: Implement prune; project == " << project->name();
    return nullptr;
}

#include "mesonbuilder.moc"
