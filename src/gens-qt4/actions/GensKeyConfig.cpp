/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * GensKeyConfig.hpp: Gens key configuration.                              *
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

#include "GensKeyConfig.hpp"

// Menu actions.
#include "GensMenuBar_menus.hpp"

// Qt includes.
#include <QtCore/QSettings>

// TODO: Create a typedef GensKeyM_t to indicate "with modifiers"?

namespace GensQt4
{

class GensKeyConfigPrivate
{
	public:
		GensKeyConfigPrivate() { }
		
		/**
		 * Key configuration.
		 * 
		 * NOTE: Key configuration expects modifiers in high 7 bits of GensKey_t.
		 * See libgens/GensInput/GensKey_t.h for more information.
		 */
		
		/// m_hashActionToKey: Converts a GensMenuBar_menus.hpp value to a GensKey_t.
		QHash<int, GensKey_t> hashActionToKey;
		
		/// m_hashActionToKey: Converts a GensKey_t to a GensMenuBar_menus.hpp value.
		QHash<GensKey_t, int> hashKeyToAction;
		
		struct DefKeySetting_t
		{
			int action;		// GensMenuBar_menus.hpp value.
			GensKey_t gensKey;	// Default GensKey_t.
			
			const char *setting;	// Settings location.
			// TODO: Padding on 32-bit.
		};
		
		/**
		 * DefKeySettings[]: Default key settings.
		 */
		static const DefKeySetting_t DefKeySettings[];
	
	private:
		Q_DISABLE_COPY(GensKeyConfigPrivate);
};


/**
 * ms_DefKeySettings[]: Default key settings.
 */
const GensKeyConfigPrivate::DefKeySetting_t GensKeyConfigPrivate::DefKeySettings[] =
{
	// Non-menu keys.
	{IDM_NOMENU_HARDRESET, KEYV_TAB | KEYM_SHIFT,		"other/hardReset"},
	{IDM_NOMENU_SOFTRESET, KEYV_TAB,			"other/softReset"},
	{IDM_NOMENU_PAUSE, KEYV_ESCAPE,				"other/pause"},
	
	{IDM_NOMENU_FASTBLUR, KEYV_F9,				"other/fastBlur"},
	
	{IDM_NOMENU_SAVESLOT_0, KEYV_0,				"other/saveSlot0"},
	{IDM_NOMENU_SAVESLOT_1, KEYV_1,				"other/saveSlot1"},
	{IDM_NOMENU_SAVESLOT_2, KEYV_2,				"other/saveSlot2"},
	{IDM_NOMENU_SAVESLOT_3, KEYV_3,				"other/saveSlot3"},
	{IDM_NOMENU_SAVESLOT_4, KEYV_4,				"other/saveSlot4"},
	{IDM_NOMENU_SAVESLOT_5, KEYV_5,				"other/saveSlot5"},
	{IDM_NOMENU_SAVESLOT_6, KEYV_6,				"other/saveSlot6"},
	{IDM_NOMENU_SAVESLOT_7, KEYV_7,				"other/saveSlot7"},
	{IDM_NOMENU_SAVESLOT_8, KEYV_8,				"other/saveSlot8"},
	{IDM_NOMENU_SAVESLOT_9, KEYV_9,				"other/saveSlot9"},
	{IDM_NOMENU_SAVESLOT_PREV, KEYV_F6,			"other/saveSlotPrev"},
	{IDM_NOMENU_SAVESLOT_NEXT, KEYV_F7,			"other/saveSlotNext"},
	{IDM_NOMENU_SAVESLOT_LOADFROM, KEYV_F8 | KEYM_SHIFT,	"other/saveLoadFrom"},
	{IDM_NOMENU_SAVESLOT_SAVEAS, KEYV_F5 | KEYM_SHIFT,	"other/saveSaveAs"},
	
	// End of default keys.
	{0, 0, NULL}
};


GensKeyConfig::GensKeyConfig()
{
	d = new GensKeyConfigPrivate();
	
	// Load the default key configuration.
	for (const GensKeyConfigPrivate::DefKeySetting_t *key = &d->DefKeySettings[0];
	    key->action != 0; key++)
	{
		d->hashActionToKey.insert(key->action, key->gensKey);
		d->hashKeyToAction.insert(key->gensKey, key->action);
	}
}

GensKeyConfig::~GensKeyConfig()
{
	delete d;
}


/**
 * keyToAction(): Look up an action based on a GensKey_t value.
 * @param key GensKey_t value. (WITH MODIFIERS)
 * @return Action, or 0 if no action was found.
 */
int GensKeyConfig::keyToAction(GensKey_t key)
{
	// TODO: Menus. This only works for non-menu actions right now.
	return d->hashKeyToAction.value(key, 0);
}


/**
 * load(): Load key configuration from a settings file.
 * NOTE: The group must be selected in the QSettings before calling this function!
 * @param settings Settings file.
 * @return 0 on success; non-zero on error.
 */
int GensKeyConfig::load(const QSettings& settings)
{
	// Clear the hash tables before loading.
	d->hashActionToKey.clear();
	d->hashKeyToAction.clear();
	
	// Load the key configuration.
	for (const GensKeyConfigPrivate::DefKeySetting_t *key = &d->DefKeySettings[0];
	    key->action != 0; key++)
	{
		const GensKey_t gensKey = settings.value(QLatin1String(key->setting), key->gensKey).toString().toUInt(NULL, 0);
		d->hashActionToKey.insert(key->action, gensKey);
		d->hashKeyToAction.insert(gensKey, key->action);
	}
	
	// Key configuration loaded.
	return 0;
}


/**
 * save(): Save key configuration to a settings file.
 * NOTE: The group must be selected in the QSettings before calling this function!
 * @param settings Settings file.
 * @return 0 on success; non-zero on error.
 */
int GensKeyConfig::save(QSettings& settings)
{
	// Save the key configuration.
	for (const GensKeyConfigPrivate::DefKeySetting_t *key = &d->DefKeySettings[0];
	    key->action != 0; key++)
	{
		const GensKey_t gensKey = d->hashActionToKey.value(key->action, 0);
		QString gensKey_str = QLatin1String("0x") +
				QString::number(gensKey, 16).toUpper().rightJustified(4, QChar(L'0'));
		
		settings.setValue(QLatin1String(key->setting), gensKey_str);
	}
	
	// Key configuration saved.
	return 0;
}

}
