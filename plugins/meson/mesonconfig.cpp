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

#include "mesonconfig.h"
#include "mesonmanager.h"
#include <KLocalizedString>
#include <QFileDialog>
#include <debug.h>
#include <interfaces/iproject.h>
#include <project/interfaces/ibuildsystemmanager.h>
#include <project/projectmodel.h>

using namespace KDevelop;
using namespace Meson;

static const QString ROOT_CONFIG = QStringLiteral("MesonManager");
static const QString NUM_BUILD_DIRS = QStringLiteral("Number of Build Directories");
static const QString CURRENT_INDEX = QStringLiteral("Current Build Directory Index");

static const QString BUILD_DIR_SEC = QStringLiteral("BuildDir %1");
static const QString BUILD_DIR_PATH = QStringLiteral("Build Directory Path");
static const QString BACKEND = QStringLiteral("Meson Generator Backend");

int MesonConfig::addBuildDir(BuildDir dir)
{
    int newIndex = buildDirs.size();
    qCDebug(KDEV_Meson) << "BuildDirectories::addBuildDir()=" << dir.buildDir;
    buildDirs.push_back(dir);

    // Make sure m_currentIndex is valid
    if (currentIndex < 0) {
        currentIndex = newIndex;
    }

    return newIndex;
}

bool MesonConfig::removeBuildDir(int index)
{
    if (index > buildDirs.size() || index < 0) {
        return false;
    }

    buildDirs.removeAt(index);

    if (currentIndex >= buildDirs.size()) {
        currentIndex = buildDirs.size() - 1;
    }

    return true;
}

KConfigGroup Meson::rootGroup(IProject* project)
{
    if (!project) {
        qCWarning(KDEV_Meson) << "Meson::rootGroup: IProject pointer is nullptr";
        return KConfigGroup();
    }

    return project->projectConfiguration()->group(ROOT_CONFIG);
}

MesonConfig Meson::getMesonConfig(IProject* project)
{
    KConfigGroup root = rootGroup(project);
    MesonConfig result;

    int numDirs = root.readEntry(NUM_BUILD_DIRS, 0);
    result.currentIndex = root.readEntry(CURRENT_INDEX, -1);

    for (int i = 0; i < numDirs; ++i) {
        QString section = BUILD_DIR_SEC.arg(i);
        if (!root.hasGroup(section)) {
            continue;
        }

        KConfigGroup current = root.group(section);
        BuildDir currBD;
        currBD.buildDir = Path(current.readEntry(BUILD_DIR_PATH, QString()));
        currBD.mesonBackend = current.readEntry(BACKEND, QString());
        result.buildDirs.push_back(currBD);
    }

    if (result.buildDirs.isEmpty()) {
        result.currentIndex = -1;
    } else if (result.currentIndex < 0 || result.currentIndex >= result.buildDirs.size()) {
        result.currentIndex = 0;
    }

    return result;
}

void Meson::writeMesonConfig(IProject* project, const MesonConfig& cfg)
{
    KConfigGroup root = rootGroup(project);

    // Make sure that the config we write is valid
    int currentIndex = cfg.currentIndex;
    if (cfg.buildDirs.isEmpty()) {
        currentIndex = -1;
    } else if (currentIndex < 0 || currentIndex >= cfg.buildDirs.size()) {
        currentIndex = 0;
    }

    root.writeEntry(NUM_BUILD_DIRS, cfg.buildDirs.size());
    root.writeEntry(CURRENT_INDEX, currentIndex);

    int counter = 0;
    for (auto const& i : cfg.buildDirs) {
        KConfigGroup current = root.group(BUILD_DIR_SEC.arg(counter++));

        current.writeEntry(BUILD_DIR_PATH, i.buildDir.path());
        current.writeEntry(BACKEND, i.mesonBackend);
    }
}

BuildDir Meson::currentBuildDir(IProject* project)
{
    Q_ASSERT(project);
    MesonConfig cfg = getMesonConfig(project);
    if (cfg.currentIndex < 0 || cfg.currentIndex >= cfg.buildDirs.size()) {
        cfg.currentIndex = 0; // Default to the first build dir

        // Request a new build directory if neccessary
        if (cfg.buildDirs.isEmpty()) {
            IBuildSystemManager* ibsm = project->buildSystemManager();
            MesonManager* bsm = dynamic_cast<MesonManager*>(ibsm);
            if (!bsm) {
                qCCritical(KDEV_Meson) << "Invalid build system manager for mesonconfig";
                qCCritical(KDEV_Meson) << "Project " << project->name() << "is probably broken";
                return BuildDir();
            }

            // newBuildDirectory() will add the build dir to the config and set cfg.currentIndex
            return bsm->newBuildDirectory(project);
        }
    }

    return cfg.buildDirs[cfg.currentIndex];
}
