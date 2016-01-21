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

#ifdef NATRON_USE_BREAKPAD

#include "ExistenceCheckThread.h"

#include <QLocalSocket>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>

#include "Engine/AppManager.h"

//This will check every X ms whether the crash reporter process still exist when
//Natron was spawned from the crash reporter process
#define NATRON_BREAKPAD_CHECK_FOR_CRASH_REPORTER_EXISTENCE_MS 10000

//After this time, we consider that the crash reporter is dead
#define NATRON_BREAKPAD_WAIT_FOR_CRASH_REPORTER_ACK_MS 5000

NATRON_NAMESPACE_ENTER;

struct ExistenceCheckerThreadPrivate
{
    boost::shared_ptr<QLocalSocket> socket;
    QString comServerPipePath;
    QString checkMessage;
    QString acknowledgementMessage;
    mutable QMutex mustQuitMutex;
    bool mustQuit;
    QWaitCondition mustQuitCond;
    
    ExistenceCheckerThreadPrivate(const QString& checkMessage,
                                  const QString& acknowledgementMessage,
                                  const QString &comServerPipePath)
    : socket()
    , comServerPipePath(comServerPipePath)
    , checkMessage(checkMessage)
    , acknowledgementMessage(acknowledgementMessage)
    , mustQuitMutex()
    , mustQuit(false)
    , mustQuitCond()
    {

    }
};

ExistenceCheckerThread::ExistenceCheckerThread(const QString& checkMessage,
                                               const QString& acknowledgementMessage,
                                               const QString &comServerPipePath)
: QThread()
, _imp(new ExistenceCheckerThreadPrivate(checkMessage, acknowledgementMessage,comServerPipePath))
{
    setObjectName("CrashReporterAliveThread");
}

ExistenceCheckerThread::~ExistenceCheckerThread()
{
    
}


void
ExistenceCheckerThread::quitThread()
{
    if (!isRunning()) {
        return;
    }
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
    wait();
}


void
ExistenceCheckerThread::run() 
{
    _imp->socket.reset(new QLocalSocket());
    _imp->socket->connectToServer(_imp->comServerPipePath, QLocalSocket::ReadWrite);

    if (!_imp->socket->waitForConnected()) {
        qDebug() << "Failed to connect local socket to " << _imp->comServerPipePath;
        return;
    }

    for (;;) {
        
        {
            QMutexLocker k(&_imp->mustQuitMutex);
            if (_imp->mustQuit) {
                _imp->mustQuit = false;
                _imp->mustQuitCond.wakeAll();
                return;
            }
        }

        {
            QString tosend(_imp->checkMessage);
            tosend.push_back('\n');
            _imp->socket->write(tosend.toStdString().c_str());
        }
        _imp->socket->flush();
        
        bool receivedAcknowledgement = false;
        while (_imp->socket->waitForReadyRead(NATRON_BREAKPAD_WAIT_FOR_CRASH_REPORTER_ACK_MS)) {

            //we received something, if it's not the ackknowledgement packet, recheck
            QString str = _imp->socket->readLine();
            while (str.endsWith('\n')) {
                str.chop(1);
            }
            if (str == _imp->acknowledgementMessage) {
                receivedAcknowledgement = true;
                break;
            }
        }
        
        if (!receivedAcknowledgement) {
            qDebug() << "Crash reporter process does not seem to be responding anymore...exiting";
            /*
             We did not receive te acknowledgement, hence quit
             */
            appPTR->abortAnyProcessing();
            Q_EMIT otherProcessUnreachable();
            return;
        }
        
        //Sleep until we need to check again
        msleep(NATRON_BREAKPAD_CHECK_FOR_CRASH_REPORTER_EXISTENCE_MS);
        
    } // for(;;)
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ExistenceCheckThread.cpp"

#endif // NATRON_USE_BREAKPAD
