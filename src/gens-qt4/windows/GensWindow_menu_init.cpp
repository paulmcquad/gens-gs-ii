/******************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                     *
 * GensWindow_menu_init.cpp: Gens Window: Menu initialization.                *
 *                                                                            *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                         *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                                *
 * Copyright (c) 2008-2015 by David Korth.                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software                *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA *
 ******************************************************************************/

// NOTE: Before committing: check the LATEST UI file!
#include "GensWindow.hpp"

// Qt includes.
#include <QtCore/QSignalMapper>
#include <QtGui/QActionGroup>

// LibGens includes.
#include "libgens/Util/MdFb.hpp"
#include "libgens/MD/SysVersion.hpp"
using LibGens::MdFb;
using LibGens::SysVersion;

#include "GensWindow_p.hpp"
namespace GensQt4 {

/** GensWindowPrivate **/

/**
 * Initialize the menu bar.
 */
void GensWindowPrivate::initMenuBar(void)
{
	Q_Q(GensWindow);

	// Initialize roles for Mac OS X.
	// We don't want any heuristics to be enabled by default.
	// NOTE: Not limiting this to OS X in case Qt adds
	// support for other platforms later on.
	foreach (QAction *action, q->actions()) {
		action->setMenuRole(QAction::NoRole);
	}

	// Set menu roles for specific actions manually.
	ui.actionFileQuit->setMenuRole(QAction::QuitRole);
	ui.actionFileGeneralConfiguration->setMenuRole(QAction::PreferencesRole);
	// TODO: Hide the "Help" menu?
	ui.actionHelpAbout->setMenuRole(QAction::AboutRole);
	// TODO: Set standard shortcuts?
	// TODO: Rename "Quit" to "Exit" on Windows.

	// Create QSignalMappers and QActionGroups for submenus
	// with lots of similar items.
	QSignalMapper *mapper;
	QActionGroup *actgrp;

	// QSignalMapper macros.
	#define initMapper(_slot) do { \
		mapper = new QSignalMapper(q); \
		QObject::connect(mapper, SIGNAL(mapped(int)), \
			q, (_slot), Qt::UniqueConnection); \
	} while (0)
	#define doMapping(_widget, _id) do { \
		mapper->setMapping((_widget), (_id)); \
		QObject::connect((_widget), SIGNAL(triggered()), mapper, \
			SLOT(map()), Qt::UniqueConnection); \
	} while (0)

	// QActionGroup macros.
	#define initActGrp() do { \
		actgrp = new QActionGroup(q); \
		actgrp->setExclusive(true); \
	} while (0)
	#define doMappingExc(_widget, _id) do { \
		doMapping((_widget), (_id)); \
		actgrp->addAction(_widget); \
	} while (0)

	// Macros for connecting the base item for submenus.
	#define doBaseMenu(_menu, _slot) do { \
		QAction *action = (_menu)->menuAction(); \
		QObject::connect(action, SIGNAL(triggered()), \
			q, (_slot), Qt::UniqueConnection); \
	} while (0)
	#define doBaseMenuKey(_menu, _slot, _shortcut) do { \
		QAction *action = (_menu)->menuAction(); \
		QObject::connect(action, SIGNAL(triggered()), \
			q, (_slot), Qt::UniqueConnection); \
		action->setShortcut(QKeySequence(_shortcut)); \
	} while (0)

	// Graphics, Resolution.
	initMapper(SLOT(map_actionGraphicsResolution_triggered(int)));
	doMapping(ui.actionGraphicsResolution1x, 1);
	doMapping(ui.actionGraphicsResolution2x, 2);
	doMapping(ui.actionGraphicsResolution3x, 3);
	doMapping(ui.actionGraphicsResolution4x, 4);
	// TODO: QActionGroup?

	// Graphics, Color Depth.
	initMapper(SLOT(map_actionGraphicsBpp_triggered(int)));
	initActGrp();
	doMappingExc(ui.actionGraphicsBpp15, MdFb::BPP_15);
	doMappingExc(ui.actionGraphicsBpp16, MdFb::BPP_16);
	doMappingExc(ui.actionGraphicsBpp32, MdFb::BPP_32);

	// Graphics, Stretch Mode.
	initMapper(SLOT(map_actionGraphicsStretch_triggered(int)));
	initActGrp();
	doMappingExc(ui.actionGraphicsStretchNone,       STRETCH_NONE);
	doMappingExc(ui.actionGraphicsStretchHorizontal, STRETCH_H);
	doMappingExc(ui.actionGraphicsStretchVertical,   STRETCH_V);
	doMappingExc(ui.actionGraphicsStretchFull,       STRETCH_FULL);
	doBaseMenuKey(ui.mnuGraphicsStretch,
		SLOT(mnu_mnuGraphicsStretch_triggered()),
		GensWindow::tr("Shift+F2"));

	// System, Region.
	initMapper(SLOT(map_actionSystemRegion_triggered(int)));
	initActGrp();
	doMappingExc(ui.actionSystemRegionAuto, SysVersion::REGION_AUTO);
	doMappingExc(ui.actionSystemRegionJPN,  SysVersion::REGION_JP_NTSC);
	doMappingExc(ui.actionSystemRegionAsia, SysVersion::REGION_ASIA_PAL);
	doMappingExc(ui.actionSystemRegionUSA,  SysVersion::REGION_US_NTSC);
	doMappingExc(ui.actionSystemRegionEUR,  SysVersion::REGION_EU_PAL);
	doBaseMenuKey(ui.mnuSystemRegion,
		SLOT(mnu_mnuSystemRegion_triggered()),
		GensWindow::tr("Shift+F3"));

	// Options, SoundTest.
	initMapper(SLOT(map_actionSound_triggered(int)));
	initActGrp();
	doMappingExc(ui.actionSound11, 11025);
	doMappingExc(ui.actionSound16, 16000);
	doMappingExc(ui.actionSound22, 22050);
	doMappingExc(ui.actionSound32, 32000);
	doMappingExc(ui.actionSound44, 44100);
	doMappingExc(ui.actionSound48, 48000);

	// Non-Menu Actions.
	initMapper(SLOT(map_actionNoMenuSaveSlot_triggered(int)));
	initActGrp();
	doMappingExc(ui.actionNoMenuSaveSlot0, 0);
	doMappingExc(ui.actionNoMenuSaveSlot1, 1);
	doMappingExc(ui.actionNoMenuSaveSlot2, 2);
	doMappingExc(ui.actionNoMenuSaveSlot3, 3);
	doMappingExc(ui.actionNoMenuSaveSlot4, 4);
	doMappingExc(ui.actionNoMenuSaveSlot5, 5);
	doMappingExc(ui.actionNoMenuSaveSlot6, 6);
	doMappingExc(ui.actionNoMenuSaveSlot7, 7);
	doMappingExc(ui.actionNoMenuSaveSlot8, 8);
	doMappingExc(ui.actionNoMenuSaveSlot9, 9);
}

/**
 * Update the menu bar visibility.
 */
void GensWindowPrivate::updateMenuBarVisibility(void)
{
	// TODO: If the value changed and we're windowed,
	// resize the window to compensate.
	Q_Q(GensWindow);
	QMenuBar *menuBar = q->menuBar();
	int height_adjust = 0;

	if (!isShowMenuBar()) {
		// Hide the menu bar.
		if (!q->isMaximized() && !q->isMinimized()) {
			height_adjust = -menuBar->height();
		}
		if (!isGlobalMenuBar()) {
			// System does not have a global menu bar.
			menuBar->setVisible(false);
		}
	} else {
		// Check if the menu bar was there already.
		const bool wasMenuBarThere = menuBar->isVisible();

		// Show the menu bar.
		menuBar->setVisible(true);

		// Adjust the menu bar size.
		if (!wasMenuBarThere && !q->isMaximized() && !q->isMinimized()) {
			menuBar->adjustSize();	// ensure the QMenuBar gets the correct size
			height_adjust = menuBar->height();
		}
	}

	// Hide "Show Menu Bar" if we're using a global menu bar.
	ui.actionGraphicsShowMenuBar->setVisible(!isGlobalMenuBar());

	if (!isGlobalMenuBar() && height_adjust != 0) {
		// Adjust the window height to compensate for the menu bar change.
		q->resize(q->width(), q->height() + height_adjust);
	}
}

}
