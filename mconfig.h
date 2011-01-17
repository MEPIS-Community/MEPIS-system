//
//   Copyright (C) 2003-2008 by Warren Woodford
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

#ifndef MCONFIG_H
#define MCONFIG_H

#include "ui_meconfig.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <qlistview.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qregexp.h>
#include <qfile.h>
#include <qlineedit.h>
#include <qprocess.h>
#include <qtimer.h>
#include <qfiledialog.h>

class MConfig : public QDialog, public Ui::MEConfig {
  Q_OBJECT
  protected:
    QProcess *proc;
    QTimer *timer;

  public:
  MConfig(QWidget* parent = 0);
    ~MConfig();
    // helpers
    static QString getCmdOut(QString cmd);
    static QStringList getCmdOuts(QString cmd);
    static QString getCmdValue(QString cmd, QString key, QString keydel, QString valdel);
    static QStringList getCmdValues(QString cmd, QString key, QString keydel, QString valdel);
    static bool replaceStringInFile(QString oldtext, QString newtext, QString filepath);
    bool mountPartition(QString dev, const char *point);
    // common
    void refresh();
    void refreshOptions();
    void refreshUSB();
    void refreshGrub();
    void refreshPartition();

    void applyOptions();
    void applyUSB();
    void makeOnTheGo();
    void applyGrub();
    bool makeGrub(QString rootpart, const char *rootmnt, bool initrd);
    void applyPartition();

  public slots:

    void formatStart();
    void formatTime();
    void eraseDone(int exitCode, QProcess::ExitStatus exitStatus);
    void formatDone(int exitCode, QProcess::ExitStatus exitStatus);
    void bootableDone(int exitCode, QProcess::ExitStatus exitStatus);
    void isoDone(int exitCode, QProcess::ExitStatus exitStatus);
    void procDone(int exitCode, QProcess::ExitStatus exitStatus);

    void partitionStdout();
    void partitionStderr();
    void partitionDone(int exitCode, QProcess::ExitStatus exitStatus);

    virtual void show();
    virtual void on_selectFilePushButton_clicked();
    virtual void on_repairBlocksCheckBox_clicked();
    virtual void on_repairTestCheckBox_clicked();
    virtual void on_repairDiskComboBox_activated();
    virtual void on_repairPartitionComboBox_activated();
    virtual void on_grubInstallComboBox_activated();
    virtual void on_tabWidget_currentChanged();
    virtual void on_keyboardCombo_activated();
    virtual void on_localeCombo_activated();
    virtual void on_computerNameEdit_textEdited();
    virtual void on_computerDomainEdit_textEdited();
    virtual void on_computerGroupEdit_textEdited();
    virtual void on_bootClearCacheCheckBox_clicked();
    virtual void on_bootClearLogsCheckBox_clicked();
    virtual void on_clearLogsCheckBox_clicked();
    virtual void on_checkLanguageLocale_clicked();
    virtual void on_checkKeyboardLayout_clicked();
    virtual void on_checkSambaWorkgroup_clicked();
    virtual void on_checkComputerDomain_clicked();
    virtual void on_checkComputerName_clicked();
    virtual void on_buttonApply_clicked();
    virtual void on_buttonCancel_clicked();
    virtual void on_buttonOk_clicked();
    virtual void on_buttonAbout_clicked();
    virtual void on_generalHelpPushButton_clicked();

  protected:
    QString oldName, oldDomain, oldGroup;

  private:
    static bool hasInternetConnection();
    static void executeChild(const char* cmd, const char* param);

  protected slots:
    /*$PROTECTED_SLOTS$*/

};

#endif

