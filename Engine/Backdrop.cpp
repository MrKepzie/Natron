/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Backdrop.h"
#include "Engine/Transform.h"
#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER;

struct BackdropPrivate
{
    
    boost::weak_ptr<KnobString> knobLabel;
    
    BackdropPrivate()
    : knobLabel()
    {
    }
    
};

Backdrop::Backdrop(NodePtr node)
: NoOpBase(node)
, _imp(new BackdropPrivate())
{
}

Backdrop::~Backdrop()
{
    
}

std::string
Backdrop::getPluginDescription() const
{
    return QObject::tr("The node backdrop is useful to group nodes and identify them in the node graph. You can also "
              "move all the nodes inside the backdrop.").toStdString() ;
}

void
Backdrop::initializeKnobs()
{
    boost::shared_ptr<KnobPage> page = AppManager::createKnob<KnobPage>(this, "Controls");
    
    boost::shared_ptr<KnobString> knobLabel = AppManager::createKnob<KnobString>( this, "Label");
    knobLabel->setAnimationEnabled(false);
    knobLabel->setAsMultiLine();
    knobLabel->setUsesRichText(true);
    knobLabel->setHintToolTip( QObject::tr("Text to display on the backdrop.").toStdString() );
    knobLabel->setEvaluateOnChange(false);
    page->addKnob(knobLabel);
    _imp->knobLabel = knobLabel;
 

}


void
Backdrop::knobChanged(KnobI* k,
                      ValueChangedReasonEnum /*reason*/,
                      int /*view*/,
                      double /*time*/,
                      bool /*originatedFromMainThread*/)
{
    if ( k == _imp->knobLabel.lock().get() ) {
        QString text( _imp->knobLabel.lock()->getValue().c_str() );
        Q_EMIT labelChanged(text);
    } 
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Backdrop.cpp"
