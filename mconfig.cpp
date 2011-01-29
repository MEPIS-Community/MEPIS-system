//
//   Copyright (C) 2003-2010 by Warren Woodford
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

#include "mconfig.h"

MConfig::MConfig(QWidget* parent) : QDialog(parent) {
  setupUi(this);
  setWindowIcon(QApplication::windowIcon());

  proc = new QProcess(this);
  timer = new QTimer(this);

  // if an apple...
  if (system("hal-get-property --udi /org/freedesktop/Hal/devices/computer --key system.firmware.vendor | grep 'Apple'") == 0) {
    grubRootRadioButton->setChecked(true);
    grubMBRRadioButton->setEnabled(false);
  }

  tabWidget->setCurrentIndex(0);
}

MConfig::~MConfig(){
}

/////////////////////////////////////////////////////////////////////////
// util functions

QString MConfig::getCmdOut(QString cmd) {
  char line[260];
  const char* ret = "";
  FILE* fp = popen(cmd.toAscii(), "r");
  if (fp == NULL) {
    return QString (ret);
  }
  int i;
  if (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    ret = line;
  }
  pclose(fp);
  return QString (ret);
}

QStringList MConfig::getCmdOuts(QString cmd) {
  char line[260];
  FILE* fp = popen(cmd.toAscii(), "r");
  QStringList results;
  if (fp == NULL) {
    return results;
  }
  int i;
  while (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    results.append(line);
  }
  pclose(fp);
  return results;
}

QString MConfig::getCmdValue(QString cmd, QString key, QString keydel, QString valdel) {
  const char *ret = "";
  char line[260];

  QStringList strings = getCmdOuts(cmd);
  for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
    strcpy(line, ((QString)*it).toAscii());
    char* keyptr = strstr(line, key.toAscii());
    if (keyptr != NULL) {
      // key found
      strtok(keyptr, keydel.toAscii());
      const char* val = strtok(NULL, valdel.toAscii());
      if (val != NULL) {
        ret = val;
      }
      break;
    }
  }
  return QString (ret);
}

QStringList MConfig::getCmdValues(QString cmd, QString key, QString keydel, QString valdel) {
  char line[130];
  FILE* fp = popen(cmd.toAscii(), "r");
  QStringList results;
  if (fp == NULL) {
    return results;
  }
  int i;
  while (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    char* keyptr = strstr(line, key.toAscii());
    if (keyptr != NULL) {
      // key found
      strtok(keyptr, keydel.toAscii());
      const char* val = strtok(NULL, valdel.toAscii());
      if (val != NULL) {
        results.append(val);
      }
    }
  }
  pclose(fp);
  return results;
}

bool MConfig::replaceStringInFile(QString oldtext, QString newtext, QString filepath) {

QString cmd = QString("sed -i 's/%1/%2/g' %3").arg(oldtext).arg(newtext).arg(filepath);
  if (system(cmd.toAscii()) != 0) {
    return false;
  }
  return true;
}

bool MConfig::mountPartition(QString dev, const char *point) {

  mkdir(point, 0755);
  QString cmd = QString("/bin/mount %1 %2").arg(dev).arg(point);
  if (system(cmd.toAscii()) != 0) {
    return false;
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////
// common


void MConfig::refresh() {
  int i = tabWidget->currentIndex();
  switch (i) {
    case 1:
      refreshUSB();
      buttonOk->setEnabled(false);
      break;

    case 2:
      refreshGrub();
      buttonOk->setEnabled(true);
      break;

    case 3:
      refreshPartition();
      buttonOk->setEnabled(false);
      break;

    default:
      refreshOptions();
      buttonApply->setEnabled(false);
      buttonOk->setEnabled(true);
      break;
  }
}

void MConfig::refreshOptions() {
  char line[130];
  char *tok;
  FILE *fp;
  int i;
  QString val;

  checkComputerName->setChecked(false);
  checkComputerDomain->setChecked(false);
  checkSambaWorkgroup->setChecked(false);
  checkKeyboardLayout->setChecked(false);
  checkLanguageLocale->setChecked(false);

// keyboard
  system("ls -1 /usr/share/keymaps/i386/azerty > /tmp/mlocale");
  system("ls -1 /usr/share/keymaps/i386/qwerty >> /tmp/mlocale");
  system("ls -1 /usr/share/keymaps/i386/qwertz >> /tmp/mlocale");
  system("ls -1 /usr/share/keymaps/i386/dvorak >> /tmp/mlocale");
  system("ls -1 /usr/share/keymaps/i386/fgGIod >> /tmp/mlocale");
  keyboardCombo->clear();
  keyboardCombo->insertItem(-1, "no change");
  fp = popen("sort /tmp/mlocale", "r");
  if (fp != NULL) {
    while (fgets(line, sizeof line, fp) != NULL) {
      i = strlen(line) - 9;
      line[i] = '\0';
     if (line != NULL && strlen(line) > 1) {
        keyboardCombo->addItem(line);
      }
    }
    pclose(fp);
  }

  // locale
  localeCombo->clear();
  fp = popen("/usr/bin/locale -a", "r");
  if (fp != NULL) {
    while (fgets(line, sizeof line, fp) != NULL) {
      i = strlen(line);
      line[--i] = '\0';
      tok = strtok(line, " ");
     if (tok != NULL && strlen(tok) > 1 && strncmp(tok, "#", 1) != 0) {
        localeCombo->addItem(tok);
      }
    }
    pclose(fp);
  }
  val = getCmdValue("cat /etc/default/locale", "LANG", " =", " ");
  if (!val.isEmpty()) {
    localeCombo->setCurrentIndex(localeCombo->findText(val.toAscii()));
  } else {
    localeCombo->setCurrentIndex(0);
  }

  oldName = getCmdOut("cat /etc/hostname");
  computerNameEdit->setText(oldName);
  oldDomain = getCmdOut("cat /etc/defaultdomain");
  computerDomainEdit->setText(oldDomain);
  oldGroup = getCmdValue("grep 'workgroup =' /etc/samba/smb.conf", "=", " ", " ");
  computerGroupEdit->setText(oldGroup);

  val = getCmdValue("cat /etc/default/mepis", "EMPTY_LOGS", " =", " ");
  bootClearLogsCheckBox->setChecked(val.localeAwareCompare("yes") == 0);

  val = getCmdValue("cat /etc/default/mepis", "EMPTY_CACHE", " =", " ");
  bootClearCacheCheckBox->setChecked(val.localeAwareCompare("yes") == 0);

}

void MConfig::refreshUSB() {
  // sd devices only
  QString cmd, mnt;
  formatDiskComboBox->clear();
  int rcount = 0;
  QStringList vals = getCmdOuts("cat /proc/partitions | grep 'sd[a-z]$'");
  QStringList::iterator it;
  if (!vals.isEmpty()) {
    for (it = vals.begin() ;it != vals.end() ;it++) {
      QString val = (*it);
      QRegExp ex("\\s+");
      mnt = val.section(ex, 4, 4);
      cmd = QString("cat /etc/fstab | grep '^/dev/%1'").arg(mnt);
      if (system(cmd.toAscii()) != 0) {
        // if not in fstab, should be plugged not fixed
        cmd = QString("cat /proc/mounts | grep '^/dev/%1'").arg(mnt);
        if (system(cmd.toAscii()) != 0) {
          // not mounted, can be used
          formatDiskComboBox->addItem(mnt);
          rcount++;
        }
      }
    }
  }

  if (rcount > 0) {
    formatDiskComboBox->setCurrentIndex(rcount-1);
    buttonApply->setEnabled(true);
  } else {
    formatDiskComboBox->addItem("none");
    buttonApply->setEnabled(false);
  }
  formatStatusEdit->setText("");
}

void MConfig::refreshGrub() {
  char line[130];

  FILE *fp = popen("cat /proc/partitions | grep '[h,s,v].[a-z]$' | sort --key=4 2>/dev/null", "r");
  if (fp == NULL) {
    return;
  }
  grubBootComboBox->clear();
  char *ndev, *nsz;
  int i, nsize;
  while (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    strtok(line, " \t");
    strtok(NULL, " \t");
    nsz = strtok(NULL, " \t");
    ndev = strtok(NULL, " \t");
    if (ndev != NULL && strlen(ndev) == 3) {
      nsize = atoi(nsz) / 1024;
      if (nsize > 1000) {
        sprintf(line, "%s", ndev);
        grubBootComboBox->addItem(line);
        grubInstallComboBox->addItem(ndev);
      }
    }
  }
  pclose(fp);
  grubBootComboBox->setCurrentIndex(0);
  grubInstallComboBox->setCurrentIndex(0);
  on_grubInstallComboBox_activated();
}

void MConfig::refreshPartition() {
  char line[130];

  FILE *fp = popen("cat /proc/partitions | grep '[h,s,v].[a-z]$' | sort --key=4 2>/dev/null", "r");
  if (fp == NULL) {
    return;
  }
  repairDiskComboBox->clear();
  char *ndev, *nsz;
  int i, nsize;
  while (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    strtok(line, " \t");
    strtok(NULL, " \t");
    nsz = strtok(NULL, " \t");
    ndev = strtok(NULL, " \t");
    if (ndev != NULL && strlen(ndev) == 3) {
      nsize = atoi(nsz) / 1024;
        sprintf(line, "%s", ndev);
        repairDiskComboBox->addItem(line);
    }
  }
  pclose(fp);
  on_repairDiskComboBox_activated();

  repairProgressTextEdit->setText("");
}

void MConfig::applyOptions() {
  QString cmd, cmd2;

  if (checkComputerName->isChecked()) {
    // see if name is reasonable
    if (computerNameEdit->text().length() < 2) {
      QMessageBox::critical(0, QString::null,
        tr("Sorry your computer name needs to be at least 2 characters long. You'll have to select a different name before proceeding."));
      return;
    }
    replaceStringInFile(oldName, computerNameEdit->text(), "/etc/hosts");
    replaceStringInFile(oldName, computerNameEdit->text(), "/etc/samba/smb.conf");
    replaceStringInFile(oldName, computerNameEdit->text(), "/etc/postfix/main.cf");
    cmd = QString("echo \"%1\" | cat > /etc/hostname").arg(computerNameEdit->text());
    system(cmd.toAscii());
    cmd = QString("echo \"%1\" | cat > /etc/mailname").arg(computerNameEdit->text());
    system(cmd.toAscii());  }
    cmd = QString("sed -i 's/.*send host-name.*/send host-name \"%1\";/g' /etc/dhcp3/dhclient.conf").arg(computerNameEdit->text());
    system(cmd.toAscii());
  
  if (checkComputerDomain->isChecked()) {
    // see if name is reasonable
    if (computerDomainEdit->text().length() < 2) {
      QMessageBox::critical(0, QString::null,
        tr("Sorry your computer domain needs to be at least 2 characters long. You'll have to select a different name before proceeding."));
      return;
    }  
    replaceStringInFile(oldDomain, computerDomainEdit->text(), "/etc/dhcp3/dhcpd.conf");
    cmd = QString("echo \"%1\" | cat > /etc/defaultdomain").arg(computerDomainEdit->text());
    system(cmd.toAscii());
  }

  if (checkSambaWorkgroup->isChecked()) {
    // see if name is reasonable
    if (computerGroupEdit->text().length() < 2) {
      QMessageBox::critical(0, QString::null,
        tr("Sorry your workgroup needs to be at least 2 characters long. You'll have to select a different name before proceeding."));
      return;
    }  
    replaceStringInFile(oldGroup, computerGroupEdit->text(), "/etc/samba/smb.conf");
  }

  setCursor(QCursor(Qt::WaitCursor));

  //keyboard
  if (checkKeyboardLayout->isChecked()) {
    QString kb = keyboardCombo->currentText();
    cmd = QString("/usr/sbin/install-keymap \"%1\"").arg(kb);
    system(cmd.toAscii());
    if (kb == "uk") {
      kb = "gb";
    }
    cmd = QString("sed -i 's/XkbLayout.*/XkbLayout\" \"%1\"/g' /etc/X11/xorg.conf").arg(kb);
    system(cmd.toAscii());
    cmd = QString("sed -i 's/LayoutList.*/LayoutList=%1/g' /etc/skel/.kde/share/config/kxkbrc").arg(kb);
    system(cmd.toAscii());
    cmd = QString("sed -i 's/LayoutList.*/LayoutList=%1/g' /root/.kde/share/config/kxkbrc").arg(kb);
    system(cmd.toAscii());
  }

  //locale
  if (checkLanguageLocale->isChecked()) {
    cmd = QString("/usr/sbin/update-locale \"LANG=%1\"").arg(localeCombo->currentText());
    system(cmd.toAscii());
    cmd = QString("Language=%1").arg(localeCombo->currentText());
    replaceStringInFile("Language=.*", cmd, "/etc/kde4/kdm/kdmrc");
  }

  if (clearLogsCheckBox->isChecked()) {
    system("rm -f /var/log/*");
  }

  if (bootClearLogsCheckBox->isChecked()) {
    replaceStringInFile("^EMPTY_LOGS.*", "EMPTY_LOGS=yes", "/etc/default/mepis");
  } else {
    replaceStringInFile("^EMPTY_LOGS.*", "EMPTY_LOGS=no", "/etc/default/mepis");
  }
  
  if (bootClearCacheCheckBox->isChecked()) {
    replaceStringInFile("^EMPTY_CACHE.*", "EMPTY_CACHE=yes", "/etc/default/mepis");
  } else {
    replaceStringInFile("^EMPTY_CACHE.*", "EMPTY_CACHE=no", "/etc/default/mepis");
  }

  refresh();

  setCursor(QCursor(Qt::ArrowCursor));

  QMessageBox::information(0, QString::null,
    tr("Your system changes will take effect when you reboot. If you have overridden the keyboard or locale in KDE, you will not see a change when running KDE."));

  // disable button
  buttonApply->setEnabled(false);
}

void MConfig::applyUSB() {
  QString cmd;
  if (formatRadioButton->isChecked()) {
    // do format only
    int ans = QMessageBox::warning(0, QString::null,
    tr(" If you format the disk, all data on it will be deleted. This can not be reversed. Are you sure you want to do this now?"),
      tr("Yes"), tr("No"));
    if (ans != 0) {
      refresh();
      return;
    }
  } else {
    // making a bootable usb
    int ans = QMessageBox::warning(0, QString::null,
    tr("It will take several minutes to make a bootable USB key. You should do this only if you have a fast USB key that is at least 1 GB in size. Be sure you have selected an appropriate MEPIS ISO source. Are you ready to proceed?"),
    tr("Yes"), tr("No"));
    if (ans != 0) {
      refresh();
      return;
    }     
    // verify source media is available
    if (cdRadioButton->isChecked()) {
      // test for valid cd TODO improve!
      if (system("mount /dev/cdrom") != 0 && system("mount /dev/cdrom1") != 0) {
        QMessageBox::warning(0, QString::null,
          tr("A MEPIS bootable CD was not found in the cdrom drive."));
        system("umount /dev/cdrom");
        system("umount /dev/cdrom1");
        refresh();
        return;
      }
    } else { 
      // test for valid file TODO
      
    }
  }
  
  setCursor(QCursor(Qt::WaitCursor));
  formatStatusEdit->setText(tr("Erase disk..."));
  disconnect(timer, SIGNAL(timeout()), 0, 0);
  connect(timer, SIGNAL(timeout()), this, SLOT(formatTime()));
  disconnect(proc, SIGNAL(started()), 0, 0);
  connect(proc, SIGNAL(started()), this, SLOT(formatStart()));
  disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
  connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(eraseDone(int, QProcess::ExitStatus)));
  cmd = QString("umount /dev/%1").arg(formatDiskComboBox->currentText());
  cmd.append("1");
  system(cmd.toAscii());
  cmd = QString("umount /dev/%1").arg(formatDiskComboBox->currentText());
  cmd.append("4");
  system(cmd.toAscii());
  cmd = QString("dd if=/dev/zero of=/dev/%1 bs=512 count=100").arg(formatDiskComboBox->currentText());
  proc->start(cmd);

}

void MConfig::applyGrub() {
  QString val;
  char line[130];

  strcpy(line, grubRootComboBox->currentText().toAscii());
  char *tok = strtok(line, " -");
  QString cmd = QString("/dev/%1").arg(tok);
  mountPartition(cmd, "/mnt/mepis");

  // replace the initrd.img file
  if (grubInitrdCheckBox->isChecked()) {
    val = getCmdOut("uname -a | grep '2.6'");
    if (!val.isEmpty()) {
      val = getCmdOut("ls /mnt/mepis/boot | grep 'initrd.img-2.6'");
      if (!val.isEmpty()) {
        setCursor(QCursor(Qt::WaitCursor));
        cmd = QString("rm /mnt/mepis/boot/%1").arg(val);
        system(cmd.toAscii());
        system("chroot /mnt/mepis mount /proc");
        cmd = QString("chroot /mnt/mepis mkinitramfs -o /boot/%1").arg(val);
        system(cmd.toAscii());
        system("chroot /mnt/mepis umount /proc");
        setCursor(QCursor(Qt::ArrowCursor));
      }
    }
  }

  QString bootdrv = QString(grubBootComboBox->currentText()).section(" ", 0, 0);
  QString rootpart = QString(grubRootComboBox->currentText()).section(" ", 0, 0);
  QString boot;

  if (grubMBRRadioButton->isChecked()) {
    boot = bootdrv;
  } else {
    boot = rootpart;
  }

  // install Grub?
  QString msg = QString( tr("Ok to install GRUB bootloader at %1 ?")).arg(boot);
  int ans = QMessageBox::warning(this, QString::null, msg,
        tr("Yes"), tr("No"));
  if (ans != 0) {
    system("/bin/umount -l /mnt/mepis");
    return;
  }
  setCursor(QCursor(Qt::WaitCursor));

  // install new Grub now
  cmd = QString("grub-install --no-floppy --root-directory=/mnt/mepis /dev/%1").arg(boot);
  if (system(cmd.toAscii()) != 0) {
    // error, try again
    // this works for reiser-grub bug
    if (system(cmd.toAscii()) != 0) {
      // error
      setCursor(QCursor(Qt::ArrowCursor));
      system("/bin/umount -l /mnt/mepis");
      QMessageBox::critical(this, QString::null,
        tr("Sorry, installing GRUB failed. This may be due to a change in the disk formatting. You can uncheck GRUB and finish installing MEPIS, then reboot to the CD and repair the installation with the reinstall GRUB function."));
      return;
    }
  }
 
  // make new menu.1st file
  if (grubMenuCheckBox->isChecked()) {
    if (!makeGrub(rootpart, "/mnt/mepis", grubInitrdCheckBox->isChecked())) {
      setCursor(QCursor(Qt::ArrowCursor));
      QMessageBox::critical(this, QString::null,
        tr("Sorry, creating menu.lst failed. Root filesystem may be faulty."));
    } else {
      QMessageBox::information(this, QString::null, tr("GRUB installed ok."));
    }
  }
  setCursor(QCursor(Qt::ArrowCursor));
  system("/bin/umount -l /mnt/mepis");
}

bool MConfig::makeGrub(QString rootpart, const char *rootmnt, bool initrd) {
  char line[130];
  char vga[20] = "vga=normal ";
  char acpi[20] = "";
  char nousb[20] = "";
  char nofloppy[20] = "";
  char generic[20] = "";

  // convert xdxn to (hdi,j)
  QString cmd = QString("/bin/grep '%1' /mnt/mepis/boot/grub/device.map").arg(rootpart.mid(0,3));
  QString val = getCmdOut(cmd.toAscii());
  QString groot = QString ("root %1,%2)\n").arg(val.mid(0,4)).arg(atoi(rootpart.mid(3).toAscii()) - 1);

  FILE *fp = popen("cat /proc/cmdline 2>/dev/null", "r");
  if (fp != NULL) {
    fgets(line, sizeof line, fp);
    int i = strlen(line);
    line[--i] = '\0';
    if (i > 2) {
      char *tok = strtok(line, " ");
      while (tok != NULL) {
        if (strncmp(tok, "vga", 3) == 0) {
          // override vga
          strcpy(vga, tok);
          strcat(vga, " ");
        } else if (strncmp(tok, "nofloppy", 8) == 0) {
          // override floppy
          strcpy(nofloppy, "nofloppy ");
        } else if (strncmp(tok, "all-generic-ide", 15) == 0) {
          // override all-generic-ide
          strcpy(generic, "all-generic-ide ");
        } else if (strncmp(tok, "nousb", 5) == 0) {
          // override usb
          strcpy(nousb, "nousb ");
        } else if (strncmp(tok, "acpi=off", 8) == 0 || strncmp(tok, "noacpi", 6) == 0) {
          // no acpi, use apm
          strcpy(acpi, "acpi=off apm=power_off noacpi ");
        }
        tok = strtok(NULL, " ");
      }
    }
   pclose(fp);
  }

  // backup old grub menu.1st
  sprintf(line, "/bin/mv -f %s/boot/grub/menu.lst %s/boot/grub/menu.lst.old", rootmnt, rootmnt);
  system(line);

  // determine resume partition, if any
//  QString resume = QString("");
//  val = getCmdOut("grep -B99 '# Dynamic' /mnt/mepis/etc/fstab | grep 'swap'");
//  if (!val.isEmpty()) {
//    resume = QString( "resume=%1 ").arg(val.mid(0,10));
//  }

  // create the menu.lst
  sprintf(line, "%s/boot/grub/menu.lst", rootmnt);
  fp = fopen(line, "w");
  if (fp != NULL) {
    // head
    fputs("timeout 10\n", fp);
    fputs("color cyan/blue white/blue\n", fp);
    fputs("foreground ffffff\n", fp);
    fputs("background 0639a1\n\n", fp);
    fputs("gfxmenu /boot/grub/message\n\n", fp);

    QStringList vals = getCmdOuts("ls /mnt/mepis/boot | grep 'vmlinuz-'");
    if (vals.empty()) {
      return false;
    }

    chdir("/mnt/mepis");

    val = QString( tr("title MEPIS at %1, newest kernel\n")).arg(rootpart);
    fputs(val.toAscii(), fp);
    fputs(groot.toAscii(), fp);
    val = QString( "kernel /boot/vmlinuz root=/dev/%1 nomce quiet splash ").arg(rootpart);
    fputs(val.toAscii(), fp);
    fputs(vga, fp);
    fputs(acpi, fp);
    fputs(nousb, fp);
//    fputs(resume.toAscii(), fp);
    fputs(nofloppy, fp);
    fputs(generic, fp);
    if (initrd) {
      val = getCmdOut("ls /mnt/mepis/boot | grep 'initrd.img'");
      if (!val.isEmpty()) {
        fputs("\ninitrd /boot/initrd.img", fp);
      }
    }
    fputs("\nboot\n\n", fp);

    for (QStringList::Iterator it = vals.end(); ;) {
      val = QString( tr("title MEPIS at %1, kernel %2\n")).arg(rootpart).arg((*--it).mid(8));
      fputs(val.toAscii(), fp);
      fputs(groot.toAscii(), fp);
      val = QString("kernel /boot/%1 root=/dev/%2 nomce quiet splash ").arg(*it).arg( rootpart);
      fputs(val.toAscii(), fp);
      fputs(vga, fp);
      fputs(acpi, fp);
      fputs(nousb, fp);
//      fputs(resume.toAscii(), fp);
      fputs(nofloppy, fp);
      fputs(generic, fp);

      if (initrd) {
        cmd = QString("ls /mnt/mepis/boot | grep 'initrd.img-%1'").arg( (*it).mid(8));
        val = getCmdOut(cmd.toAscii());
        if (!val.isEmpty()) {
          cmd = QString("\ninitrd /boot/initrd.img-%1").arg((*it).mid(8));
          fputs(cmd.toAscii(), fp);
        }
      }
      fputs("\nboot\n\n", fp);

      if (it == vals.begin()) {
        break;
      }
    }

    // add windows entries the new way
    QStringList strings = getCmdOuts("/usr/bin/os-prober | grep chain");
    for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
      QString chain = *it;
      QString chainDev = chain.section(":", 0, 0).section("/",2,2);
      QString chainName = chain.section(":", 1, 1);
      val = QString(tr("title %1 at %2\n")).arg(chainName).arg(chainDev);
      fputs(val.toAscii(), fp);

      // convert xdxn to (hdi,j)
      cmd = QString("/bin/grep '%1' /mnt/mepis/boot/grub/device.map").arg(chainDev.mid(0,3));
      val = getCmdOut(cmd.toAscii());

      //if not hd0
      if (val.mid(0,5).compare("(hd0)") != 0) {
        cmd = QString ("map (hd0) %1\n").arg(val.mid(0,5));
        fputs(cmd.toAscii(), fp);
        cmd = QString ("map %1 (hd0)\n").arg(val.mid(0,5));
        fputs(cmd.toAscii(), fp);
      }

      cmd = QString ("rootnoverify %1,%2)\n").arg(val.mid(0,4)).arg(atoi(chainDev.mid(3).toAscii()) - 1);
      fputs(cmd.toAscii(), fp);
      fputs("chainloader +1\n\n", fp);
    }

    // add other linux entries the new way
    val = QString("/usr/bin/os-prober | grep linux | grep -v %1").arg(rootpart);
    strings = getCmdOuts(val.toAscii());
    for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
      QString lin = *it;
      QString linDev = lin.section(":", 0, 0);
      cmd = QString("/usr/bin/linux-boot-prober %1 | grep 'vmlinuz'").arg(linDev);
      lin = getCmdOut(cmd.toAscii());
      if (!lin.isEmpty()) {
        QString val2 = lin.section(":", 2, 2);
        val = QString("title %1\n").arg(val2);
        fputs(val.toAscii(), fp);
        linDev = lin.section(":", 0, 0).section("/",2,2);
        // convert xdxn to (hdi,j)
        cmd = QString("/bin/grep '%1' /mnt/mepis/boot/grub/device.map").arg(linDev.mid(0,3));
        val = getCmdOut(cmd.toAscii());
        cmd = QString ("root %1,%2)\n").arg(val.mid(0,4)).arg(atoi(linDev.mid(3).toAscii()) - 1);
        fputs(cmd.toAscii(), fp);
        val2 = lin.section(":", 3, 3);
        QString val3 = lin.section(":", 5, 5);
        val = QString("kernel %1 %2\n").arg(val2).arg(val3);
        fputs(val.toAscii(), fp);
        val2 = lin.section(":", 4, 4);
        if (!val2.isEmpty()) {
          val = QString("initrd %1\n").arg(val2);
          fputs(val.toAscii(), fp);
        }
      }
      fputs("\n", fp);
    }

    // memtest
    fputs("title MEMTEST\n", fp);
    fputs("kernel /boot/memtest86+.bin\n\n", fp);


    fclose(fp);
  } else {
    return false;
  }
  return true;

}

void MConfig::applyPartition() {
  QString cmd, msg;

  disconnect(proc, SIGNAL(readyReadStandardOutput()), 0, 0);
  connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(partitionStdout()));
  disconnect(proc, SIGNAL(readyReadStandardError()), 0, 0);
  connect(proc, SIGNAL(readyReadStandardError()), this, SLOT(partitionStderr()));
  disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
  connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(partitionDone(int, QProcess::ExitStatus)));

  QString part = repairPartitionComboBox->currentText().section(' ', 0, 0);

  if (repairPartitionComboBox->currentText().contains("linux", Qt::CaseInsensitive)) {
    cmd = QString("/sbin/e2fsck");
    if (repairTestCheckBox->isChecked()) {
      cmd.append(" -n");
    } else {
      cmd.append(" -p");
    }
    if (repairBlocksCheckBox->isChecked()) {
      msg = QString(tr("The bad blocks check will take a long time.  Please be patient."));
      repairProgressTextEdit->append(msg);
      cmd.append(" -ck");
    }
    cmd.append(" /dev/");
    cmd.append(part);
    msg = QString(tr("Checking/repairing filesystem (e2fsck) on /dev/%1.")).arg(part);
    repairProgressTextEdit->append(msg);
    setCursor(QCursor(Qt::WaitCursor));
    proc->start(cmd);
  } else if (repairPartitionComboBox->currentText().contains("w95", Qt::CaseInsensitive) || repairPartitionComboBox->currentText().contains("fat", Qt::CaseInsensitive)) {
    cmd = QString("/sbin/dosfsck");
    if (repairTestCheckBox->isChecked()) {
      cmd.append(" -n");
    } else {
      cmd.append(" -a");
    }
    if (repairBlocksCheckBox->isChecked()) {
      msg = QString(tr("The bad blocks check will take a long time.  Please be patient."));
      repairProgressTextEdit->append(msg);
      cmd.append(" -t");
    }
    cmd.append(" /dev/");
    cmd.append(part);
    msg = QString(tr("Checking/repairing filesystem (dosfsck) on /dev/%1.")).arg(part);
    repairProgressTextEdit->append(msg);
    setCursor(QCursor(Qt::WaitCursor));
    proc->start(cmd);
  }
}

/////////////////////////////////////////////////////////////////////////
// format process events

void MConfig::formatStart() {
  timer->start(100);
}

void MConfig::formatTime() {
  int i = formatProgressBar->value() + 1;
  if (i > 100) {
    i = 0;
  }
  formatProgressBar->setValue(i);
}

void MConfig::eraseDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    if (createNewRadioButton->isChecked() && zipCheckBox->isChecked()) {
      // new bootable usb, format zip for syslinux
      formatStatusEdit->setText(tr("Format disk..."));
      disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
      connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(formatDone(int, QProcess::ExitStatus)));
      // format as zip
      QString cmd = QString("mkdiskimage -4 /dev/%1 0 64 32").arg(formatDiskComboBox->currentText());
      proc->start(cmd.toAscii());
    } else {
      // standard usb, add new partition table
      QString cmd = QString("sfdisk /dev/%1").arg(formatDiskComboBox->currentText());
      FILE *fp = popen(cmd.toAscii(), "w");
      if (fp != NULL) {
        fputs("0,,0b,*\n;\n;\n;\ny\n", fp);
        fputc(EOF, fp);
        pclose(fp);
      } else {
        // error
        system("umount /dev/cdrom");
        system("umount /dev/cdrom1");
        timer->stop();
        formatProgressBar->setValue(0);
        setCursor(QCursor(Qt::ArrowCursor));
        formatStatusEdit->setText(tr("Erase disk...failed"));
        return;
      }
      // format standard usb
      formatStatusEdit->setText(tr("Format disk..."));
      disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
      connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(formatDone(int, QProcess::ExitStatus)));
      // format as dos
      cmd = QString("mkdosfs -I -F 32 /dev/%1").arg(formatDiskComboBox->currentText());
      cmd.append("1");
      proc->start(cmd.toAscii());    
    }
  } else {
    // oops
    system("umount /dev/cdrom");
    system("umount /dev/cdrom1");
    timer->stop();
    formatProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    formatStatusEdit->setText(tr("Erase disk...failed"));
  }
}


void MConfig::formatDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    if (formatRadioButton->isChecked()) {
      // formatting only
      timer->stop();
      formatProgressBar->setValue(0);
      setCursor(QCursor(Qt::ArrowCursor));
      // done
      formatStatusEdit->setText(tr("Format disk...ok"));  
      QMessageBox::information(0, QString::null,
      tr("The USB key has been erased and formatted. There appears to be no errors."));
    } else {
      // making some kind of bootable
      QString cmd = QString("partprobe /dev/%1").arg(formatDiskComboBox->currentText());
      system(cmd.toAscii());
      // wait for probe to finish
      system("sleep 5");

//      QMessageBox::information(0, QString::null,
//      tr("VERY IMPORTANT! Unplug the USB key, then plug it back in, wait a few seconds, then click OK to continue."));


      // for the 'easy' method usb drive nedn't be mounted to do it before any mounting is done.
      if (createEasyRadioButton->isChecked()) {
        // anticapitalista's 'easy' USB disk creation
        //TODO - check file is writeable

        if (fileLineEdit->text().isEmpty()) { // user hasn't selected anything
         // ERROR - report that a filename is required. Cannot use CD/DVD as source. Maybe can via 'cat'- TODO?
         timer->stop();
         formatProgressBar->setValue(0);
         setCursor(QCursor(Qt::ArrowCursor));
         formatStatusEdit->setText(tr("No ISO file defined...failed"));
         return;
        }
        formatStatusEdit->setText(tr("Preparing ISO file..."));
        QString cmd = QString("isohybrid %1").arg(fileLineEdit->text());  // fiddle with ISO file
        system(cmd.toAscii());
        formatStatusEdit->setText(tr("Writing to USB..."));
        cmd = QString("dd if=%1 of=/dev/%2").arg(fileLineEdit->text()).arg(formatDiskComboBox->currentText());  // write it to usb drive
        system(cmd.toAscii());
        // and we're done - I think...
        timer->stop();
        formatProgressBar->setValue(0);
        setCursor(QCursor(Qt::ArrowCursor));
        formatStatusEdit->setText(tr("Copy system...ok"));
        QMessageBox::information(0, QString::null,
         tr("The system has been copied to the USB key. There appears to be no errors. To boot from USB key, usually you have to press a special key when the computer starts. The key will not boot on a Mac computer."));
        return;
      }


      // mount usb partition
      cmd = QString("/dev/%1").arg(formatDiskComboBox->currentText());
      if (createNewRadioButton->isChecked() && zipCheckBox->isChecked()) {
        cmd.append("4");
      } else {
        cmd.append("1");
      }
      mountPartition(cmd, "/mnt/temp");
      formatStatusEdit->setText(tr("Copy system..."));
      disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
      connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(bootableDone(int, QProcess::ExitStatus)));
      // copy from source
      if (cdRadioButton->isChecked()) {
	// copy from cd
        if (system("/bin/ls /media/cdrom/ | grep -q 'version'") == 0) {
          system("/bin/cp -p /media/cdrom/version /mnt/temp");
          proc->start("/bin/cp -a /media/cdrom/boot /media/cdrom/mepis /mnt/temp");
        } else {
          system("/bin/cp -p /media/cdrom1/version /mnt/temp");
          proc->start("/bin/cp -a /media/cdrom1/boot /media/cdrom1/mepis /mnt/temp");
        }
      
      } else {
        //copy from file
        mkdir("/mnt/iso", 0755);
        system("umount /mnt/iso");
        QString cmd = QString("mount -o loop %1 /mnt/iso").arg(fileLineEdit->text());
        system(cmd.toAscii());
        system("/bin/cp -p /mnt/iso/version /mnt/temp");
        proc->start("/bin/cp -a /mnt/iso/boot /mnt/iso/mepis /mnt/temp");	
      }
    }
  } else {
    // oops, bad format
    system("umount /dev/cdrom");
    system("umount /dev/cdrom1");
    timer->stop();
    formatProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    formatStatusEdit->setText(tr("Format disk...failed"));
  }
}

void MConfig::bootableDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    if (createOldRadioButton->isChecked()) {
      // old style bootable
      system("rm -f /mnt/temp/boot/grub/stage2_eltorito");
      system("cp -a /usr/share/mepis-system/grub/* /mnt/temp/boot/grub/");

      FILE *fp = popen("grub --device-map=/dev/null --batch", "w");
      if (fp != NULL) {
        fputs("device (hd0) /dev/", fp);
        fputs(formatDiskComboBox->currentText().toAscii(), fp);
        fputs("\n", fp);
        fputs("root (hd0,0)\n", fp);
        fputs("setup (hd0)\n", fp);
        fputs("quit\n", fp);
        fputc(EOF, fp);
        pclose(fp);
      }
      system("umount /mnt/temp");
    } else {
      // new style bootable
      system("mv /mnt/temp/boot/isolinux/isolinux.cfg /mnt/temp/boot/isolinux/syslinux.cfg");
      system("mv /mnt/temp/boot/isolinux /mnt/temp/boot/syslinux");
      system("umount /mnt/temp");
      system("umount /mnt/iso");
      QString cmd = QString("syslinux -d /boot/syslinux /dev/%1").arg(formatDiskComboBox->currentText());
      if (zipCheckBox->isChecked()) {
        cmd.append("4");
        system(cmd.toAscii());
      } else {
        cmd.append("1");
        system(cmd.toAscii());
        cmd = QString("install-mbr /dev/%1").arg(formatDiskComboBox->currentText());	
        system(cmd.toAscii());
      }
    }
    system("umount /dev/cdrom");
    system("umount /dev/cdrom1");
    timer->stop();
    formatProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    formatStatusEdit->setText(tr("Copy system...ok"));
    QMessageBox::information(0, QString::null,
      tr("The system has been copied to the USB key. There appears to be no errors. To boot from USB key, usually you have to press a special key when the computer starts. The key will not boot on a Mac computer."));
  } else {
    system("umount /dev/cdrom");
    system("umount /dev/cdrom1");
    system("umount /mnt/temp");
    timer->stop();
    formatProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    formatStatusEdit->setText(tr("Copy system...failed"));
    QMessageBox::information(0, QString::null,
      tr("The system failed to copy to the USB key. This could happen if you tried to copy an older MEPIS CD or a CD that does not read reliably."));
  }
}

void MConfig::isoDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    formatStatusEdit->setText(tr("Formatting iso..."));
    disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(procDone(int, QProcess::ExitStatus)));
    QString cmd = QString("mkfs.ext3 -F /media/%1").arg(formatDiskComboBox->currentText());
    cmd.append("1/.onthego.iso");
    proc->start(cmd);
  } else {
    QString cmd = QString("pumount /dev/%1").arg(formatDiskComboBox->currentText());
    cmd.append("1");
    system(cmd.toAscii());
    timer->stop();
    formatProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    formatStatusEdit->setText(tr("Creating iso...failed"));
  }
}

void MConfig::procDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
   // add home
    formatStatusEdit->setText(tr("Finalize OnTheGo..."));
    mkdir("/mnt/temp", 0755);
    QString cmd = QString("mount -t ext3 /media/%1").arg(formatDiskComboBox->currentText());
    cmd.append("1/.onthego.iso /mnt/temp -o loop");
    system(cmd.toAscii());

    system("rsync -a /etc/skel/ /mnt/temp 2>/dev/null");
    system("chown -R 29999:100 /mnt/temp");
    system("umount /mnt/temp 2>/dev/null");

    cmd = QString("pumount /dev/%1").arg(formatDiskComboBox->currentText());
    cmd.append("1");
    system(cmd.toAscii());
    timer->stop();
    formatProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    formatStatusEdit->setText(tr("Finalize OnTheGo...ok"));
  } else {
    QString cmd = QString("pumount /dev/%1").arg(formatDiskComboBox->currentText());
    cmd.append("1");
    system(cmd.toAscii());
    timer->stop();
    formatProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    formatStatusEdit->setText(tr("Formatting iso...failed"));
  }
}

/////////////////////////////////////////////////////////////////////////
// partition process events

void MConfig::partitionStdout() {
  repairProgressTextEdit->append(QString(proc->readAllStandardOutput()).remove(0x08));
}

void MConfig::partitionStderr() {
  repairProgressTextEdit->append(QString(proc->readAllStandardError()).remove(0x08));
}

void MConfig::partitionDone(int exitCode, QProcess::ExitStatus exitStatus) {
  setCursor(QCursor(Qt::ArrowCursor));
  if (exitStatus == QProcess::NormalExit) {
    repairProgressTextEdit->append(tr("Check partition completed."));
  } else {
    repairProgressTextEdit->append(tr("Check partition aborted."));
  }
}

/////////////////////////////////////////////////////////////////////////
// slots

void MConfig::show() {
  QDialog::show();
  refresh();
}

void MConfig::on_selectFilePushButton_clicked() {
   QString isoname = QFileDialog::getOpenFileName(this,
     tr("Choose MEPIS ISO Source"), "/home", tr("ISO Files (*.iso)"));
   if (!isoname.isNull()) {
     fileLineEdit->setText(isoname);
   }
}

void MConfig::on_repairBlocksCheckBox_clicked() {
  if (repairBlocksCheckBox->isChecked()) {
    repairTestCheckBox->setChecked(true);
  }
}

void MConfig::on_repairTestCheckBox_clicked() {
  if (!repairTestCheckBox->isChecked()) {
    repairBlocksCheckBox->setChecked(false);
  }
}

void MConfig::on_repairDiskComboBox_activated() {
  char line[130];
  QString drv = QString("/dev/%1").arg(repairDiskComboBox->currentText());

  repairPartitionComboBox->clear();
  QString cmd = QString("/sbin/fdisk -l %1 | /bin/grep \"^/dev\"").arg(drv);
  FILE *fp = popen(cmd.toAscii(), "r");
  int rcount = 0;
  if (fp != NULL) {
    char *ndev, *nsz, *nsys, *nsys2;
    int nsize;
    int dsize;
    int i;
    while (fgets(line, sizeof line, fp) != NULL) {
      i = strlen(line);
      line[--i] = '\0';
      strtok(line, " /*+\t");
      ndev = strtok(NULL, " /*+\t");
      strtok(NULL, " *+\t");
      strtok(NULL, " *+\t");
      nsz = strtok(NULL, " *+\t");
      strtok(NULL, " *+\t");
      nsys = strtok(NULL, " *+\t");
      nsys2 = strtok(NULL, " *+\t");
      nsize = atoi(nsz);
      dsize = nsize / 1024;
      if (nsys2 == NULL || (strncmp(nsys2, "Ext", 3) != 0 && strncmp(nsys2, "swap", 4) != 0)) {
        if (strncmp(nsys, "Linux", 5) == 0 || strncmp(nsys, "W95", 3) == 0 || strncmp(nsys, "FAT", 3) == 0) {
          sprintf(line, "%s - %dMB - %s", ndev, dsize, nsys);
          repairPartitionComboBox->addItem(line);
          rcount++;
	}
      }
    }
    pclose(fp);
  }
  if (rcount == 0) {
    buttonApply->setEnabled(false);
    repairPartitionComboBox->addItem("none");
  } else {
    buttonApply->setEnabled(true);
  }
  on_repairPartitionComboBox_activated();
}

void MConfig::on_repairPartitionComboBox_activated() {
  char line[130];

  strcpy(line, repairPartitionComboBox->currentText().toAscii());
  char *tok = strtok(line, " -");
  QString cmd = QString("cat /proc/mounts | grep '/dev/%1' 2>/dev/null").arg(tok);
  QString cmd2 = QString("cat /etc/fstab | grep '/dev/%1 / ' 2>/dev/null").arg(tok);
  if (system(cmd.toAscii()) == 0 || system(cmd2.toAscii()) == 0) {
    // mounted
    repairTestCheckBox->setEnabled(false);
    repairTestCheckBox->setChecked(true);
    repairBlocksCheckBox->setEnabled(false);
    repairBlocksCheckBox->setChecked(false);
  } else {
    repairTestCheckBox->setEnabled(true);
    repairTestCheckBox->setChecked(false);
    repairBlocksCheckBox->setEnabled(true);
  }
}

// disk selection changed
void MConfig::on_grubInstallComboBox_activated() {
  char line[130];
  QString drv = QString("/dev/%1").arg(grubInstallComboBox->currentText());

  grubRootComboBox->clear();
  QString cmd = QString("/sbin/fdisk -l %1 | /bin/grep \"^/dev\"").arg(drv);
  FILE *fp = popen(cmd.toAscii(), "r");
  int rcount = 0;
  if (fp != NULL) {
    char *ndev, *nsz, *nsys, *nsys2;
    int nsize;
    int i;
    while (fgets(line, sizeof line, fp) != NULL) {
      i = strlen(line);
      line[--i] = '\0';
      strtok(line, " /*+\t");
      ndev = strtok(NULL, " /*+\t");
      strtok(NULL, " *+\t");
      strtok(NULL, " *+\t");
      nsz = strtok(NULL, " *+\t");
      strtok(NULL, " *+\t");
      nsys = strtok(NULL, " *+\t");
      nsys2 = strtok(NULL, " *+\t");
      nsize = atoi(nsz);
      nsize = nsize / 1024;
      if (nsize >= 1200 && strncmp(nsys, "Linux", 5) == 0 && nsys2 == NULL ) {
        sprintf(line, "%s - %dMB - %s", ndev, nsize, nsys);
        grubRootComboBox->addItem(line);
        rcount++;
      }
    }
    pclose(fp);
  }
  if (rcount == 0) {
    grubRootComboBox->addItem("none");
    buttonApply->setEnabled(false);
  } else {
    buttonApply->setEnabled(true);
  }
}

void MConfig::on_tabWidget_currentChanged() {
  refresh();
}

void MConfig::on_keyboardCombo_activated() {
  buttonApply->setEnabled(true);
}

void MConfig::on_localeCombo_activated() {
  buttonApply->setEnabled(true);
}

void MConfig::on_computerNameEdit_textEdited() {
  buttonApply->setEnabled(true);
}

void MConfig::on_computerDomainEdit_textEdited() {
  buttonApply->setEnabled(true);
}

void MConfig::on_computerGroupEdit_textEdited() {
  buttonApply->setEnabled(true);
}

void MConfig::on_bootClearCacheCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_bootClearLogsCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_clearLogsCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_checkLanguageLocale_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_checkKeyboardLayout_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_checkSambaWorkgroup_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_checkComputerDomain_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_checkComputerName_clicked() {
  buttonApply->setEnabled(true);
}

// apply but do not close
void MConfig::on_buttonApply_clicked() {
  if (!buttonApply->isEnabled()) {
    return;
  }

  int i = tabWidget->currentIndex();
  switch (i) {
    case 1:
      applyUSB();
      break;

    case 2:
      buttonApply->setEnabled(false);
      applyGrub();
      buttonApply->setEnabled(true);
      break;

    case 3:
      buttonApply->setEnabled(false);
      applyPartition();
      buttonApply->setEnabled(true);
      break;

    default:
      setCursor(QCursor(Qt::WaitCursor));
      applyOptions();
      setCursor(QCursor(Qt::ArrowCursor));
      buttonApply->setEnabled(false);
      break;
  }
}

// close but do not apply
void MConfig::on_buttonCancel_clicked() {
  close();
}

// apply then close
void MConfig::on_buttonOk_clicked() {
  on_buttonApply_clicked();
  close();
}

bool MConfig::hasInternetConnection() 
{
   bool internetConnection  = false;
   // Query network interface status
   QStringList interfaceList  = getCmdOuts("ifconfig -a -s");
   int i=1;
   while (i<interfaceList.size()) {
      QString interface = interfaceList.at(i);
      interface = interface.left(interface.indexOf(" "));
      if ((interface != "lo") && (interface != "wmaster0") && (interface != "wifi0")) {
         QStringList ifStatus  = getCmdOuts(QString("ifconfig %1").arg(interface));
         QString unwrappedList = ifStatus.join(" ");
         if (unwrappedList.indexOf("UP ") != -1) {
            if (unwrappedList.indexOf(" RUNNING ") != -1) {
               internetConnection  = true;
            }
         }
      }
      ++i;
   }
   return internetConnection;
}

void MConfig::executeChild(const char* cmd, const char* param)
{
   pid_t childId;
   childId = fork();
   if (childId >= 0)
      {
      if (childId == 0)
         {
         execl(cmd, cmd, param, (char *) 0);

         //system(cmd);
         }
      }
}

void MConfig::on_generalHelpPushButton_clicked() 
{
   QString manualPackage = tr("mepis-manual");
   QString statusl = getCmdValue(QString("dpkg -s %1 | grep '^Status'").arg(manualPackage).toAscii(), "ok", " ", " ");
   if (statusl.compare("installed") != 0) {
      if (this->hasInternetConnection()) {
         setCursor(QCursor(Qt::WaitCursor));
         int ret = QMessageBox::information(0, tr("The MEPIS manual is not installed"),
                      tr("The MEPIS manual is not installed, do you want to install it now?"),
                      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
         if (ret == QMessageBox::Yes) {
            system(QString("apt-get install -qq %1").arg(manualPackage).toAscii());
            setCursor(QCursor(Qt::ArrowCursor));
            statusl = getCmdValue(QString("dpkg -s %1 | grep '^Status'").arg(manualPackage).toAscii(), "ok", " ", " ");
            if (statusl.compare("installed") != 0) {
               QMessageBox::information(0, tr("The MEPIS manual hasn't been installed"),
               tr("The MEPIS manual cannot be installed. This may mean you are using the LiveCD or that there are some kind of transitory problem with the repositories,"),
               QMessageBox::Ok);

            }
         }
         else {
            setCursor(QCursor(Qt::ArrowCursor));               
            return;
            }
      }
      else {
         QMessageBox::information(0, tr("The MEPIS manual is not installed"),
            tr("The MEPIS manual is not installed and no Internet connection could be detected so it cannot be installed"),
            QMessageBox::Ok);
         return;
      }
   }
   QString page;
   page = tr("file:///usr/share/mepis-manual/en/index.html#section05-3-2");

   //executeChild("/usr/bin/konqueror", page.toAscii());
   executeChild("/etc/alternatives/x-www-browser", page.toAscii());
}

// show about
void MConfig::on_buttonAbout_clicked() {
  QMessageBox::about(0, tr("About"),
      tr("<p><b>MEPIS System</b></p>"
      "<p>Copyright (C) 2003-10 by MEPIS LLC.  All rights reserved.</p>"));
}

