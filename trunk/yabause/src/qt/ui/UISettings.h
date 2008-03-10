/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef UISETTINGS_H
#define UISETTINGS_H

#include "ui_UISettings.h"
#include "../QtYabause.h"

class UISettings : public QDialog, public Ui::UISettings
{
	Q_OBJECT
	
public:
	UISettings( QWidget* parent = 0 );

protected:
	void requestFile( const QString& caption, QLineEdit* edit );
	void requestNewFile( const QString& caption, QLineEdit* edit );
	void requestFolder( const QString& caption, QLineEdit* edit );
	void requestDrive( const QString& caption, QLineEdit* edit );
	void loadCores();
	void loadSettings();
	void saveSettings();

protected slots:
	void tbBrowse_clicked();
	void pbInputs_clicked();
	void accept();
};

#endif // UISETTINGS_H
