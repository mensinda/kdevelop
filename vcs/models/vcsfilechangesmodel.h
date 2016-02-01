/*  This file is part of KDevelop
    Copyright 2010 Aleix Pol <aleixpol@kde.org>

    Split into separate class
    Copyright 2011 Andrey Batyiev <batyiev@gmail.com>

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

#ifndef KDEVPLATFORM_VCSFILECHANGESMODEL_H
#define KDEVPLATFORM_VCSFILECHANGESMODEL_H

#include <QStandardItemModel>

#include <vcs/vcsstatusinfo.h>

#include <vcs/vcsexport.h>

class QUrl;

namespace KDevelop
{
class VcsStatusInfo;

/**
 * This class holds and represents information about changes in files.
 * Also it is possible to provide tree like models by inheriting this class, see protected members.
 * All stuff should be pulled in by @p updateState.
 */

class VcsFileChangesModelPrivate;

class KDEVPLATFORMVCS_EXPORT VcsFileChangesModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum ItemRoles { VcsStatusInfoRole = Qt::UserRole+1, UrlRole, LastItemRole };

    /**
     * Constructor for class.
     * @param isCheckable if true, model will show checkboxes on items.
     */
    explicit VcsFileChangesModel(QObject *parent, bool isCheckable = false);
    ~VcsFileChangesModel() override;

    QVariant data(const QModelIndex &index, int role) const override;

    /**
     * Returns list of currently checked statuses.
     */
    QList<VcsStatusInfo> checkedStatuses() const {
        return checkedStatuses(invisibleRootItem());
    }

    /**
     * Returns list of currently checked urls.
     */
    QList<QUrl> checkedUrls() const {
        return checkedUrls(invisibleRootItem());
    }

    /**
     * Returns urls of all files
     * */
    QList<QUrl> urls() const {
        return urls(invisibleRootItem());
    }
    
    /**
     * Set the checked urls
     * */
    void setCheckedUrls(const QList<QUrl>& urls) const {
        return checkUrls(invisibleRootItem(), urls);
    }

    /**
     * Changes the check-state of all files to the given state
     * */
    void setAllChecked(bool checked);

    /**
     * Simple helper to get VcsStatusInfo.
     */
    VcsStatusInfo statusInfo(int row, const QModelIndex &parent) const;
    VcsStatusInfo statusInfo(const QModelIndex &idx) const { return statusInfo(idx.row(), idx.parent()); }

    void setIsCheckbable(bool checkable);
    bool isCheckable() const;

    QModelIndex indexForUrl(const QUrl& url) const;
    bool removeUrl(const QUrl& url);

public slots:
    /**
     * Used to post update of status of some file. Any status except UpToDate
     * and Unknown will update (or add) item representation.
     */
    void updateState(const KDevelop::VcsStatusInfo &status) {
        updateState(invisibleRootItem(), status);
    }

protected:
    /**
     * Post update of status of some file.
     * @return changed row or -1 if row is deleted
     */
    int updateState(QStandardItem *parent, const KDevelop::VcsStatusInfo &status);

    /**
     * Returns list of currently checked statuses.
     */
    QList<VcsStatusInfo> checkedStatuses(QStandardItem *parent) const;

    /**
     * Returns list of currently checked urls.
     */
    QList<QUrl> checkedUrls(QStandardItem *parent) const;
    
    /**
     * Checks the given urls, unchecks all others.
     * */
    void checkUrls(QStandardItem *parent, const QList<QUrl>& urls) const;
    
    /**
     * Returns all urls
     * */
    QList<QUrl> urls(QStandardItem *parent) const;

    /**
     * Returns item for particular url.
     */
    QStandardItem* fileItemForUrl(QStandardItem *parent, const QUrl &url) const;
    QModelIndex indexForUrl(const QModelIndex& parent, const QUrl &url) const;

private:
    QScopedPointer<VcsFileChangesModelPrivate> const d;
};
}

#endif // KDEVPLATFORM_VCSFILECHANGESMODEL_H
