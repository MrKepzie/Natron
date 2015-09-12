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

#ifndef _Engine_CLArgs_h_
#define _Engine_CLArgs_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <string>
#include "Global/GlobalDefines.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QStringList>
#include <QtCore/QString>
CLANG_DIAG_ON(deprecated)

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
//#include <boost/noncopyable.hpp>
#endif

struct CLArgsPrivate;
class CLArgs //: boost::noncopyable // GCC 4.2 requires the copy constructor
{
public:
    
    struct WriterArg
    {
        QString name;
        QString filename;
        bool mustCreate;
        
        WriterArg()
        : name(), filename(), mustCreate(false)
        {
            
        }
    };
    
    CLArgs();
    
    CLArgs(int& argc,char* argv[],bool forceBackground);

    CLArgs(const QStringList& arguments, bool forceBackground);
    
    CLArgs(const CLArgs& other); // GCC 4.2 requires the copy constructor
    
    ~CLArgs();

    void operator=(const CLArgs& other);
    
    bool isEmpty() const;
    
    static void printBackGroundWelcomeMessage();
    
    static void printUsage(const std::string& programName);
    
    int getError() const;
    
    const std::list<CLArgs::WriterArg>& getWriterArgs() const;
    
    bool hasFrameRange() const;
    
    const std::pair<int,int>& getFrameRange() const;
    
    bool isBackgroundMode() const;
    
    bool isInterpreterMode() const;
    
    const QString& getFilename() const;
    
    const QString& getDefaultOnProjectLoadedScript() const;
    
    const QString& getIPCPipeName() const;
    
    bool isPythonScript() const;
    
    bool areRenderStatsEnabled() const;
    
private:
    
    boost::scoped_ptr<CLArgsPrivate> _imp;
};


#endif // _Engine_CLArgs_h_

