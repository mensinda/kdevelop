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

#include "mesonnewbuilddir.h"
#include "mesonbuilder.h"
#include "mesonmanager.h"
#include "ui_mesonnewbuilddir.h"

#include <interfaces/icore.h>
#include <interfaces/iproject.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/iruntime.h>
#include <interfaces/iruntimecontroller.h>
#include <project/helper.h>

#include <QDialogButtonBox>
#include <QFileInfo>
#include <algorithm>
#include <debug.h>
#include <kcolorscheme.h>

using namespace KDevelop;

MesonNewBuildDir::MesonNewBuildDir(IProject* project, QWidget* parent)
    : QDialog(parent)
    , m_project(project)
{
    Q_ASSERT(project); // Just in case
    MesonManager* mgr = dynamic_cast<MesonManager*>(m_project->buildSystemManager());
    Q_ASSERT(mgr); // This dialog only works with the MesonManager

    setWindowTitle(
        i18n("Configure a build directory - %1", ICore::self()->runtimeController()->currentRuntime()->name()));

    m_ui = new Ui::MesonNewBuildDir;
    m_ui->setupUi(this);

    m_ui->advanced->setSupportedBackends(mgr->supportedMesonBackends());

    connect(m_ui->b_buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* b) {
        if (m_ui->b_buttonBox->buttonRole(b) == QDialogButtonBox::ResetRole) {
            resetFields();
        }
    });

    resetFields();
}

MesonNewBuildDir::~MesonNewBuildDir()
{
    delete m_ui;
}

void MesonNewBuildDir::resetFields()
{
    Meson::MesonConfig cfg = Meson::getMesonConfig(m_project);
    Path projectPath = m_project->path();
    MesonManager* mgr = dynamic_cast<MesonManager*>(m_project->buildSystemManager());
    Q_ASSERT(mgr); // This dialog only works with the MesonManager

    auto aConf = m_ui->advanced->getConfig();

    // Find a build dir that is not already configured
    Path buildDirPath = projectPath;
    buildDirPath.addPath(QStringLiteral("build"));

    auto checkInCfg = [](Meson::MesonConfig const& cfg, Path const& p) -> bool {
        for (auto const& i : cfg.buildDirs) {
            if (i.buildDir == p) {
                return true;
            }
        }
        return false;
    };

    for (int i = 2; checkInCfg(cfg, buildDirPath); ++i) {
        buildDirPath = projectPath;
        buildDirPath.addPath(QStringLiteral("build%1").arg(i));
    }

    m_ui->i_buildDir->setUrl(buildDirPath.toUrl());

    // Init build type
    // TODO use introspection once https://github.com/mesonbuild/meson/pull/4564 is merged
    QStringList buildTypes = { QStringLiteral("plain"),   QStringLiteral("debug"),   QStringLiteral("debugoptimized"),
                               QStringLiteral("release"), QStringLiteral("minsize"), QStringLiteral("custom") };

    m_ui->i_buildType->clear();
    m_ui->i_buildType->addItems(buildTypes);
    m_ui->i_buildType->setCurrentIndex(std::max(0, buildTypes.indexOf(QStringLiteral("debug"))));

    // Install prefix
    m_ui->i_installPrefix->clear();

    // Extra args
    aConf.args.clear();

    // Backend
    aConf.backend = mgr->defaultMesonBackend();

    // Meson exe
    aConf.meson = mgr->findMeson();

    m_ui->advanced->setConfig(aConf);
    updated();
}

void MesonNewBuildDir::setStatus(const QString& str, bool validConfig)
{
    m_configIsValid = validConfig;

    KColorScheme scheme(QPalette::Normal);
    KColorScheme::ForegroundRole role;
    if (validConfig) {
        role = KColorScheme::PositiveText;
    } else {
        role = KColorScheme::NegativeText;
    }

    m_ui->l_statusMessage->setText(
        QStringLiteral("<i><font color='%1'>%2</font></i>").arg(scheme.foreground(role).color().name(), str));

    auto okButton = m_ui->b_buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(m_configIsValid);
    if (m_configIsValid) {
        auto cancelButton = m_ui->b_buttonBox->button(QDialogButtonBox::Cancel);
        cancelButton->clearFocus();
    }
}

void MesonNewBuildDir::updated()
{
    auto advanced = m_ui->advanced->getConfig();
    Path buildDir = Path(m_ui->i_buildDir->url());
    QFileInfo mesonExe(advanced.meson.toLocalFile());

    if (!mesonExe.exists() || !mesonExe.isExecutable()
        || !mesonExe.permission(QFileDevice::ReadUser | QFileDevice::ExeUser)) {
        setStatus(i18n("Specified meson executable does not exist"), false);
        return;
    }

    MesonBuilder::DirectoryStatus status = MesonBuilder::evaluateBuildDirectory(buildDir, advanced.backend);
    switch (status) {
    case MesonBuilder::CLEAN:
    case MesonBuilder::DOES_NOT_EXIST:
        setStatus(i18n("Creating new build directory"), true);
        break;
    case MesonBuilder::MESON_CONFIGURED:
        setStatus(i18n("Using already configured build directory"), true);
        break;
    case MesonBuilder::INVALID_BUILD_DIR:
        setStatus(i18n("Can not use specified directory"), false);
        break;
    case MesonBuilder::DIR_NOT_EMPTY:
        setStatus(i18n("There are already files in the build directory"), false);
        break;
    case MesonBuilder::EMPTY_STRING:
        setStatus(i18n("The build directory field must not be empty"), false);
        break;
    case MesonBuilder::___UNDEFINED___:
        setStatus(i18n("You have reached unreachable code. This is a bug"), false);
        break;
    }
}

Meson::BuildDir MesonNewBuildDir::currentConfig() const
{
    Meson::BuildDir buildDir;
    if (!m_configIsValid) {
        qCDebug(KDEV_Meson) << "Can not generate build dir config from invalid config";
        return buildDir;
    }

    auto advanced = m_ui->advanced->getConfig();

    buildDir.buildDir = Path(m_ui->i_buildDir->url());
    buildDir.buildType = m_ui->i_buildType->currentText();
    buildDir.installPrefix = Path(m_ui->i_installPrefix->url());
    buildDir.mesonArgs = advanced.args;
    buildDir.mesonBackend = advanced.backend;
    buildDir.mesonExecutable = advanced.meson;

    return buildDir;
}

bool MesonNewBuildDir::isConfigValid() const
{
    return m_configIsValid;
}
