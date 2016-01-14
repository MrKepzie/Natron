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

#ifndef _CrashReporter_CrashDialog_h_
#define _CrashReporter_CrashDialog_h_

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

class QVBoxLayout;
class QLabel;
class QTextEdit;
class QGridLayout;
class QPushButton;
class QHBoxLayout;
class QFrame;
class PlaceHolderTextEdit;

class CrashDialog : public QDialog
{    
    Q_OBJECT
    
public:
    enum UserChoice {
        eUserChoiceUpload,
        eUserChoiceSave,
        eUserChoiceIgnore
    };

    CrashDialog(const QString& filePath);

    virtual ~CrashDialog();

    QString getDescription() const;

    UserChoice getUserChoice() const;
    
    const QString& getOriginalDumpFilePath() const
    {
        return _filePath;
    }
    
public Q_SLOTS:
    
    void onSendClicked();
    
    void onDontSendClicked();
    
    void onSaveClicked();
    
private:
    
    QString _filePath;
    
    QVBoxLayout* _mainLayout;
    QFrame* _mainFrame;
    QGridLayout* _gridLayout;
    QLabel* _iconLabel;
    QLabel* _infoLabel;
    QLabel* _refLabel;
    QLabel* _refContent;
    QLabel* _descLabel;
    PlaceHolderTextEdit* _descEdit;
    QFrame* _buttonsFrame;
    QHBoxLayout* _buttonsLayout;
    QPushButton* _sendButton;
    QPushButton* _dontSendButton;
    QPushButton* _saveReportButton;
    QPushButton* _pressedButton;
};

#endif // _CrashReporter_CrashDialog_h_
