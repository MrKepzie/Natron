/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef _Gui_AddKnobDialog_h_
#define _Gui_AddKnobDialog_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(uninitialized)

class KnobI;
class DockablePanel;

struct AddKnobDialogPrivate;
class AddKnobDialog : public QDialog
{
    Q_OBJECT
public:
    
    AddKnobDialog(DockablePanel* panel,const boost::shared_ptr<KnobI>& knob, QWidget* parent);
    
    virtual ~AddKnobDialog();
    
    boost::shared_ptr<KnobI> getKnob() const;

public Q_SLOTS:
    
    void onPageCurrentIndexChanged(int index);
    
    void onTypeCurrentIndexChanged(int index);
    
    void onOkClicked();
private:
    
    boost::scoped_ptr<AddKnobDialogPrivate> _imp;
};
#endif // _Gui_AddKnobDialog_h_
