/* This file is part of KDevelop
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

#include "mesonconfigpage.h"
#include "mesonmanager.h"
#include "mesonnewbuilddir.h"
#include "ui_mesonconfigpage.h"
#include <QIcon>
#include <debug.h>
#include <interfaces/iplugin.h>
#include <interfaces/iproject.h>

using namespace KDevelop;

MesonConfigPage::MesonConfigPage(IPlugin* plugin, IProject* project, QWidget* parent)
    : ConfigPage(plugin, nullptr, parent)
    , m_project(project)
{
    Q_ASSERT(project); // Catch errors early
    MesonManager* mgr = dynamic_cast<MesonManager*>(m_project->buildSystemManager());
    Q_ASSERT(mgr); // This dialog only works with the MesonManager

    m_ui = new Ui::MesonConfigPage;
    m_ui->setupUi(this);
    m_ui->advanced->setSupportedBackends(mgr->supportedMesonBackends());

    m_config = Meson::getMesonConfig(m_project);
    if (m_config.buildDirs.isEmpty()) {
        m_config.currentIndex = -1;
        reset();
        return;
    } else if (m_config.currentIndex < 0 || m_config.currentIndex >= m_config.buildDirs.size()) {
        m_config.currentIndex = 0;
    }

    QStringList buildPathList;
    for (auto& i : m_config.buildDirs) {
        buildPathList << i.buildDir.toLocalFile();
    }

    m_ui->i_buildDirs->blockSignals(true);
    m_ui->i_buildDirs->clear();
    m_ui->i_buildDirs->addItems(buildPathList);
    m_ui->i_buildDirs->setCurrentIndex(m_config.currentIndex);
    m_ui->i_buildDirs->blockSignals(false);

    reset();
}

void MesonConfigPage::apply()
{
    qCDebug(KDEV_Meson) << "Writing meson config for build dir " << m_current.buildDir;
    if (m_config.currentIndex >= 0) {
        readCurrentBuildDir();
        m_config.buildDirs[m_config.currentIndex] = m_current;
    }
    Meson::writeMesonConfig(m_project, m_config);
}

void MesonConfigPage::defaults()
{
    qCDebug(KDEV_Meson) << "Restoring build dir " << m_current.buildDir << " to it's default values";
    MesonManager* mgr = dynamic_cast<MesonManager*>(m_project->buildSystemManager());
    Q_ASSERT(mgr);

    m_current.mesonArgs.clear();
    m_current.mesonBackend = mgr->defaultMesonBackend();
    m_current.mesonExecutable = mgr->findMeson();

    updateCurrentBuildDir();
}

void MesonConfigPage::reset()
{
    if (m_config.buildDirs.isEmpty()) {
        m_config.currentIndex = -1;
        setWidgetsDisabled(true);
        return;
    } else if (m_config.currentIndex < 0 || m_config.currentIndex >= m_config.buildDirs.size()) {
        m_config.currentIndex = 0;
    }

    setWidgetsDisabled(false);
    qCDebug(KDEV_Meson) << "Resetting changes for build dir " << m_current.buildDir;

    m_current = m_config.buildDirs[m_config.currentIndex];
    updateCurrentBuildDir();
}

void MesonConfigPage::updateCurrentBuildDir()
{
    m_ui->i_buildType->setCurrentIndex(1);

    QStringList buildTypes = { QStringLiteral("plain"),   QStringLiteral("debug"),   QStringLiteral("debugoptimized"),
                               QStringLiteral("release"), QStringLiteral("minsize"), QStringLiteral("custom") };

    m_ui->i_buildType->clear();
    m_ui->i_buildType->addItems(buildTypes);
    m_ui->i_buildType->setCurrentIndex(std::max(0, buildTypes.indexOf(m_current.buildType)));

    m_ui->i_installPrefix->setUrl(m_current.installPrefix.toUrl());

    auto aConf = m_ui->advanced->getConfig();
    aConf.args = m_current.mesonArgs;
    aConf.backend = m_current.mesonBackend;
    aConf.meson = m_current.mesonExecutable;
    m_ui->advanced->setConfig(aConf);
}

void MesonConfigPage::readCurrentBuildDir()
{
    m_current.installPrefix = Path(m_ui->i_installPrefix->url());
    m_current.buildType = m_ui->i_buildType->currentText();

    auto aConf = m_ui->advanced->getConfig();
    m_current.mesonArgs = aConf.args;
    m_current.mesonBackend = aConf.backend;
    m_current.mesonExecutable = aConf.meson;
}

void MesonConfigPage::setWidgetsDisabled(bool disabled)
{
    if(disabled) {
        defaults();
    }

    m_ui->advanced->setDisabled(disabled);
    m_ui->c_01_basic->setDisabled(disabled);
    m_ui->c_02_buildConfig->setDisabled(disabled);
    m_ui->b_rmDir->setDisabled(disabled);
}

void MesonConfigPage::addBuildDir()
{
    qCDebug(KDEV_Meson) << "Adding build directory";
    MesonNewBuildDir newBD(m_project);

    if (!newBD.exec() || !newBD.isConfigValid()) {
        qCDebug(KDEV_Meson) << "Failed to create a new build directory";
        return;
    }

    m_current = newBD.currentConfig();
    m_config.currentIndex = m_config.addBuildDir(m_current);
    m_ui->i_buildDirs->blockSignals(true);
    m_ui->i_buildDirs->addItem(m_current.buildDir.toLocalFile());
    m_ui->i_buildDirs->setCurrentIndex(m_config.currentIndex);
    m_ui->i_buildDirs->blockSignals(false);
    apply();
    reset();
}

void MesonConfigPage::removeBuildDir()
{
    qCDebug(KDEV_Meson) << "Removing current build directory";
    m_ui->i_buildDirs->blockSignals(true);
    m_ui->i_buildDirs->removeItem(m_config.currentIndex);
    m_config.removeBuildDir(m_config.currentIndex);
    if (m_config.buildDirs.isEmpty()) {
        m_config.currentIndex = -1;
    } else if (m_config.currentIndex < 0 || m_config.currentIndex >= m_config.buildDirs.size()) {
        m_config.currentIndex = 0;
    }
    m_ui->i_buildDirs->setCurrentIndex(m_config.currentIndex);
    m_ui->i_buildDirs->blockSignals(false);
    apply();
    reset();
}

void MesonConfigPage::changeBuildDirIndex(int index)
{
    if (index == m_config.currentIndex || m_config.buildDirs.isEmpty()) {
        return;
    }

    if (index < 0 || index >= m_config.buildDirs.size()) {
        qCWarning(KDEV_Meson) << "Invalid build dir index " << index;
        return;
    }

    qCDebug(KDEV_Meson) << "Changing build directory to index " << index;

    m_config.currentIndex = index;
    reset();
}

QString MesonConfigPage::name() const
{
    return i18n("Meson");
}

QString MesonConfigPage::fullName() const
{
    return i18n("Meson project configuration");
}

QIcon MesonConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("meson"));
}
