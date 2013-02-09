/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * McdControl.hpp: Sega CD Control Panel.                                  *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
 * Copyright (c) 2008-2011 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __GENS_QT4_MCDCONTROLWINDOW_HPP__
#define __GENS_QT4_MCDCONTROLWINDOW_HPP__

#include "ui_McdControlWindow.h"

// Qt includes.
#include <QtGui/QDialog>

// Qt forward declarations.
class QPushButton;

// Find CD-ROM drives base class.
#include "cdrom/FindCdrom.hpp"

namespace GensQt4
{

class McdControlWindow : public QDialog, public Ui::McdControlWindow
{
	Q_OBJECT
	
	public:
		static void ShowSingle(QWidget *parent = nullptr);

	protected:
		McdControlWindow(QWidget *parent = nullptr);
		virtual ~McdControlWindow();

		// State change event. (Used for switching the UI language at runtime.)
		void changeEvent(QEvent *event);

	protected slots:
		void query(void);
		void driveUpdated(const CdromDriveEntry& drive);
		void driveQueryFinished(void);
		void driveRemoved(QString path);

	private:
		static McdControlWindow *m_McdControlWindow;

		// Refresh button.
		QPushButton *btnRefresh;

		FindCdrom *m_drives;
		bool m_isQuerying;
		QList<CdromDriveEntry> m_queryList;
		void addDriveEntry(const CdromDriveEntry& drive, int index = -1);
};

}

#endif /* __GENS_QT4_MCDCONTROLWINDOW_HPP__ */
