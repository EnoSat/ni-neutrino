/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2012-2013 Stefan Seyfried

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
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "infoviewer_bb.h"

#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>
#include <sys/timeb.h>
#include <sys/param.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <global.h>
#include <neutrino.h>

#include <gui/infoviewer.h>
#include <gui/bouquetlist.h>
#include "gui/keybind_setup.h"
#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/pictureviewer.h>
#include <gui/movieplayer.h>
#include <system/helpers.h>
#include <system/hddstat.h>
#include <daemonc/remotecontrol.h>
#include <driver/display.h>
#include <driver/radiotext.h>
#include <driver/volume.h>
#include <driver/fontrenderer.h>

#include <zapit/femanager.h>
#include <zapit/zapit.h>
#include <zapit/capmt.h> //NI

#include <hardware/video.h>

extern CRemoteControl *g_RemoteControl;	/* neutrino.cpp */
extern cVideo * videoDecoder;

#define COL_INFOBAR_BUTTONS_BACKGROUND (COL_MENUFOOT_PLUS_0)

CInfoViewerBB::CInfoViewerBB()
{
	frameBuffer = CFrameBuffer::getInstance();

	is_visible		= false;
	scrambledErr		= false;
	scrambledErrSave	= false;
	scrambledNoSig		= false;
	scrambledNoSigSave	= false;
	scrambledT		= 0;
#if 0
	if(!scrambledT) {
		pthread_create(&scrambledT, NULL, scrambledThread, (void*) this) ;
		pthread_detach(scrambledT);
	}
#endif
	hddscale 		= NULL;
	sysscale 		= NULL;
	bbIconInfo[0].x = 0;
	bbIconInfo[0].h = 0;
	BBarY = 0;
	BBarFontY = 0;
	foot			= NULL;
	ca_bar			= NULL;
	Init();
}

void CInfoViewerBB::Init()
{
	hddwidth		= 0;
	bbIconMaxH 		= 0;
	bbButtonMaxH 		= 0;
	bbIconMinX 		= 0;
	bbButtonMaxX 		= 0;
	fta			= true;
	minX			= 0;

	//NI init
	DecEndx = 0;
	decode = UNKNOWN;
	camCI = false;
	int CiSlots = cCA::GetInstance()->GetNumberCISlots();
	int acc = 0;
	while (acc < CiSlots && acc < 2) {
		if (cCA::GetInstance()->ModulePresent(CA_SLOT_TYPE_CI, acc)) {
			printf("CI: CAM found in Slot %i\n", acc);
			camCI = true;
		}
		else
			printf("CI: CAM not found\n");
		acc++;
	}

	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		tmp_bbButtonInfoText[i] = "";
		bbButtonInfo[i].x   = -1;
	}

	// not really a CComponentsFooter but let's take its height
	CComponentsFooter footer;
	InfoHeightY_Info = footer.getHeight();

	ca_h = 0;
	initBBOffset();

	changePB();
}

CInfoViewerBB::~CInfoViewerBB()
{
	if(scrambledT) {
		pthread_cancel(scrambledT);
		scrambledT = 0;
	}
	ResetModules();
}

CInfoViewerBB* CInfoViewerBB::getInstance()
{
	static CInfoViewerBB* InfoViewerBB = NULL;

	if(!InfoViewerBB) {
		InfoViewerBB = new CInfoViewerBB();
	}
	return InfoViewerBB;
}

bool CInfoViewerBB::checkBBIcon(const char * const icon, int *w, int *h)
{
	frameBuffer->getIconSize(icon, w, h);
	if ((*w != 0) && (*h != 0))
		return true;
	return false;
}

void CInfoViewerBB::getBBIconInfo()
{
	bbIconMaxH 		= 0;
	initBBOffset();
	BBarY 			= g_InfoViewer->BoxEndY + bottom_bar_offset;
	BBarFontY 		= BBarY + InfoHeightY_Info - (InfoHeightY_Info - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_FOOT]->getHeight()) / 2; /* center in buttonbar */
	bbIconMinX 		= g_InfoViewer->BoxEndX - OFFSET_INNER_MID;
	int curMode     = CNeutrinoApp::getInstance()->getMode();
	bool isRadioMode  = (curMode == NeutrinoModes::mode_radio || curMode == NeutrinoModes::mode_webradio);
	bool isTSMode     = (curMode == NeutrinoModes::mode_ts);
	bool firstIcon    = true;

	for (int i = 0; i < CInfoViewerBB::ICON_MAX; i++) {
		int w = 0, h = 0;
		bool iconView = false;
		switch (i) {
		case CInfoViewerBB::ICON_SUBT:  //no radio
			if (!isRadioMode)
				iconView = checkBBIcon(NEUTRINO_ICON_SUBT, &w, &h);
			break;
		case CInfoViewerBB::ICON_VTXT:  //no radio
			if (!isRadioMode)
				iconView = checkBBIcon(NEUTRINO_ICON_VTXT, &w, &h);
			break;
		case CInfoViewerBB::ICON_RT:
			if (isRadioMode && g_settings.radiotext_enable)
				iconView = checkBBIcon(NEUTRINO_ICON_RADIOTEXT_GET, &w, &h);
			break;
		case CInfoViewerBB::ICON_DD:
			if( g_settings.infobar_show_dd_available )
				iconView = checkBBIcon(NEUTRINO_ICON_DD, &w, &h);
			break;
		case CInfoViewerBB::ICON_16_9:  //no radio
			if (!isRadioMode)
				iconView = checkBBIcon(NEUTRINO_ICON_16_9, &w, &h);
			break;
		case CInfoViewerBB::ICON_RES:  //no radio
			if (!isRadioMode && g_settings.infobar_show_res < 2)
				iconView = checkBBIcon(NEUTRINO_ICON_RESOLUTION_1280, &w, &h);
			break;
		case CInfoViewerBB::ICON_CA:
			if (g_settings.infobar_casystem_display == 2)
				iconView = checkBBIcon(NEUTRINO_ICON_SCRAMBLED, &w, &h);
			break;
		case CInfoViewerBB::ICON_TUNER:
			if (CFEManager::getInstance()->getEnabledCount() > 1 && g_settings.infobar_show_tuner == 1 && !isTSMode && !IS_WEBCHAN(g_InfoViewer->get_current_channel_id()))
				iconView = checkBBIcon(NEUTRINO_ICON_TUNER, &w, &h);
			break;
		default:
			break;
		}
		if (iconView) {
			if (firstIcon)
				firstIcon = false;
			else
				bbIconMinX -= OFFSET_INNER_SMALL;
			bbIconMinX -= w;
			bbIconInfo[i].x = bbIconMinX;
			bbIconInfo[i].h = h;
		}
		else
			bbIconInfo[i].x = -1;
	}
	for (int i = 0; i < CInfoViewerBB::ICON_MAX; i++) {
		if (bbIconInfo[i].x != -1)
			bbIconMaxH = std::max(bbIconMaxH, bbIconInfo[i].h);
	}
	if (g_settings.infobar_show_sysfs_hdd)
		bbIconMinX -= hddwidth + OFFSET_INNER_MID; //NI
}

void CInfoViewerBB::getBBButtonInfo()
{
	bbButtonMaxH = 0;
	bbButtonMaxX = g_InfoViewer->ChanInfoX;
	int bbButtonMaxW = 0;
	int mode = NeutrinoModes::mode_unknown;
	int pers = -1;
	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		int w = 0, h = 0;
		bool active;
		std::string text, icon;
		switch (i) {
		case CInfoViewerBB::BUTTON_RED:
			pers = SNeutrinoSettings::P_MAIN_RED_BUTTON;
			icon = NEUTRINO_ICON_BUTTON_RED;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			mode = CNeutrinoApp::getInstance()->getMode();
			if (mode == NeutrinoModes::mode_ts) {
				text = CKeybindSetup::getMoviePlayerButtonName(CRCInput::RC_red, active, g_settings.infobar_buttons_usertitle);
				if (!text.empty())
					break;
			}
			text = CUserMenu::getUserMenuButtonName(0, active, g_settings.infobar_buttons_usertitle);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_RED]->title;
			break;
		case CInfoViewerBB::BUTTON_GREEN:
			pers = SNeutrinoSettings::P_MAIN_GREEN_BUTTON;
			icon = NEUTRINO_ICON_BUTTON_GREEN;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			mode = CNeutrinoApp::getInstance()->getMode();
			if (mode == NeutrinoModes::mode_ts) {
				text = CKeybindSetup::getMoviePlayerButtonName(CRCInput::RC_green, active, g_settings.infobar_buttons_usertitle);
				if (!text.empty())
					break;
			}
			text = CUserMenu::getUserMenuButtonName(1, active, g_settings.infobar_buttons_usertitle);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_GREEN]->title;
			break;
		case CInfoViewerBB::BUTTON_YELLOW:
			pers = SNeutrinoSettings::P_MAIN_YELLOW_BUTTON;
			icon = NEUTRINO_ICON_BUTTON_YELLOW;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			mode = CNeutrinoApp::getInstance()->getMode();
			if (mode == NeutrinoModes::mode_ts) {
				text = CKeybindSetup::getMoviePlayerButtonName(CRCInput::RC_yellow, active, g_settings.infobar_buttons_usertitle);
				if (!text.empty())
					break;
			}
			text = CUserMenu::getUserMenuButtonName(2, active, g_settings.infobar_buttons_usertitle);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_YELLOW]->title;
			break;
		case CInfoViewerBB::BUTTON_BLUE:
			pers = SNeutrinoSettings::P_MAIN_BLUE_BUTTON;
			icon = NEUTRINO_ICON_BUTTON_BLUE;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			mode = CNeutrinoApp::getInstance()->getMode();
			if (mode == NeutrinoModes::mode_ts) {
				text = CKeybindSetup::getMoviePlayerButtonName(CRCInput::RC_blue, active, g_settings.infobar_buttons_usertitle);
				if (!text.empty())
					break;
			}
			text = CUserMenu::getUserMenuButtonName(3, active, g_settings.infobar_buttons_usertitle);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_BLUE]->title;
			break;
		default:
			break;
		}
		//label audio control button in movieplayer mode
		if (mode == NeutrinoModes::mode_ts && !CMoviePlayerGui::getInstance().timeshift)
		{
			if (text == g_Locale->getText(LOCALE_MPKEY_AUDIO) && !g_settings.infobar_buttons_usertitle)
				text = CMoviePlayerGui::getInstance(false).CurrentAudioName(); // use instance_mp
		}
		bbButtonInfo[i].w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_FOOT]->getRenderWidth(text) + w + OFFSET_INNER_MID;
		bbButtonInfo[i].cx = w + OFFSET_INNER_SMALL;
		bbButtonInfo[i].h = h;
		bbButtonInfo[i].text = text;
		bbButtonInfo[i].icon = icon;
		if (pers > -1 && (g_settings.personalize[pers] != CPersonalizeGui::PERSONALIZE_ACTIVE_MODE_ENABLED))
			active = false;
		bbButtonInfo[i].active = active;
	}
	// Calculate position/size of buttons
	minX = std::min(bbIconMinX, g_InfoViewer->ChanInfoX + (((g_InfoViewer->BoxEndX - g_InfoViewer->ChanInfoX) * 75) / 100));
	int MaxBr = minX - (g_InfoViewer->ChanInfoX + OFFSET_INNER_MID);
	bbButtonMaxX = g_InfoViewer->ChanInfoX + OFFSET_INNER_MID;
	int br = 0, count = 0;
	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		if (!bbButtonInfo[i].active)
			bbButtonInfo[i].paint = false;
		else
		{
			count++;
			bbButtonInfo[i].paint = true;
			br += bbButtonInfo[i].w;
			bbButtonInfo[i].x = bbButtonMaxX;
			bbButtonMaxX += bbButtonInfo[i].w;
			bbButtonMaxW = std::max(bbButtonMaxW, bbButtonInfo[i].w);
		}
	}
	if (br > MaxBr)
		printf("[infoviewer_bb:%s#%d] width br (%d) > MaxBr (%d) count %d\n", __func__, __LINE__, br, MaxBr, count);
	bbButtonMaxX = g_InfoViewer->ChanInfoX + OFFSET_INNER_MID;
	int step = MaxBr / 4;
	if (count > 0) { /* avoid div-by-zero :-) */
		step = MaxBr / count;
		count = 0;
		for (int i = 0; i < BUTTON_MAX; i++) {
			if (!bbButtonInfo[i].paint)
				continue;
			bbButtonInfo[i].x = bbButtonMaxX + step * count;
			// printf("%s: i = %d count = %d b.x = %d\n", __func__, i, count, bbButtonInfo[i].x);
			count++;
		}
	} else {
		printf("[infoviewer_bb:%s#%d: count <= 0???\n", __func__, __LINE__);
		bbButtonInfo[BUTTON_RED].x	= bbButtonMaxX;
		bbButtonInfo[BUTTON_GREEN].x	= bbButtonMaxX + step;
		bbButtonInfo[BUTTON_YELLOW].x	= bbButtonMaxX + 2*step;
		bbButtonInfo[BUTTON_BLUE].x	= bbButtonMaxX + 3*step;
	}
}

void CInfoViewerBB::showBBButtons(bool paintFooter)
{
	if (!is_visible)
		return;
	int i;
	bool paint = false;

	if (g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_BOTTOM_LEFT || 
	    g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_BOTTOM_RIGHT || 
	    g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_BOTTOM_CENTER || 
	    g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_HIGHER_CENTER)
		g_InfoViewer->isVolscale = CVolume::getInstance()->hideVolscale();
	else
		g_InfoViewer->isVolscale = false;

	getBBButtonInfo();
	for (i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		if (tmp_bbButtonInfoText[i] != bbButtonInfo[i].text) {
			paint = true;
			break;
		}
	}

	if (paint) {
		fb_pixel_t *pixbuf = NULL;
		int buf_x = bbIconMinX - OFFSET_INNER_SMALL;
		int buf_y = BBarY;
		int buf_w = g_InfoViewer->BoxEndX-buf_x;
		int buf_h = InfoHeightY_Info;
		if (paintFooter) {
			pixbuf = new fb_pixel_t[buf_w * buf_h];
//printf("\nbuf_x: %d, buf_y: %d, buf_w: %d, buf_h: %d, pixbuf: %p\n \n", buf_x, buf_y, buf_w, buf_h, pixbuf);
			frameBuffer->SaveScreen(buf_x, buf_y, buf_w, buf_h, pixbuf);
			paintFoot();
			if (pixbuf != NULL) {
				if (g_settings.theme.infobar_gradient_bottom)
					frameBuffer->waitForIdle("CInfoViewerBB::showBBButtons");
				frameBuffer->RestoreScreen(buf_x, buf_y, buf_w, buf_h, pixbuf);
				delete [] pixbuf;
			}
		}
		int last_x = minX;
		for (i = BUTTON_MAX; i > 0;) {
			--i;
			if ((bbButtonInfo[i].x <= g_InfoViewer->ChanInfoX) || (bbButtonInfo[i].x >= g_InfoViewer->BoxEndX) || (!bbButtonInfo[i].paint))
				continue;
			if (bbButtonInfo[i].x > 0) {
				if (bbButtonInfo[i].x + bbButtonInfo[i].w > last_x) /* text too long */
					bbButtonInfo[i].w = last_x - bbButtonInfo[i].x;
				last_x = bbButtonInfo[i].x;
				if (bbButtonInfo[i].w - bbButtonInfo[i].cx <= 0) {
					printf("[infoviewer_bb:%d cannot paint icon %d (not enough space)\n",
							__LINE__, i);
					continue;
				}
				if (bbButtonInfo[i].active) {
					frameBuffer->paintIcon(bbButtonInfo[i].icon, bbButtonInfo[i].x, BBarY, InfoHeightY_Info);

					g_Font[SNeutrinoSettings::FONT_TYPE_MENU_FOOT]->RenderString(bbButtonInfo[i].x + bbButtonInfo[i].cx, BBarFontY,
							bbButtonInfo[i].w - bbButtonInfo[i].cx, bbButtonInfo[i].text, COL_MENUFOOT_TEXT);
				}
			}
		}

		for (i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
			tmp_bbButtonInfoText[i] = bbButtonInfo[i].text;
		}
	}
	if (g_InfoViewer->isVolscale)
		CVolume::getInstance()->showVolscale();
}

void CInfoViewerBB::showBBIcons(const int modus, const std::string & icon)
{
	if ((bbIconInfo[modus].x <= g_InfoViewer->ChanInfoX) || (bbIconInfo[modus].x >= g_InfoViewer->BoxEndX))
		return;
	if ((modus >= CInfoViewerBB::ICON_SUBT) && (modus < CInfoViewerBB::ICON_MAX) && (bbIconInfo[modus].x != -1) && (is_visible)) {
		frameBuffer->paintIcon(icon, bbIconInfo[modus].x, BBarY, 
				       InfoHeightY_Info, 1, true, !g_settings.theme.infobar_gradient_bottom, COL_INFOBAR_BUTTONS_BACKGROUND);
	}
}

void CInfoViewerBB::paintshowButtonBar(bool noTimer/*=false*/)
{
	if (!is_visible)
		return;
	getBBIconInfo();
	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		tmp_bbButtonInfoText[i] = "";
	}

	if (!noTimer)
		g_InfoViewer->sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	if (g_settings.infobar_casystem_display < 2)
		paint_ca_bar();

	paintFoot();

	g_InfoViewer->showSNR();

	// Buttons
	showBBButtons();

	//NI
	if (g_settings.infobar_casystem_display < 2)
		paint_cam_icons();

#if 0
	scrambledCheck(true);
#endif
	paint_ca_icons(0);

	// Icons, starting from right
	showIcon_SubT();
	showIcon_VTXT();
	showIcon_DD();
	showIcon_16_9();
	showIcon_Resolution();
	showIcon_Tuner();
	showSysfsHdd();
}

void CInfoViewerBB::paintFoot(int w)
{
	int width = (w == 0) ? g_InfoViewer->BoxEndX - g_InfoViewer->ChanInfoX : w;

	if (foot == NULL)
		foot = new CComponentsShapeSquare(g_InfoViewer->ChanInfoX, BBarY, width, InfoHeightY_Info, NULL, CC_SHADOW_ON);

	foot->setColorBody(COL_INFOBAR_BUTTONS_BACKGROUND);
	foot->enableColBodyGradient(g_settings.theme.infobar_gradient_bottom, COL_INFOBAR_PLUS_0, g_settings.theme.infobar_gradient_bottom_direction);
	foot->setCorner(RADIUS_LARGE, CORNER_BOTTOM);

	foot->paint(CC_SAVE_SCREEN_NO);
}

void CInfoViewerBB::showIcon_SubT()
{
	if (!is_visible)
		return;
	bool have_sub = false;
	CZapitChannel * cc = CNeutrinoApp::getInstance()->channelList->getChannel(CNeutrinoApp::getInstance()->channelList->getActiveChannelNumber());
	if (cc && cc->getSubtitleCount())
		have_sub = true;

	showBBIcons(CInfoViewerBB::ICON_SUBT, (have_sub) ? NEUTRINO_ICON_SUBT : NEUTRINO_ICON_SUBT_GREY);
#ifdef ENABLE_GRAPHLCD
	if (cc && cc->getSubtitleCount())
		cGLCD::lockIcon(cGLCD::SUB);
	else
		cGLCD::unlockIcon(cGLCD::SUB);
#endif
}

void CInfoViewerBB::showIcon_VTXT()
{
	if (!is_visible)
		return;
	showBBIcons(CInfoViewerBB::ICON_VTXT, (g_RemoteControl->current_PIDs.PIDs.vtxtpid != 0) ? NEUTRINO_ICON_VTXT : NEUTRINO_ICON_VTXT_GREY);
#ifdef ENABLE_GRAPHLCD
	if (g_RemoteControl->current_PIDs.PIDs.vtxtpid)
		cGLCD::lockIcon(cGLCD::TXT);
	else
		cGLCD::unlockIcon(cGLCD::TXT);
#endif
}

void CInfoViewerBB::showIcon_DD()
{
	if (!is_visible || !g_settings.infobar_show_dd_available)
		return;
	std::string dd_icon;
	if ((g_RemoteControl->current_PIDs.PIDs.selected_apid < g_RemoteControl->current_PIDs.APIDs.size()) && 
	    (g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3))
		dd_icon = NEUTRINO_ICON_DD;
	else
		dd_icon = g_RemoteControl->has_ac3 ? NEUTRINO_ICON_DD_AVAIL : NEUTRINO_ICON_DD_GREY;

	showBBIcons(CInfoViewerBB::ICON_DD, dd_icon);
#ifdef ENABLE_GRAPHLCD
	if ((g_RemoteControl->current_PIDs.PIDs.selected_apid < g_RemoteControl->current_PIDs.APIDs.size()) &&
	    (g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3))
		cGLCD::lockIcon(cGLCD::DD);
	else
		cGLCD::unlockIcon(cGLCD::DD);
#endif
}

void CInfoViewerBB::showIcon_RadioText(bool rt_available)
{
	if (!is_visible || !g_settings.radiotext_enable)
		return;

	std::string rt_icon;
	if (rt_available)
		rt_icon = (g_Radiotext->S_RtOsd) ? NEUTRINO_ICON_RADIOTEXT_GET : NEUTRINO_ICON_RADIOTEXT_WAIT;
	else
		rt_icon = NEUTRINO_ICON_RADIOTEXT_OFF;

	showBBIcons(CInfoViewerBB::ICON_RT, rt_icon);
}

void CInfoViewerBB::showIcon_16_9()
{
	if (!is_visible)
		return;

	if ((g_InfoViewer->aspectRatio == 0) || ( g_RemoteControl->current_PIDs.PIDs.vpid == 0 ) || (g_InfoViewer->aspectRatio != videoDecoder->getAspectRatio())) {
		if (g_InfoViewer->chanready &&
		   (g_RemoteControl->current_PIDs.PIDs.vpid > 0 || IS_WEBCHAN(g_InfoViewer->get_current_channel_id())))
			g_InfoViewer->aspectRatio = videoDecoder->getAspectRatio();
		else
			g_InfoViewer->aspectRatio = 0;

		showBBIcons(CInfoViewerBB::ICON_16_9, (g_InfoViewer->aspectRatio > 2) ? NEUTRINO_ICON_16_9 : NEUTRINO_ICON_16_9_GREY);
	}
}

void CInfoViewerBB::showIcon_Resolution()
{
	if ((!is_visible) || (g_settings.infobar_show_res == 2)) //show resolution icon is off
		return;
	if (CNeutrinoApp::getInstance()->getMode() == NeutrinoModes::mode_radio || CNeutrinoApp::getInstance()->getMode() == NeutrinoModes::mode_webradio)
		return;
	const char *icon_name = NULL;
#if 0
	if ((scrambledNoSig) || ((!fta) && (scrambledErr)))
#endif
	if (!g_InfoViewer->chanready || videoDecoder->getBlank())
	{
		icon_name = NEUTRINO_ICON_RESOLUTION_000;
	} else {
		int xres, yres, framerate;
		if (g_settings.infobar_show_res == 0) {//show resolution icon on infobar
			videoDecoder->getPictureInfo(xres, yres, framerate);
			switch (yres) {
			case 2160:
				icon_name = NEUTRINO_ICON_RESOLUTION_2160;
				break;
			case 1920:
				icon_name = NEUTRINO_ICON_RESOLUTION_1920;
				break;
			case 1088:
			case 1080:
				icon_name = NEUTRINO_ICON_RESOLUTION_1080;
				break;
			case 1440:
				icon_name = NEUTRINO_ICON_RESOLUTION_1440;
				break;
			case 1280:
				icon_name = NEUTRINO_ICON_RESOLUTION_1280;
				break;
			case 720:
				icon_name = NEUTRINO_ICON_RESOLUTION_720;
				break;
			case 704:
				icon_name = NEUTRINO_ICON_RESOLUTION_704;
				break;
			case 576:
				icon_name = NEUTRINO_ICON_RESOLUTION_576;
				break;
			case 544:
				icon_name = NEUTRINO_ICON_RESOLUTION_544;
				break;
			case 528:
				icon_name = NEUTRINO_ICON_RESOLUTION_528;
				break;
			case 480:
				icon_name = NEUTRINO_ICON_RESOLUTION_480;
				break;
			case 382:
				icon_name = NEUTRINO_ICON_RESOLUTION_382;
				break;
			case 352:
				icon_name = NEUTRINO_ICON_RESOLUTION_352;
				break;
			case 288:
				icon_name = NEUTRINO_ICON_RESOLUTION_288;
				break;
			default:
				icon_name = NEUTRINO_ICON_RESOLUTION_000;
				break;
			}
		}
		if (g_settings.infobar_show_res == 1) {//show simple resolution icon on infobar
			videoDecoder->getPictureInfo(xres, yres, framerate);
			if (yres > 1088)
				icon_name = NEUTRINO_ICON_RESOLUTION_UHD;
			else if (yres > 704)
				icon_name = NEUTRINO_ICON_RESOLUTION_HD;
			else if (yres > 0)
				icon_name = NEUTRINO_ICON_RESOLUTION_SD;
			else
				icon_name = NEUTRINO_ICON_RESOLUTION_000;
		}
	}
	showBBIcons(CInfoViewerBB::ICON_RES, icon_name);
}

void CInfoViewerBB::showIcon_CA()
{
	std::string sIcon = "";
#if 0
	if (CNeutrinoApp::getInstance()->getMode() != NeutrinoModes::mode_radio) {
		if (scrambledNoSig)
			sIcon = NEUTRINO_ICON_SCRAMBLED_BLANK;
		else {	
			if (fta)
				sIcon = NEUTRINO_ICON_SCRAMBLED_GREY;
			else
				sIcon = (scrambledErr) ? NEUTRINO_ICON_SCRAMBLED_RED : NEUTRINO_ICON_SCRAMBLED;
		}
	}
	else
#endif
		sIcon = (fta) ? NEUTRINO_ICON_SCRAMBLED_GREY : NEUTRINO_ICON_SCRAMBLED;
	showBBIcons(CInfoViewerBB::ICON_CA, sIcon);
}

void CInfoViewerBB::showIcon_Tuner()
{
	if (CFEManager::getInstance()->getEnabledCount() <= 1 ||
			!g_settings.infobar_show_tuner ||
			NeutrinoModes::mode_ts == CNeutrinoApp::getInstance()->getMode() ||
			IS_WEBCHAN(g_InfoViewer->get_current_channel_id())) {
		return;
	}

	char icon_name[12];
	snprintf(icon_name, sizeof(icon_name), "%s_%02d", NEUTRINO_ICON_TUNER, CFEManager::getInstance()->getLiveFE()->getNumber() + 1);

	int w = 0, h = 0;
	if (!checkBBIcon(icon_name, &w, &h))
		snprintf(icon_name, sizeof(icon_name), "%s", NEUTRINO_ICON_TUNER);

	showBBIcons(CInfoViewerBB::ICON_TUNER, icon_name);
}

void CInfoViewerBB::showSysfsHdd()
{
	if (g_settings.infobar_show_sysfs_hdd)
	{
		// sysfs info
		int percent = 0;
		uint64_t t, u;
		if (get_fs_usage("/", t, u))
			percent = (int)((u * 100ULL) / t);
		showBarSys(percent);

		// hdd info
		showBarHdd(cHddStat::getInstance()->getPercent());
	}
}

void CInfoViewerBB::showBarSys(int percent)
{	
	if (is_visible)
	{
		int sysscale_height = InfoHeightY_Info/4;
		sysscale->reset();
		sysscale->doPaintBg(false);
		sysscale->setDimensionsAll(bbIconMinX, BBarY + InfoHeightY_Info/2 - OFFSET_INNER_MIN - sysscale_height, hddwidth, sysscale_height);
		sysscale->setValues(percent, 100);
		sysscale->paint();
	}
}

void CInfoViewerBB::showBarHdd(int percent)
{
	if (is_visible)
	{
		int hddscale_height = InfoHeightY_Info/4;
		hddscale->reset();
		hddscale->doPaintBg(false);
		hddscale->setDimensionsAll(bbIconMinX, BBarY + InfoHeightY_Info/2 + OFFSET_INNER_MIN, hddwidth, hddscale_height);
		hddscale->setValues(percent, 100);
		hddscale->paint();

		if (percent < 0)
		{
			// mark undetected hdd fill level strike through
			frameBuffer->paintHLineRel(bbIconMinX, hddwidth, BBarY + InfoHeightY_Info/2 + OFFSET_INNER_MIN + hddscale_height/2, COL_INFOBAR_BUTTONS_BACKGROUND);
		}
	}
}

void CInfoViewerBB::paint_ca_icon(int caid, const char *icon, int &icon_space_offset)
{
	char buf[20];
	int endx = g_InfoViewer->BoxEndX - OFFSET_INNER_MID - (g_settings.infobar_casystem_frame ? FRAME_WIDTH_MIN + OFFSET_INNER_SMALL : 0);
	int py = g_InfoViewer->BoxEndY + OFFSET_INNER_SMALL;
	int px = 0;
	static std::map<int, std::pair<int,const char*> > icon_map;

	const int icon_space = OFFSET_INNER_SMALL, icon_number = 14;

	static int icon_offset[icon_number] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	static int icon_sizeW [icon_number] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	static bool init_flag = false;

	if (!init_flag) {
		init_flag = true;
		int icon_sizeH = 0, index = 0;
		std::map<int, std::pair<int,const char*> >::const_iterator it;

		icon_map[0x0000] = std::make_pair(index++,"dec"); //NI
		icon_map[0x0E00] = std::make_pair(index++,"powervu");
		icon_map[0x1000] = std::make_pair(index++,"tan");
		icon_map[0x2600] = std::make_pair(index++,"biss");
		icon_map[0x4A00] = std::make_pair(index++,"d");
		icon_map[0x0600] = std::make_pair(index++,"ird");
		icon_map[0x1700] = std::make_pair(index++,"bc");
		icon_map[0x0100] = std::make_pair(index++,"seca");
		icon_map[0x0500] = std::make_pair(index++,"via");
		icon_map[0x1800] = std::make_pair(index++,"nagra");
		icon_map[0x0B00] = std::make_pair(index++,"conax");
		icon_map[0x0D00] = std::make_pair(index++,"cw");
		icon_map[0x0900] = std::make_pair(index++,"nds");
		icon_map[0x5600] = std::make_pair(index  ,"vmx");

		for (it=icon_map.begin(); it!=icon_map.end(); ++it) {
			snprintf(buf, sizeof(buf), "%s_%s", (*it).second.second, (*it).second.first==0 ? "na" : "white"); //NI
			frameBuffer->getIconSize(buf, &icon_sizeW[(*it).second.first], &icon_sizeH);
		}

		for (int j = 0; j < icon_number; j++) {
			for (int i = j; i < icon_number; i++) {
				icon_offset[j] += icon_sizeW[i] + icon_space;
			}
		}
	}
	caid &= 0xFF00;

	if (icon_offset[icon_map[caid].first] == 0)
		return;

	if (g_settings.infobar_casystem_display == 0) {
		px = endx - (icon_offset[icon_map[caid].first] - icon_space );
	} else {
		icon_space_offset += icon_sizeW[icon_map[caid].first];
		px = endx - icon_space_offset;
		icon_space_offset += icon_space; //NI
	}

	if (px) {
		snprintf(buf, sizeof(buf), "%s_%s", icon_map[caid].second, icon);
		if ((px >= (endx-OFFSET_INNER_MID)) || (px <= 0))
			printf("#####[%s:%d] Error paint icon %s, px: %d,  py: %d, endx: %d, icon_offset: %d\n", 
				__FUNCTION__, __LINE__, buf, px, py, endx, icon_offset[icon_map[caid].first]);
		else if (strstr(buf,"dec_white") == 0) //NI
			frameBuffer->paintIcon(buf, px, py);
	}
}

void CInfoViewerBB::paint_ca_icons(int notfirst)
{
	//NI
	int acaid = 0;
	if (g_settings.show_ecm && (notfirst || g_settings.infobar_casystem_display > 1)) //bad mess :(
		acaid = check_ecmInfo();

	if (g_settings.infobar_casystem_display == 3)
		return;

	if(NeutrinoModes::mode_ts == CNeutrinoApp::getInstance()->getMode() && !CMoviePlayerGui::getInstance().timeshift){
		if (g_settings.infobar_casystem_display == 2) {
			fta = true;
			showIcon_CA();
		}
		return;
	}

	int caids[] = { 0x5600, 0x900, 0xD00, 0xB00, 0x1800, 0x0500, 0x0100, 0x1700, 0x600, 0x4a00, 0x2600, 0x1000, 0x0E00, 0x0000 };
	const char *green = "green"; //NI
	const char *white = "white";
	const char *yellow = "yellow";
	int icon_space_offset = 0;

	//NI
	const char *dec_icon_name[] = {"na","na","fta","int","card","net"};
	decode = UNKNOWN;

	if(!g_InfoViewer->chanready) {
		if (g_settings.infobar_casystem_display == 2) {
			fta = true;
			showIcon_CA();
		}
		else if(g_settings.infobar_casystem_display == 0) {
			for (int i = 0; i < (int)(sizeof(caids)/sizeof(int)); i++) {
				paint_ca_icon(caids[i], white, icon_space_offset);
			}
		}
		return;
	}

	CZapitChannel * channel = CZapit::getInstance()->GetCurrentChannel();
	if(!channel)
		return;

	if (g_settings.infobar_casystem_display == 2) {
		fta = channel->camap.empty();
		showIcon_CA();
		return;
	}

	if(!notfirst) {
		//NI - check ecm.info
		acaid = check_ecmInfo();
		if(acaid == 0){
			if(camCI)
				decode = CARD;
		}

		//NI - map betacrypt to nagra
		if((acaid & 0xFF00)== 0x1700 && (caids[3]& 0xFF00) == 0x1800)
			acaid=0x1800;

		for (int i = 0; i < (int)(sizeof(caids)/sizeof(int)); i++) {
			//printf("caids[%i] = %X\n",i,caids[i]); //NI
			bool dcaid = false; //NI
			bool found = false;
			for(casys_map_iterator_t it = channel->camap.begin(); it != channel->camap.end(); ++it) {
				int caid = (*it) & 0xFF00;
				if((found = (caid == caids[i])))
				{
					//NI
					//printf("   ****** caid = %X -  acaid = %X ******\n", caid, acaid & 0xFF00);
					dcaid = ((caid) == (acaid & 0xFF00));
					fta=false;

					break;
				}
			}
			//NI - decode info
			if(i == (int)(sizeof(caids)/sizeof(int))-1) {
				paint_ca_icon(caids[i], fta ? "fta" : dec_icon_name[decode], icon_space_offset);
				continue;
			}

			if(g_settings.infobar_casystem_display == 0)
				paint_ca_icon(caids[i], (found ? (dcaid ? green : yellow) : white), icon_space_offset); //NI
			else if(found)
				paint_ca_icon(caids[i], (dcaid ? green : yellow), icon_space_offset); //NI
		}

		//NI
		if (camCI)
			paint_cam_icons();
	}
}

void CInfoViewerBB::paint_ca_bar()
{
	initBBOffset();
	int ca_x = g_InfoViewer->ChanInfoX;
	int ca_y = g_InfoViewer->BoxEndY;
	int ca_w = g_InfoViewer->BoxEndX - g_InfoViewer->ChanInfoX;

	if (g_settings.infobar_casystem_frame)
	{
		ca_x += OFFSET_INNER_MID;
		ca_w -= 2*OFFSET_INNER_MID;
	}

	if (ca_bar == NULL)
		ca_bar = new CComponentsShapeSquare(ca_x, ca_y, ca_w, ca_h, NULL, CC_SHADOW_ON, COL_INFOBAR_CASYSTEM_PLUS_2, COL_INFOBAR_CASYSTEM_PLUS_0);
	//ca_bar->setColorBody(COL_INFOBAR_CASYSTEM_PLUS_0);
	ca_bar->enableColBodyGradient(g_settings.theme.infobar_gradient_bottom, COL_INFOBAR_BUTTONS_BACKGROUND, g_settings.theme.infobar_gradient_bottom_direction);
	ca_bar->enableShadow(CC_SHADOW_ON, OFFSET_SHADOW/2, true);

	if (g_settings.infobar_casystem_frame)
	{
		ca_bar->setFrameThickness(FRAME_WIDTH_MIN);
		ca_bar->setCorner(RADIUS_SMALL, CORNER_ALL);
	}

	ca_bar->paint(CC_SAVE_SCREEN_NO);

//NI
#if 0
	if (g_settings.infobar_casystem_dotmatrix)
	{
		int xcnt = (g_InfoViewer->BoxEndX - g_InfoViewer->ChanInfoX - (g_settings.infobar_casystem_frame ? 24 : 0)) / 4;
		int ycnt = (bottom_bar_offset - (g_settings.infobar_casystem_frame ? 14 : 0)) / 4;

		for (int i = 0; i < xcnt; i++)
		{
			for (int j = 0; j < ycnt; j++)
			{
				frameBuffer->paintBoxRel((g_InfoViewer->ChanInfoX + (g_settings.infobar_casystem_frame ? 14 : 2)) + i*4, g_InfoViewer->BoxEndY + (g_settings.infobar_casystem_frame ? 4 : 2) + j*4, 2, 2, COL_INFOBAR_PLUS_1);
			}
		}
	}
#endif
}

void CInfoViewerBB::changePB()
{
	hddwidth = frameBuffer->getScreenWidth(true) / 100 * 10; // 10 percent of screen width

	if (!hddscale) {
		hddscale = new CProgressBar();
		hddscale->setType(CProgressBar::PB_REDRIGHT);
	}
	
	if (!sysscale) {
		sysscale = new CProgressBar();
		sysscale->setType(CProgressBar::PB_REDRIGHT);
	}
}

void CInfoViewerBB::reset_allScala()
{
	hddscale->reset();
	sysscale->reset();
	//lasthdd = lastsys = -1;
}

void CInfoViewerBB::ResetModules()
{
	if (hddscale){
		delete hddscale; hddscale = NULL;
	}
	if (sysscale){
		delete sysscale; sysscale = NULL;
	}
	if (foot){
		delete foot; foot = NULL;
	}
	if (ca_bar){
		delete ca_bar; ca_bar = NULL;
	}
}

void CInfoViewerBB::initBBOffset()
{
	int icon_w = 0, icon_h = 0;
	frameBuffer->getIconSize("nagra_white", &icon_w, &icon_h); // take any ca icon to get its height
	ca_h = icon_h + 2*OFFSET_INNER_SMALL;

	bottom_bar_offset = 0;
	if (g_settings.infobar_casystem_display < 2)
	{
		if (g_settings.infobar_casystem_frame)
			bottom_bar_offset = ca_h + OFFSET_SHADOW/2 + OFFSET_INNER_SMALL;
		else
			bottom_bar_offset = ca_h;
	}
}

void* CInfoViewerBB::scrambledThread(void *arg)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	CInfoViewerBB *infoViewerBB = static_cast<CInfoViewerBB*>(arg);
	while(1) {
		if (infoViewerBB->is_visible)
			infoViewerBB->scrambledCheck();
		usleep(500*1000);
	}
	return 0;
}

void CInfoViewerBB::scrambledCheck(bool force)
{
	scrambledErr = false;
	scrambledNoSig = false;
	if (videoDecoder->getBlank()) {
		if (videoDecoder->getPlayState())
			scrambledErr = true;
		else
			scrambledNoSig = true;
	}
	
	if ((scrambledErr != scrambledErrSave) || (scrambledNoSig != scrambledNoSigSave) || force) {
		paint_ca_icons(0);
		showIcon_Resolution();
		scrambledErrSave = scrambledErr;
		scrambledNoSigSave = scrambledNoSig;
	}
}

//NI
void CInfoViewerBB::paint_cam_icons()
{
	std::ostringstream buf;
	int emu_pic_startx = g_InfoViewer->ChanInfoX + OFFSET_INNER_MID + (g_settings.infobar_casystem_frame ? FRAME_WIDTH_MIN + OFFSET_INNER_SMALL : 0);
	int py = g_InfoViewer->BoxEndY + OFFSET_INNER_SMALL;
	const char *icon_name[] = {"mgcamd","doscam","ncam","osmod","oscam","cccam","gbox"};
	const int icon_space = OFFSET_INNER_SMALL;
	int icon_sizeH = 0;
	int icon_sizeW = 0;
	bool useCI = CCamManager::getInstance()->getUseCI();
	bool filter_channels = CCamManager::getInstance()->getChannelFilter();

	for (int i=0; i < (int)(sizeof(icon_name)/sizeof(icon_name[0])); i++) {
		if ( getpidof(icon_name[i]) ) {
			buf.str("");
			buf << icon_name[i] << "_green";
			if(strstr(icon_name[i], "doscam") || strstr(icon_name[i], "ncam") || strstr(icon_name[i], "osmod") || strstr(icon_name[i], "oscam")) {
				if(camCI && useCI && filter_channels) {
					buf.str("");
					buf << icon_name[i] << "_yellow";
				}
			}
			frameBuffer->paintIcon(buf.str().c_str(), emu_pic_startx, py );
			frameBuffer->getIconSize(buf.str().c_str(), &icon_sizeW, &icon_sizeH);
			emu_pic_startx += icon_space;
			emu_pic_startx += icon_sizeW;
		}
	}

	if (camCI) {
		if (useCI)
			frameBuffer->paintIcon("ci+_green", emu_pic_startx, py);
		else
			frameBuffer->paintIcon("ci+_grey", emu_pic_startx, py);
	}
}

//NI ecm-Info
int CInfoViewerBB::check_ecmInfo()
{
	int caid = 0;
	CFileHelpers fh;
	if (fh.copyFile("/tmp/ecm.info", "/tmp/ecm.info.tmp", 0644)) {
		g_InfoViewer->md5_ecmInfo = filehash((char *)"/tmp/ecm.info.tmp");
		caid = parse_ecmInfo("/tmp/ecm.info.tmp");
	}
	return caid;
}

int CInfoViewerBB::parse_ecmInfo(const char * file)
{
	int acaid = 0;
	char *buffer;
	ssize_t read;
	size_t len;
	std::string ecm_txt("");
	FILE *fh;
	int w = 0;
	int ecm_width = 0;
	int ecm_height = 0;
	Font * ecm_font = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_ECMINFO];
	int font_height = ecm_font->getHeight();

	buffer=NULL;
	if((fh = fopen(file, "r")))
	{
		while ((read = getline(&buffer, &len, fh)) != -1)
		{
			if (g_settings.show_ecm)
			{
				w = ecm_font->getRenderWidth(buffer);
				ecm_width = std::max(w, ecm_width);
				ecm_height += font_height;
				ecm_txt += buffer;
			}

			if ( !acaid && strstr(buffer, "0x") )
			{
				if(sscanf(buffer, "%*[^9-0]%x", &acaid ) == 1)
					continue;
			} 
			else if ( strstr(buffer, "source:") ||		//mgcamd
				  strstr(buffer, "decode:") ||		//gbox
				  strstr(buffer, "address:") ||		//cccam
				  strstr(buffer, "protocol:") ||	//doscam or oscam constcw
				  strstr(buffer, "from:"))		//oscam
			{
				if ( strstr(buffer, "emu") ||		//mgcamd
					strstr(buffer, "constcw") ||	//doscam or oscam constcw
					strstr(buffer, "cache3") ||	//oscam with cacheex
					strstr(buffer, "Internal"))	//gbox
				{
					decode = LOCAL;
				}
				else if ( strstr(buffer, "slot") ||	//gbox
					  strstr(buffer, "/dev/sci") ||	//cccam
					  strstr(buffer, "local") ||	//oscam
					  strstr(buffer, "com"))
				{
					decode = CARD;
				}
				else if ( strstr(buffer, "net") ||	//mgcamd
					  strstr(buffer, "Network") ||	//gbox
					  strstr(buffer, "."))		//oscam
				{
					if ( strstr(buffer, "localhost") || strstr(buffer, "127.0.0.1"))
						decode = CARD;
					else
						decode = REMOTE;
				}
			}
		}
		fclose(fh);
		remove(file);
		if(buffer)
			free(buffer);
	}

	if (g_settings.show_ecm)
	{
		if(decode == UNKNOWN || decode == NA || ecm_txt.empty()) {
			g_InfoViewer->ecmInfoBox_hide();
		}
		else {
			g_InfoViewer->ecmInfoBox_show(ecm_txt.c_str(), ecm_width, ecm_height, ecm_font);
		}
	}

	return(acaid);
}
