/* prefs_editor_qt.cpp
 *
 * SheepShaver (C) 1997-2008 Christian Bauer and Marc Hellwig
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "sysdeps.h"

#include "prefs_editor_qt.h"
#include "ui_prefs_editor_qt.h"
#include "prefs.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <iostream>

using namespace std;

PrefsEditorQt::PrefsEditorQt(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PrefsEditorQt)
{
    ui->setupUi(this);
    initDialog();
    volumes = new QStringListModel();
    ui->volumesList->setModel(volumes);
    startClicked = false;
}

PrefsEditorQt::~PrefsEditorQt()
{
    delete ui;
    delete volumes;
}


void PrefsEditorQt::initDialog()
{

    QString string = QString::fromUtf8(PrefsFindString("rom"));
    ui->romFileEntry->setText(string);
}


void PrefsEditorQt::on_removeVolumeButton_clicked()
{
    for (const auto &index : ui->volumesList->selectionModel()->selectedIndexes())
    {
        volumes->removeRow(index.row());
    }
}

void PrefsEditorQt::on_createVolumeButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setLabelText(QFileDialog::Accept, "Create");
    dialog.setFileMode(QFileDialog::AnyFile);

    QInputDialog sizeDialog(this);
    sizeDialog.setLabelText(QString::fromUtf8("Volume Size (MB):"));
    sizeDialog.setIntMinimum(1);
    sizeDialog.setIntMaximum(2000);
    sizeDialog.setIntValue(64);
    sizeDialog.setOkButtonText(QString::fromUtf8("Create"));
    sizeDialog.setInputMode(QInputDialog::IntInput);

    if (dialog.exec() && sizeDialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        for (const auto &filename : filenames)
        {
            // Create a volume file initialized to zeroes
            QFile volume(filename);
            if (volume.open(QIODeviceBase::Truncate | QIODeviceBase::WriteOnly, (QFileDevice::Permissions) 0x644))
            {
                volume.resize(sizeDialog.intValue() * 1024 * 1024);
                volume.close();
                volumes->insertRow(volumes->rowCount());
                QModelIndex index = volumes->index(volumes->rowCount() - 1);
                volumes->setData(index, filename);
            }
        }
    }
}


void PrefsEditorQt::on_actionQuit_triggered()
{
    QApplication::quit();
}


void PrefsEditorQt::on_actionAboutQt_triggered()
{
    QApplication::aboutQt();
}


void PrefsEditorQt::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About SheepShaver", "SheepShaver is an emulator for PowerPC-based Macs");
}


void PrefsEditorQt::on_startButton_clicked()
{
    startClicked = true;
    //save_volumes();
    SavePrefs();
    QApplication::quit();
}


void PrefsEditorQt::on_quitButton_clicked()
{
    QApplication::quit();
}


void PrefsEditorQt::on_addVolumeButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setWindowTitle("Add Volume File");
    dialog.setLabelText(QFileDialog::Accept, "Add");
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        for (const auto &filename : filenames)
        {
            volumes->insertRow(volumes->rowCount());
            QModelIndex index = volumes->index(volumes->rowCount() - 1);
            volumes->setData(index, filename);
        }
    }
}

void PrefsEditorQt::browseForFile(QLineEdit *entry, bool directory)
{
    QFileDialog dialog(this, directory ? "Select Folder" : "Select File");
    if (directory)
        dialog.setOption(QFileDialog::ShowDirsOnly);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setLabelText(QFileDialog::Accept, "Select");
    dialog.setFileMode(directory ? QFileDialog::Directory : QFileDialog::ExistingFile);
    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        for (const auto &filename : filenames)
        {
            entry->setText(filename);
        }
    }
}

void PrefsEditorQt::on_sharedFolderSelectButton_clicked()
{
    browseForFile(ui->sharedFolderEntry, true);
}


void PrefsEditorQt::on_romFileSelectButton_clicked()
{
    browseForFile(ui->romFileEntry, false);
}


void PrefsEditorQt::on_keycodeFileSelectButton_clicked()
{
    browseForFile(ui->keycodeFileEntry, false);
}


void PrefsEditorQt::on_screenModeComboBox_currentIndexChanged(int index)
{
    cout << "Index changed: " << index << endl;
}


void PrefsEditorQt::on_resolutionComboBox_editTextChanged(const QString &text)
{
    cout << "res: " << text.toStdString() << endl;
}


bool PrefsEditorQt::result()
{
    return startClicked;
}

void closeEvent(QCloseEvent &ev)
{
    SavePrefs();
}
