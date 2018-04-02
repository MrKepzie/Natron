/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "ProjectPrivate.h"

#include <list>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/NodeSerialization.h"
#include "Serialization/ProjectSerialization.h"


NATRON_NAMESPACE_ENTER

ProjectPrivate::ProjectPrivate(Project* project)
    : _publicInterface(project)
    , projectLock()
    , hasProjectBeenSavedByUser(false)
    , ageSinceLastSave( QDateTime::currentDateTime() )
    , lastAutoSave()
    , builtinFormats()
    , additionalFormats()
    , formatMutex(QMutex::Recursive)
    , envVars()
    , projectName()
    , projectPath()
    , formatKnob()
    , addFormatKnob()
    , previewMode()
    , colorSpace8u()
    , colorSpace16u()
    , colorSpace32f()
    , natronVersion()
    , originalAuthorName()
    , lastAuthorName()
    , projectCreationDate()
    , saveDate()
    , onProjectLoadCB()
    , onProjectSaveCB()
    , onProjectCloseCB()
    , onNodeCreated()
    , onNodeDeleted()
    , timeline()
    , autoSetProjectFormat( appPTR->getCurrentSettings()->isAutoProjectFormatEnabled() )
    , isLoadingProjectMutex()
    , isLoadingProject(false)
    , isLoadingProjectInternal(false)
    , isSavingProjectMutex()
    , isSavingProject(false)
    , autoSaveTimer()
    , projectClosing(false)
    , tlsData()

{
    timeline = boost::make_shared<TimeLine>(project);
    autoSaveTimer = boost::make_shared<QTimer>();
    tlsData = boost::make_shared<TLSHolder<Project::ProjectTLSData> >();

    autoSaveTimer->setSingleShot(true);
}


bool
ProjectPrivate::findFormat(int index,
                           Format* format) const
{
    if ( index >= (int)( builtinFormats.size() + additionalFormats.size() ) ) {
        return false;
    }

    int i = 0;
    if ( index >= (int)builtinFormats.size() ) {
        ///search in the additional formats
        index -= builtinFormats.size();

        for (std::list<Format>::const_iterator it = additionalFormats.begin(); it != additionalFormats.end(); ++it) {
            if (i == index) {
                assert( !it->isNull() );
                *format = *it;

                return true;
            }
            ++i;
        }
    } else {
        ///search in the builtins formats
        for (std::list<Format>::const_iterator it = builtinFormats.begin(); it != builtinFormats.end(); ++it) {
            if (i == index) {
                assert( !it->isNull() );
                *format = *it;

                return true;
            }
            ++i;
        }
    }

    return false;
}

void
ProjectPrivate::autoSetProjectDirectory(const QString& path)
{
    std::string pathCpy = path.toStdString();

    if ( !pathCpy.empty() && (pathCpy[pathCpy.size() - 1] == '/') ) {
        pathCpy.erase(pathCpy.size() - 1, 1);
    }
    std::string env;
    try {
        env = envVars->getValue();
    } catch (...) {
        // ignore
    }

    std::list<std::vector<std::string> > table;
    envVars->decodeFromKnobTableFormat(env, &table);

    ///If there was already a OCIO variable, update it, otherwise create it
    bool foundProject = false;
    for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it) {
        if ( (*it)[0] == NATRON_PROJECT_ENV_VAR_NAME ) {
            (*it)[1] = pathCpy;
            foundProject = true;
            break;
        }
    }
    if (!foundProject) {
        std::vector<std::string> vec(2);
        vec[0] = NATRON_PROJECT_ENV_VAR_NAME;
        vec[1] = pathCpy;
        table.push_back(vec);
    }


    std::string newEnv = envVars->encodeToKnobTableFormat(table);
    if (env != newEnv) {
        if ( appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled() ) {
            _publicInterface->fixRelativeFilePaths(NATRON_PROJECT_ENV_VAR_NAME, pathCpy, false);
        }
        envVars->setValue(newEnv);
    }
}

std::string
ProjectPrivate::runOnProjectSaveCallback(const std::string& filename,
                                         bool autoSave)
{
    std::string onProjectSave = _publicInterface->getOnProjectSaveCB();

    if ( !onProjectSave.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(onProjectSave, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onProjectSave callback: ")
                                                              + e.what() );

            return filename;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + error);

            return filename;
        } else {
            std::string signatureError;
            signatureError.append("The on project save callback supports the following signature(s):\n");
            signatureError.append("- callback(filename,app,autoSave)");
            if (args.size() != 3) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + signatureError);

                return filename;
            }
            if ( (args[0] != "filename") || (args[1] != "app") || (args[2] != "autoSave") ) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + signatureError);

                return filename;
            }
            std::string appID = _publicInterface->getApp()->getAppIDString();
            std::stringstream ss;
            if (appID != "app") {
                ss << "app = " << appID << "\n";
            }
            ss << "ret = " << onProjectSave << "(\"" << filename << "\"," << appID << ",";
            if (autoSave) {
                ss << "True)\n";
            } else {
                ss << "False)\n";
            }

            onProjectSave = ss.str();
            std::string err;
            std::string output;
            if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(onProjectSave, &err, &output) ) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + err);

                return filename;
            } else {
                PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
                assert(mainModule);
                PyObject* ret = PyObject_GetAttrString(mainModule, "ret");
                if (!ret) {
                    return filename;
                }
                std::string filePath = filename;
                if (ret) {
                    filePath = NATRON_PYTHON_NAMESPACE::PyStringToStdString(ret);
                    std::string script = "del ret\n";
                    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
                    assert(ok);
                    if (!ok) {
                        throw std::runtime_error("ProjectPrivate::runOnProjectSaveCallback(): interpretPythonScript(" + script + ") failed!");
                    }
                }
                if ( !output.empty() ) {
                    _publicInterface->getApp()->appendToScriptEditor(output);
                }

                return filePath;
            }
        }
    }

    return filename;
} // ProjectPrivate::runOnProjectSaveCallback

void
ProjectPrivate::runOnProjectCloseCallback()
{
    std::string onProjectClose = _publicInterface->getOnProjectCloseCB();

    if ( !onProjectClose.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(onProjectClose, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onProjectClose callback: ")
                                                              + e.what() );

            return;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The on project close callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if (args.size() != 1) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + signatureError);

            return;
        }
        if (args[0] != "app") {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + signatureError);

            return;
        }
        std::string appID = _publicInterface->getApp()->getAppIDString();
        std::string script;
        if (appID != "app") {
            script = script +  "app = " + appID;
        }
        script = script + "\n" + onProjectClose + "(" + appID + ")\n";
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + err);
        } else {
            if ( !output.empty() ) {
                _publicInterface->getApp()->appendToScriptEditor(output);
            }
        }
    }
} // ProjectPrivate::runOnProjectCloseCallback

void
ProjectPrivate::runOnProjectLoadCallback()
{
    std::string cb = _publicInterface->getOnProjectLoadCB();

    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onProjectLoaded callback: ")
                                                              + e.what() );

            return;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The on  project loaded callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if (args.size() != 1) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + signatureError);

            return;
        }
        if (args[0] != "app") {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + signatureError);

            return;
        }

        std::string appID = _publicInterface->getApp()->getAppIDString();
        std::string script;
        if (appID != "app") {
            script =  script + "app = " + appID;
        }
        script =  script + "\n" + cb + "(" + appID + ")\n";
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + err);
        } else {
            if ( !output.empty() ) {
                _publicInterface->getApp()->appendToScriptEditor(output);
            }
        }
    }
} // ProjectPrivate::runOnProjectLoadCallback

void
ProjectPrivate::setProjectFilename(const std::string& filename)
{
    projectName->setValue(filename);
}

std::string
ProjectPrivate::getProjectFilename() const
{
    return projectName->getValue();
}

void
ProjectPrivate::setProjectPath(const std::string& path)
{
    projectPath->setValue(path);
}

std::string
ProjectPrivate::getProjectPath() const
{
    return projectPath->getValue();
}

NATRON_NAMESPACE_EXIT
