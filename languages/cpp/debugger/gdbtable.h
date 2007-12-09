/***************************************************************************
*   Copyright (C) 2003 by Alexander Dymo                                  *
*   cloudtemple@mksat.net                                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
***************************************************************************/
#ifndef GDBDEBUGGERGDBTABLE_H
#define GDBDEBUGGERGDBTABLE_H

#include <q3table.h>
#include <QKeyEvent>

namespace GDBDebugger {

class GDBTable : public Q3Table
{
Q_OBJECT
public:
    GDBTable(QWidget *parent = 0);
    GDBTable( int numRows, int numCols, QWidget * parent = 0 );
    ~GDBTable();

    virtual void keyPressEvent ( QKeyEvent * e );

Q_SIGNALS:
    void keyPressed(int key);

    void returnPressed();
    void f2Pressed();
    void insertPressed();
    void deletePressed();
};

}

#endif

