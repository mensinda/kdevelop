/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KDEVPLATFORM_PLUGIN_EDITORSVIEW_H
#define KDEVPLATFORM_PLUGIN_EDITORSVIEW_H

#include <QDialog>
#include <QMultiMap>
#include <KJob>

#include "ui_editorsview.h"

class CvsPlugin;
class CvsJob;

/**
 * This is a helper class for the EditorsView::parseOutput() method.
 * It holds information about a single locker of a file.
 * @see EditorsView::parseOutput()
 */
class CvsLocker {
public:
    QString user;
    QString date;
    QString machine;
    QString localrepo;
};

/**
 * Shows the output from @code cvs editors @endcode in a nice way.
 * Create a CvsJob by calling CvsProxy::editors() and connect the job's
 * result(KJob*) signal to EditorsView::slotJobFinished(KJob* job)
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 */
class EditorsView : public QWidget, private Ui::EditorsViewBase
{
Q_OBJECT
public:
    explicit EditorsView(CvsJob* job=nullptr, QWidget *parent = nullptr);
    ~EditorsView() override;

    /**
     * Parses the output generated by a <tt>cvs editors</tt> command and
     * fills the given QMultiMap with all files and their lockers found in the output.
     * @param jobOutput Pass in the plain output of a <tt>cvs editors</tt> job
     * @param editorsInfo This QMultiMap will be filled with information about which files
     *                    are locked by whom. The key of the map is the filename. For each
     *                    filename a list of CvsLocker objects will be created, depending
     *                    on how many people are editing the file.
     *                    If editorsInfo.size() is zero, this means that no information was
     *                    found in the given @p jobOutput.
     */
    static void parseOutput(const QString& jobOutput,
                            QMultiMap<QString,CvsLocker>& editorsInfo);

private Q_SLOTS:
    /**
     * Connect a job's result() signal to this slot. When called, the output from the job
     * will be passed to the parseOutput() method and any found locker information will be
     * displayed.
     * @note If you pass a CvsJob object to the ctor, it's result() signal
     *       will automatically be connected to this slot.
     */
    void slotJobFinished(KJob* job);

private:
    QString m_output;
};

#endif
