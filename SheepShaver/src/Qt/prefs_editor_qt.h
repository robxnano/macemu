/* prefs_editor_qt.h
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

#pragma once

#include <QMainWindow>
#include <QStringListModel>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class PrefsEditorQt;
}
QT_END_NAMESPACE

class PrefsEditorQt : public QMainWindow
{
    Q_OBJECT

public:
    PrefsEditorQt(QWidget *parent = nullptr);
    ~PrefsEditorQt();

    bool result();

protected:
    void closeEvent(QCloseEvent &ev);

private slots:
    void on_createVolumeButton_clicked();

    void on_actionQuit_triggered();

    void on_actionAboutQt_triggered();

    void on_actionAbout_triggered();

    void on_quitButton_clicked();

    void on_startButton_clicked();

    void on_addVolumeButton_clicked();

    void on_removeVolumeButton_clicked();

    void on_screenModeComboBox_currentIndexChanged(int index);

    void on_sharedFolderSelectButton_clicked();

    void on_resolutionComboBox_editTextChanged(const QString &text);

    void on_romFileSelectButton_clicked();

    void on_keycodeFileSelectButton_clicked();


private:
    Ui::PrefsEditorQt *ui;
    QStringListModel *volumes;
    bool startClicked;

    void initDialog();

    void browseForFile(QLineEdit *entry, bool directory);
};
