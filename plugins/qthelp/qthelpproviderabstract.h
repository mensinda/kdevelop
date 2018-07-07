/*  This file is part of KDevelop
    Copyright 2009 Aleix Pol <aleixpol@kde.org>
    Copyright 2009 David Nolden <david.nolden.kdevelop@art-master.de>
    Copyright 2010 Benjamin Port <port.benjamin@gmail.com>

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

#ifndef QTHELPPROVIDERABSTRACT_H
#define QTHELPPROVIDERABSTRACT_H

#include <interfaces/idocumentationprovider.h>
#include <QObject>
#include <QVariantList>
#include <QHelpEngine>

class HelpNetworkAccessManager;

class QtHelpProviderAbstract : public QObject, public KDevelop::IDocumentationProvider
{
    Q_OBJECT
    Q_INTERFACES( KDevelop::IDocumentationProvider )
public:
    QtHelpProviderAbstract(QObject *parent, const QString &collectionFileName, const QVariantList & args);
    ~QtHelpProviderAbstract() override;
    KDevelop::IDocumentation::Ptr documentationForDeclaration (KDevelop::Declaration*) const override;

    KDevelop::IDocumentation::Ptr documentationForIndex(const QModelIndex& idx) const override;
    QAbstractItemModel* indexModel() const override;

    KDevelop::IDocumentation::Ptr homePage() const override;

    /// @return False in case we failed to load any documentation files, else true
    bool isValid() const;

    QHelpEngine* engine() { return &m_engine; }

    HelpNetworkAccessManager* networkAccess() const;
public Q_SLOTS:
    void jumpedTo(const QUrl& newUrl);
protected:
    QHelpEngine m_engine;
    HelpNetworkAccessManager* const m_nam;
};

#endif // QTHELPPROVIDERABSTRACT_H
