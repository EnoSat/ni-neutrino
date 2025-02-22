/*
	$port: themes.cpp,v 1.16 2010/09/05 21:27:44 tuxbox-cvs Exp $

	Neutrino-GUI  -   DBoxII-Project

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	Copyright (C) 2007, 2008, 2009 (flasher) Frank Liebelt

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <global.h>
#include <neutrino.h>
#include "widget/menue.h"
#include <system/helpers.h>
#include <system/debug.h>
#include <system/setting_helpers.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/keyboard_input.h>
#include <gui/widget/msgbox.h>
#include <driver/screen_max.h>

#include <sys/stat.h>
#include <sys/time.h>

#include "themes.h"

#define USERDIR "/var" THEMESDIR
#define FILE_SUFFIX ".theme"

static 	SNeutrinoTheme &t = g_settings.theme;

CThemes::CThemes(): themefile('\t')
{
	width = 40;

	hasThemeChanged = false;
}

CThemes *CThemes::getInstance()
{
	static CThemes *th = NULL;

	if (!th)
	{
		th = new CThemes();
		dprintf(DEBUG_DEBUG, "CThemes Instance created\n");
	}
	return th;
}

int CThemes::exec(CMenuTarget *parent, const std::string &actionKey)
{
	int res = menu_return::RETURN_REPAINT;

	if (!actionKey.empty())
	{
		if (actionKey == "default_theme")
		{
			if (!applyDefaultTheme())
				setupDefaultColors(); // fallback
			changeNotify(NONEXISTANT_LOCALE, NULL);
		}
		else
		{
			std::string themeFile = actionKey;
			if (strstr(themeFile.c_str(), "{U}") != 0)
			{
				themeFile.erase(0, 3);
				readFile(((std::string)THEMESDIR_VAR + "/" + themeFile + FILE_SUFFIX).c_str());
			}
			else
				readFile(((std::string)THEMESDIR + "/" + themeFile + FILE_SUFFIX).c_str());
			g_settings.theme_name = themeFile;
		}
		OnAfterSelectTheme();
		CFrameBuffer::getInstance()->clearIconCache();
		return res;
	}


	if (parent)
		parent->hide();

	if (!hasThemeChanged)
		rememberOldTheme(true);

	return Show();
}

void CThemes::initThemesMenu(CMenuWidget &themes)
{
	struct dirent **themelist;
	int n;
	const char *pfade[] = {THEMESDIR, THEMESDIR_VAR};
	bool hasCVSThemes, hasUserThemes;
	hasCVSThemes = hasUserThemes = false;
	std::string userThemeFile = "";
	CMenuForwarder *oj;

#if 0
	// only to visualize if we have a migrated theme
	if (g_settings.theme_name.empty() || g_settings.theme_name == MIGRATE_THEME_NAME)
	{
		themes.addItem(new CMenuSeparator(CMenuSeparator::LINE));
		themes.addItem(new CMenuForwarder(MIGRATE_THEME_NAME, false, "", this));
	}
#endif

	for (int p = 0; p < 2; p++)
	{
		n = scandir(pfade[p], &themelist, 0, alphasort);
		if (n < 0)
			perror("loading themes: scandir");
		else
		{
			for (int count = 0; count < n; count++)
			{
				char *file = themelist[count]->d_name;
				char *pos = strstr(file, ".theme");
				if (pos != NULL)
				{
					if (p == 0 && hasCVSThemes == false)
					{
						themes.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORTHEMEMENU_SELECT2));
						hasCVSThemes = true;
					}
					else if (p == 1 && hasUserThemes == false)
					{
						themes.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORTHEMEMENU_SELECT1));
						hasUserThemes = true;
					}
					*pos = '\0';
					if (p == 1)
					{
						userThemeFile = "{U}" + (std::string)file;
						oj = new CMenuForwarder(file, true, "", this, userThemeFile.c_str());
					}
					else
						oj = new CMenuForwarder(file, true, "", this, file);

					themes.addItem(oj);
				}
				free(themelist[count]);
			}
			free(themelist);
		}
	}

	//on first paint exec this methode to set marker to item
	markSelectedTheme(&themes);

	//for all next menu paints markers are setted with this slot inside exec()
	if (OnAfterSelectTheme.empty())
		OnAfterSelectTheme.connect(sigc::bind(sigc::mem_fun(this, &CThemes::markSelectedTheme), &themes));
}

int CThemes::Show()
{
	move_userDir();

	std::string file_name = "";

	CMenuWidget themes(LOCALE_COLORMENU_MENUCOLORS, NEUTRINO_ICON_SETTINGS, width);
	sigc::slot0<void> slot_repaint = sigc::mem_fun(themes, &CMenuWidget::hide); //we want to clean screen before repaint after changed Option
	themes.OnBeforePaint.connect(slot_repaint);

	themes.addIntroItems(LOCALE_COLORTHEMEMENU_HEAD2);

	//set default theme
	std::string default_theme = DEFAULT_THEME;
	CMenuForwarder *fw = new CMenuForwarder(LOCALE_COLORTHEMEMENU_NEUTRINO_THEME, true, default_theme.c_str(), this, "default_theme", CRCInput::RC_red);
	themes.addItem(fw);
	fw->setHint("", LOCALE_COLORTHEMEMENU_NEUTRINO_THEME_HINT);

	initThemesMenu(themes);

	CKeyboardInput nameInput(LOCALE_COLORTHEMEMENU_NAME, &file_name);
	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_COLORTHEMEMENU_SAVE, true, NULL, &nameInput, NULL, CRCInput::RC_green);

	if (!CFileHelpers::createDir(THEMESDIR_VAR))
	{
		printf("[neutrino theme] error creating %s\n", THEMESDIR_VAR);
	}

	if (access(THEMESDIR_VAR, F_OK) == 0)
	{
		themes.addItem(GenericMenuSeparatorLine);
		themes.addItem(m1);
	}
	else
	{
		delete m1;
		printf("[neutrino theme] error accessing %s\n", THEMESDIR_VAR);
	}

	int res = themes.exec(NULL, "");

	if (!file_name.empty())
	{
		saveFile(((std::string)THEMESDIR_VAR + "/" + file_name + FILE_SUFFIX).c_str());
	}

	if (hasThemeChanged)
	{
		if (ShowMsg(LOCALE_MESSAGEBOX_INFO, LOCALE_COLORTHEMEMENU_QUESTION, CMsgBox::mbrYes, CMsgBox::mbYes | CMsgBox::mbNo, NEUTRINO_ICON_SETTINGS) != CMsgBox::mbrYes)
			rememberOldTheme(false);
		else
			hasThemeChanged = false;
	}
	return res;
}

void CThemes::rememberOldTheme(bool remember)
{
	if (remember)
	{
		oldTheme = t;
		oldTheme_name = g_settings.theme_name;
	}
	else
	{
		t = oldTheme;
		g_settings.theme_name = oldTheme_name;

		changeNotify(NONEXISTANT_LOCALE, NULL);
		hasThemeChanged = false;
	}
}

void CThemes::readFile(const char *themename)
{
	if (themefile.loadConfig(themename))
	{
		getTheme(themefile);

		changeNotify(NONEXISTANT_LOCALE, NULL);
		hasThemeChanged = true;
	}
	else
		printf("[neutrino theme] %s not found\n", themename);
}

void CThemes::saveFile(const char *themename)
{
	setTheme(themefile);

	if (themefile.getModifiedFlag())
	{
		printf("[neutrino theme] save theme into %s\n", themename);
		if (!themefile.saveConfig(themename))
			printf("[neutrino theme] %s write error\n", themename);
	}
}

bool CThemes::applyDefaultTheme()
{
	g_settings.theme_name = DEFAULT_THEME;
	std::string default_theme = THEMESDIR "/" + g_settings.theme_name + ".theme";
	if (themefile.loadConfig(default_theme))
	{
		getTheme(themefile);
		return true;
	}
	dprintf(DEBUG_NORMAL, "[CThemes]\t[%s - %d], No default theme found, creating migrated theme from current theme settings\n", __func__, __LINE__);
	return false;
}

// setup default Colors
void CThemes::setupDefaultColors()
{
	CConfigFile empty(':');
	getTheme(empty);
}

void CThemes::setTheme(CConfigFile &configfile)
{
	configfile.setInt32("menu_Head_alpha", t.menu_Head_alpha);
	configfile.setInt32("menu_Head_red", t.menu_Head_red);
	configfile.setInt32("menu_Head_green", t.menu_Head_green);
	configfile.setInt32("menu_Head_blue", t.menu_Head_blue);
	configfile.setInt32("menu_Head_Text_alpha", t.menu_Head_Text_alpha);
	configfile.setInt32("menu_Head_Text_red", t.menu_Head_Text_red);
	configfile.setInt32("menu_Head_Text_green", t.menu_Head_Text_green);
	configfile.setInt32("menu_Head_Text_blue", t.menu_Head_Text_blue);

	configfile.setInt32("menu_Head_gradient", t.menu_Head_gradient);
	configfile.setInt32("menu_Head_gradient_direction", t.menu_Head_gradient_direction);

	configfile.setInt32("menu_SubHead_gradient", t.menu_SubHead_gradient);
	configfile.setInt32("menu_SubHead_gradient_direction", t.menu_SubHead_gradient_direction);

	configfile.setInt32("menu_Separator_gradient_enable", t.menu_Separator_gradient_enable);

	configfile.setInt32("menu_Content_alpha", t.menu_Content_alpha);
	configfile.setInt32("menu_Content_red", t.menu_Content_red);
	configfile.setInt32("menu_Content_green", t.menu_Content_green);
	configfile.setInt32("menu_Content_blue", t.menu_Content_blue);
	configfile.setInt32("menu_Content_Text_alpha", t.menu_Content_Text_alpha);
	configfile.setInt32("menu_Content_Text_red", t.menu_Content_Text_red);
	configfile.setInt32("menu_Content_Text_green", t.menu_Content_Text_green);
	configfile.setInt32("menu_Content_Text_blue", t.menu_Content_Text_blue);
	configfile.setInt32("menu_Content_Selected_alpha", t.menu_Content_Selected_alpha);
	configfile.setInt32("menu_Content_Selected_red", t.menu_Content_Selected_red);
	configfile.setInt32("menu_Content_Selected_green", t.menu_Content_Selected_green);
	configfile.setInt32("menu_Content_Selected_blue", t.menu_Content_Selected_blue);
	configfile.setInt32("menu_Content_Selected_Text_alpha", t.menu_Content_Selected_Text_alpha);
	configfile.setInt32("menu_Content_Selected_Text_red", t.menu_Content_Selected_Text_red);
	configfile.setInt32("menu_Content_Selected_Text_green", t.menu_Content_Selected_Text_green);
	configfile.setInt32("menu_Content_Selected_Text_blue", t.menu_Content_Selected_Text_blue);
	configfile.setInt32("menu_Content_inactive_alpha", t.menu_Content_inactive_alpha);
	configfile.setInt32("menu_Content_inactive_red", t.menu_Content_inactive_red);
	configfile.setInt32("menu_Content_inactive_green", t.menu_Content_inactive_green);
	configfile.setInt32("menu_Content_inactive_blue", t.menu_Content_inactive_blue);
	configfile.setInt32("menu_Content_inactive_Text_alpha", t.menu_Content_inactive_Text_alpha);
	configfile.setInt32("menu_Content_inactive_Text_red", t.menu_Content_inactive_Text_red);
	configfile.setInt32("menu_Content_inactive_Text_green", t.menu_Content_inactive_Text_green);
	configfile.setInt32("menu_Content_inactive_Text_blue", t.menu_Content_inactive_Text_blue);
	configfile.setInt32("menu_Foot_alpha", t.menu_Foot_alpha);
	configfile.setInt32("menu_Foot_red", t.menu_Foot_red);
	configfile.setInt32("menu_Foot_green", t.menu_Foot_green);
	configfile.setInt32("menu_Foot_blue", t.menu_Foot_blue);
	configfile.setInt32("menu_Foot_Text_alpha", t.menu_Foot_Text_alpha);
	configfile.setInt32("menu_Foot_Text_red", t.menu_Foot_Text_red);
	configfile.setInt32("menu_Foot_Text_green", t.menu_Foot_Text_green);
	configfile.setInt32("menu_Foot_Text_blue", t.menu_Foot_Text_blue);

	configfile.setInt32("menu_Hint_gradient", t.menu_Hint_gradient);
	configfile.setInt32("menu_Hint_gradient_direction", t.menu_Hint_gradient_direction);
	configfile.setInt32("menu_ButtonBar_gradient", t.menu_ButtonBar_gradient);
	configfile.setInt32("menu_ButtonBar_gradient_direction", t.menu_ButtonBar_gradient_direction);

	configfile.setInt32("infobar_alpha", t.infobar_alpha);
	configfile.setInt32("infobar_red", t.infobar_red);
	configfile.setInt32("infobar_green", t.infobar_green);
	configfile.setInt32("infobar_blue", t.infobar_blue);
	configfile.setInt32("infobar_casystem_alpha", t.infobar_casystem_alpha);
	configfile.setInt32("infobar_casystem_red", t.infobar_casystem_red);
	configfile.setInt32("infobar_casystem_green", t.infobar_casystem_green);
	configfile.setInt32("infobar_casystem_blue", t.infobar_casystem_blue);
	configfile.setInt32("infobar_Text_alpha", t.infobar_Text_alpha);
	configfile.setInt32("infobar_Text_red", t.infobar_Text_red);
	configfile.setInt32("infobar_Text_green", t.infobar_Text_green);
	configfile.setInt32("infobar_Text_blue", t.infobar_Text_blue);

	configfile.setInt32("infobar_gradient_top", t.infobar_gradient_top);
	configfile.setInt32("infobar_gradient_top_direction", t.infobar_gradient_top_direction);
	configfile.setInt32("infobar_gradient_body", t.infobar_gradient_body);
	configfile.setInt32("infobar_gradient_body_direction", t.infobar_gradient_body_direction);
	configfile.setInt32("infobar_gradient_bottom", t.infobar_gradient_bottom);
	configfile.setInt32("infobar_gradient_bottom_direction", t.infobar_gradient_bottom_direction);

	configfile.setInt32("colored_events_alpha", t.colored_events_alpha);
	configfile.setInt32("colored_events_red", t.colored_events_red);
	configfile.setInt32("colored_events_green", t.colored_events_green);
	configfile.setInt32("colored_events_blue", t.colored_events_blue);
	configfile.setInt32("colored_events_channellist", t.colored_events_channellist);
	configfile.setInt32("colored_events_infobar", t.colored_events_infobar);

	configfile.setInt32("clock_Digit_alpha", t.clock_Digit_alpha);
	configfile.setInt32("clock_Digit_red", t.clock_Digit_red);
	configfile.setInt32("clock_Digit_green", t.clock_Digit_green);
	configfile.setInt32("clock_Digit_blue", t.clock_Digit_blue);

	configfile.setInt32("progressbar_design", t.progressbar_design);
	configfile.setInt32("progressbar_design_channellist", t.progressbar_design_channellist);
	configfile.setInt32("progressbar_gradient", t.progressbar_gradient);
	configfile.setInt32("progressbar_timescale_red", t.progressbar_timescale_red);
	configfile.setInt32("progressbar_timescale_green", t.progressbar_timescale_green);
	configfile.setInt32("progressbar_timescale_yellow", t.progressbar_timescale_yellow);
	configfile.setInt32("progressbar_timescale_invert", t.progressbar_timescale_invert);

	configfile.setInt32("shadow_alpha", t.shadow_alpha);
	configfile.setInt32("shadow_red", t.shadow_red);
	configfile.setInt32("shadow_green", t.shadow_green);
	configfile.setInt32("shadow_blue", t.shadow_blue);

	// progressbar
	configfile.setInt32("progressbar_active_red", t.progressbar_active_red);
	configfile.setInt32("progressbar_active_green", t.progressbar_active_green);
	configfile.setInt32("progressbar_active_blue", t.progressbar_active_blue);
	configfile.setInt32("progressbar_passive_red", t.progressbar_passive_red);
	configfile.setInt32("progressbar_passive_green", t.progressbar_passive_green);
	configfile.setInt32("progressbar_passive_blue", t.progressbar_passive_blue);

	// corners
	configfile.setInt32("rounded_corners", t.rounded_corners);

	// message frames
	configfile.setInt32("message_frame_enable", t.message_frame_enable);
}

void CThemes::getTheme(CConfigFile &configfile)
{
	t.menu_Head_alpha = configfile.getInt32("menu_Head_alpha", 10);
	t.menu_Head_red = configfile.getInt32("menu_Head_red", 0);
	t.menu_Head_green = configfile.getInt32("menu_Head_green", 0);
	t.menu_Head_blue = configfile.getInt32("menu_Head_blue", 0);
	t.menu_Head_Text_alpha = configfile.getInt32("menu_Head_Text_alpha", 0);
	t.menu_Head_Text_red = configfile.getInt32("menu_Head_Text_red", 99);
	t.menu_Head_Text_green = configfile.getInt32("menu_Head_Text_green", 43);
	t.menu_Head_Text_blue = configfile.getInt32("menu_Head_Text_blue", 7);

	t.menu_Head_gradient = configfile.getInt32("menu_Head_gradient", CC_COLGRAD_OFF);
	t.menu_Head_gradient_direction = configfile.getInt32("menu_Head_gradient_direction", CFrameBuffer::gradientVertical);

	t.menu_SubHead_gradient = configfile.getInt32("menu_SubHead_gradient", CC_COLGRAD_OFF);
	t.menu_SubHead_gradient_direction = configfile.getInt32("menu_SubHead_gradient_direction", CFrameBuffer::gradientVertical);

	t.menu_Separator_gradient_enable = configfile.getInt32("menu_Separator_gradient_enable", 0);

	t.menu_Content_alpha = configfile.getInt32("menu_Content_alpha", 10);
	t.menu_Content_red = configfile.getInt32("menu_Content_red", 13);
	t.menu_Content_green = configfile.getInt32("menu_Content_green", 13);
	t.menu_Content_blue = configfile.getInt32("menu_Content_blue", 13);
	t.menu_Content_Text_alpha = configfile.getInt32("menu_Content_Text_alpha", 0);
	t.menu_Content_Text_red = configfile.getInt32("menu_Content_Text_red", 98);
	t.menu_Content_Text_green = configfile.getInt32("menu_Content_Text_green", 98);
	t.menu_Content_Text_blue = configfile.getInt32("menu_Content_Text_blue", 98);
	t.menu_Content_Selected_alpha = configfile.getInt32("menu_Content_Selected_alpha", 10);
	t.menu_Content_Selected_red = configfile.getInt32("menu_Content_Selected_red", 99);
	t.menu_Content_Selected_green = configfile.getInt32("menu_Content_Selected_green", 43);
	t.menu_Content_Selected_blue = configfile.getInt32("menu_Content_Selected_blue", 7);
	t.menu_Content_Selected_Text_alpha = configfile.getInt32("menu_Content_Selected_Text_alpha", 0);
	t.menu_Content_Selected_Text_red = configfile.getInt32("menu_Content_Selected_Text_red", 0);
	t.menu_Content_Selected_Text_green = configfile.getInt32("menu_Content_Selected_Text_green", 0);
	t.menu_Content_Selected_Text_blue = configfile.getInt32("menu_Content_Selected_Text_blue", 0);
	t.menu_Content_inactive_alpha = configfile.getInt32("menu_Content_inactive_alpha", 10);
	t.menu_Content_inactive_red = configfile.getInt32("menu_Content_inactive_red", 13);
	t.menu_Content_inactive_green = configfile.getInt32("menu_Content_inactive_green", 13);
	t.menu_Content_inactive_blue = configfile.getInt32("menu_Content_inactive_blue", 13);
	t.menu_Content_inactive_Text_alpha = configfile.getInt32("menu_Content_inactive_Text_alpha", 0);
	t.menu_Content_inactive_Text_red = configfile.getInt32("menu_Content_inactive_Text_red", 62);
	t.menu_Content_inactive_Text_green = configfile.getInt32("menu_Content_inactive_Text_green", 62);
	t.menu_Content_inactive_Text_blue = configfile.getInt32("menu_Content_inactive_Text_blue", 62);

	t.menu_Hint_gradient = configfile.getInt32("menu_Hint_gradient", CC_COLGRAD_OFF);
	t.menu_Hint_gradient_direction = configfile.getInt32("menu_Hint_gradient_direction", CFrameBuffer::gradientVertical);
	t.menu_ButtonBar_gradient = configfile.getInt32("menu_ButtonBar_gradient", CC_COLGRAD_OFF);
	t.menu_ButtonBar_gradient_direction = configfile.getInt32("menu_ButtonBar_gradient_direction", CFrameBuffer::gradientVertical);

	t.infobar_alpha = configfile.getInt32("infobar_alpha", 10);
	t.infobar_red = configfile.getInt32("infobar_red", 13);
	t.infobar_green = configfile.getInt32("infobar_green", 13);
	t.infobar_blue = configfile.getInt32("infobar_blue", 13);

	//t.menu_Foot default historically depends on t.infobar
	t.menu_Foot_alpha = configfile.getInt32("menu_Foot_alpha", 10);
	t.menu_Foot_red = configfile.getInt32("menu_Foot_red", 0);
	t.menu_Foot_green = configfile.getInt32("menu_Foot_green", 0);
	t.menu_Foot_blue = configfile.getInt32("menu_Foot_blue", 0);

	t.infobar_gradient_top = configfile.getInt32("infobar_gradient_top", CC_COLGRAD_OFF);
	t.infobar_gradient_top_direction = configfile.getInt32("infobar_gradient_top_direction", CFrameBuffer::gradientVertical);
	t.infobar_gradient_body = configfile.getInt32("infobar_gradient_body", CC_COLGRAD_OFF);
	t.infobar_gradient_body_direction = configfile.getInt32("infobar_gradient_body_direction", CFrameBuffer::gradientVertical);
	t.infobar_gradient_bottom = configfile.getInt32("infobar_gradient_bottom", CC_COLGRAD_OFF);
	t.infobar_gradient_bottom_direction = configfile.getInt32("infobar_gradient_bottom_direction", CFrameBuffer::gradientVertical);

	t.infobar_casystem_alpha = configfile.getInt32("infobar_casystem_alpha", 10);
	t.infobar_casystem_red = configfile.getInt32("infobar_casystem_red", 13);
	t.infobar_casystem_green = configfile.getInt32("infobar_casystem_green", 13);
	t.infobar_casystem_blue = configfile.getInt32("infobar_casystem_blue", 13);
	t.infobar_Text_alpha = configfile.getInt32("infobar_Text_alpha", 0);
	t.infobar_Text_red = configfile.getInt32("infobar_Text_red", 98);
	t.infobar_Text_green = configfile.getInt32("infobar_Text_green", 98);
	t.infobar_Text_blue = configfile.getInt32("infobar_Text_blue", 98);

	//t.menu_Foot_Text default historically depends on t.infobar_Text
	t.menu_Foot_Text_alpha = configfile.getInt32("menu_Foot_Text_alpha", 0);
	t.menu_Foot_Text_red = configfile.getInt32("menu_Foot_Text_red", 98);
	t.menu_Foot_Text_green = configfile.getInt32("menu_Foot_Text_green", 98);
	t.menu_Foot_Text_blue = configfile.getInt32("menu_Foot_Text_blue", 98);

	t.colored_events_alpha = configfile.getInt32("colored_events_alpha", 0);
	t.colored_events_red = configfile.getInt32("colored_events_red", 99);
	t.colored_events_green = configfile.getInt32("colored_events_green", 43);
	t.colored_events_blue = configfile.getInt32("colored_events_blue", 7);
	t.colored_events_channellist = configfile.getInt32("colored_events_channellist", 1);

	t.colored_events_infobar = configfile.getInt32("colored_events_infobar", 1);
	t.clock_Digit_alpha = configfile.getInt32("clock_Digit_alpha", 0);
	t.clock_Digit_red = configfile.getInt32("clock_Digit_red", 62);
	t.clock_Digit_green = configfile.getInt32("clock_Digit_green", 62);
	t.clock_Digit_blue = configfile.getInt32("clock_Digit_blue", 62);

	t.progressbar_design = configfile.getInt32("progressbar_design", CProgressBar::PB_MONO);
	t.progressbar_design_channellist = configfile.getInt32("progressbar_design_channellist", t.progressbar_design);
	t.progressbar_gradient = configfile.getInt32("progressbar_gradient", 1);
	t.progressbar_timescale_red = configfile.getInt32("progressbar_timescale_red", 0);
	t.progressbar_timescale_green = configfile.getInt32("progressbar_timescale_green", 100);
	t.progressbar_timescale_yellow = configfile.getInt32("progressbar_timescale_yellow", 70);
	t.progressbar_timescale_invert = configfile.getInt32("progressbar_timescale_invert", 0);

	t.shadow_alpha = configfile.getInt32("shadow_alpha", 25);
	t.shadow_red = configfile.getInt32("shadow_red", 0);
	t.shadow_green = configfile.getInt32("shadow_green", 0);
	t.shadow_blue = configfile.getInt32("shadow_blue", 0);

	// progressbar
	t.progressbar_active_red = configfile.getInt32("progressbar_active_red", 62);
	t.progressbar_active_green = configfile.getInt32("progressbar_active_green", 62);
	t.progressbar_active_blue = configfile.getInt32("progressbar_active_blue", 62);
	t.progressbar_passive_red = configfile.getInt32("progressbar_passive_red", 26);
	t.progressbar_passive_green = configfile.getInt32("progressbar_passive_green", 26);
	t.progressbar_passive_blue = configfile.getInt32("progressbar_passive_blue", 26);

	// corners
	t.rounded_corners = configfile.getInt32("rounded_corners", 0);

	// message frames
	t.message_frame_enable = configfile.getInt32("message_frame_enable", 0);

	if (g_settings.theme_name.empty())
		applyDefaultTheme();
}

void CThemes::move_userDir()
{
	if (access(USERDIR, F_OK) == 0)
	{
		if (!CFileHelpers::createDir(THEMESDIR_VAR))
		{
			printf("[neutrino theme] error creating %s\n", THEMESDIR_VAR);
			return;
		}
		struct dirent **themelist;
		int n = scandir(USERDIR, &themelist, 0, alphasort);
		if (n < 0)
		{
			perror("loading themes: scandir");
			return;
		}
		else
		{
			for (int count = 0; count < n; count++)
			{
				const char *file = themelist[count]->d_name;
				if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0)
					continue;
				const char *dest = ((std::string)USERDIR + "/" + file).c_str();
				const char *target = ((std::string)THEMESDIR_VAR + "/" + file).c_str();
				printf("[neutrino theme] moving %s to %s\n", dest, target);
				rename(dest, target);
				free(themelist[count]);
			}
			free(themelist);
		}
		printf("[neutrino theme] removing %s\n", USERDIR);
		remove(USERDIR);
	}
}

void CThemes::markSelectedTheme(CMenuWidget *w)
{
	for (int i = 0; i < w->getItemsCount(); i++)
	{
		w->getItem(i)->setInfoIconRight(NULL);
		if (g_settings.theme_name == w->getItem(i)->getName())
			w->getItem(i)->setInfoIconRight(NEUTRINO_ICON_MARKER_DIALOG_OK);
	}
}
