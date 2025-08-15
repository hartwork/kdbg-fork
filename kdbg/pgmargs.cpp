/*
 * Copyright Johannes Sixt
 * This file is licensed under the GNU General Public License Version 2.
 * See the file COPYING in the toplevel directory of the source directory.
 */

#include "pgmargs.h"
#include <klocalizedstring.h>		/* i18n */
#include <QFileDialog>
#include <QStringList>
#include "mydebug.h"

PgmArgs::PgmArgs(QWidget* parent, const QString& pgm,
		 const std::map<QString,QString>& envVars) :
	QDialog(parent)
{
    setupUi(this);

    {
	QFileInfo fi(pgm);
	QString newText = labelArgs->text().arg(fi.fileName());
	labelArgs->setText(newText);
    }

    EnvVar val;
    val.status = EnvVar::EVclean;
    for (std::map<QString,QString>::const_iterator i = envVars.begin(); i != envVars.end(); ++i)
    {
	val.value = i->second;
	val.item = new QTreeWidgetItem(envList, QStringList() << i->first << i->second);
	m_envVars[i->first] = val;
    }

    envList->setAllColumnsShowFocus(true);
    buttonDelete->setEnabled(envList->currentItem() != nullptr);
}

PgmArgs::~PgmArgs()
{
}

// this is a slot
void PgmArgs::on_buttonModify_clicked()
{
    modifyVar(true);	// re-add deleted entries
}

void PgmArgs::modifyVar(bool resurrect)
{
    QString name, value;
    parseEnvInput(name, value);
    if (name.isEmpty() || name.indexOf(QLatin1Char(' ')) >= 0)	// disallow spaces in names
	return;

    // lookup the value in the dictionary
    EnvVar* val;
    std::map<QString,EnvVar>::iterator i = m_envVars.find(name);
    if (i != m_envVars.end()) {
	val = &i->second;
	// see if this is a zombie
	if (val->status == EnvVar::EVdeleted) {
	    // resurrect
	    if (resurrect)
	    {
		val->value = value;
		val->status = EnvVar::EVdirty;
		val->item = new QTreeWidgetItem(envList, QStringList() << name << value);
	    }
	} else if (value != val->value) {
	    // change the value
	    val->value = value;
	    val->status = EnvVar::EVdirty;
	    val->item->setText(1, value);
	}
    } else {
	// add the value
	val = &m_envVars[name];
	val->value = value;
	val->status = EnvVar::EVnew;
	val->item = new QTreeWidgetItem(envList, QStringList() << name << value);
    }
    envList->setCurrentItem(val->item);
    buttonDelete->setEnabled(true);
}

// delete the selected item
void PgmArgs::on_buttonDelete_clicked()
{
    QTreeWidgetItem* item = envList->currentItem();
    if (!item)
	return;
    QString name = item->text(0);

    // lookup the value in the dictionary
    std::map<QString,EnvVar>::iterator i = m_envVars.find(name);
    if (i != m_envVars.end())
    {
	EnvVar* val = &i->second;
	// delete from list
	val->item = nullptr;
	// if this is a new item, delete it completely, otherwise zombie-ize it
	if (val->status == EnvVar::EVnew) {
	    m_envVars.erase(i);
	} else {
	    // mark value deleted
	    val->status = EnvVar::EVdeleted;
	}
    }
    delete item;
    // there is no selected item anymore
    buttonDelete->setEnabled(false);
}

void PgmArgs::parseEnvInput(QString& name, QString& value)
{
    // parse input from edit field
    QString input = envVar->text();
    int equalSign = input.indexOf(QLatin1Char('='));
    if (equalSign >= 0) {
	name = input.left(equalSign).trimmed();
	value = input.mid(equalSign+1);
    } else {
	name = input.trimmed();
	value = QString();		/* value is empty */
    }
}

void PgmArgs::on_envList_currentItemChanged()
{
    QTreeWidgetItem* item = envList->currentItem();
    buttonDelete->setEnabled(item != nullptr);
    if (!item)
	return;

    // must get name from list box
    QString name = item->text(0);
    envVar->setText(name + QLatin1Char('=') + m_envVars[name].value);
}

void PgmArgs::accept()
{
    // simulate that the Modify button was pressed, but don't revive
    // dead entries even if the user changed the edit box
    modifyVar(false);
    QDialog::accept();
}

void PgmArgs::on_wdBrowse_clicked()
{
    // browse for the working directory
    QString newDir = QFileDialog::getExistingDirectory(this, {}, wd());
    if (!newDir.isEmpty()) {
	setWd(newDir);
    }
}

void PgmArgs::on_insertFile_clicked()
{
    QString caption = i18n("Select a file name to insert as program argument");

    QFileDialog dlg(this, caption);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setLabelText(QFileDialog::Accept,
		// i18n: replacement text for the "Open" button in a file dialog
		// the selected file name will be inserted in a text box
		i18nc("@action:button file name into text box", "Insert"));

    if (dlg.exec() != QDialog::Accepted)
	return;

    auto names = dlg.selectedFiles();
    // don't clear the selection if no file was selected
    if (names.count() > 0)
	programArgs->insert(names.first());
}

void PgmArgs::on_insertDir_clicked()
{
    QString caption = i18n("Select a directory to insert as program argument");

    // use the selection as default
    QString f = programArgs->selectedText();
    f = QFileDialog::getExistingDirectory(this, caption, f);
    // don't clear the selection if no file was selected
    if (!f.isEmpty()) {
	programArgs->insert(f);
    }
}
