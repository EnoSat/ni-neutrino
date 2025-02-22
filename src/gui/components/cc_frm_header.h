/*
	Based up Neutrino-GUI - Tuxbox-Project 
	Copyright (C) 2001 by Steffen Hehn 'McClean'

	Classes for generic GUI-related components.
	Copyright (C) 2012, 2013, 2014, Thilo Graf 'dbt'

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __CC_FORM_HEADER_H__
#define __CC_FORM_HEADER_H__


#include "cc_frm.h"
#include "cc_item_picture.h"
#include "cc_item_text.h"
#include "cc_frm_icons.h"
#include "cc_frm_clock.h"
#include <driver/colorgradient.h>

#define DEFAULT_LOGO_ALIGN CCHeaderTypes::CC_LOGO_RIGHT
#define DEFAULT_TITLE_ALIGN CCHeaderTypes::CC_TITLE_LEFT

class CCHeaderTypes
{
	public:
		///logo position options
		typedef enum
		{
			CC_LOGO_RIGHT 	= 0x01,
			CC_LOGO_LEFT 	= 0x02,
			CC_LOGO_CENTER  = 0x04
		}cc_logo_alignment_t;

		///title position options
		typedef enum
		{	/*for compatibilty use CTextBox enums values*/
			CC_TITLE_LEFT 	= 0x400,
			CC_TITLE_CENTER	= 0x40 ,
			CC_TITLE_RIGHT	= 0x80
		}cc_title_alignment_t;

	protected:
		///required logo data type
		typedef struct cch_logo_t
		{
			uint64_t Id;
			std::string Name;
			int32_t	dx_max;
			int32_t	dy_max;
			cc_logo_alignment_t Align;
		} cch_logo_struct_t;
};


//! Sub class of CComponentsForm. Shows a header with prepared items.
/*!
CComponentsHeader provides prepared items like icon, caption and context button icons, mostly for usage in menues or simple windows
*/

class CComponentsHeader : public CComponentsForm, public CCTextScreen, CCHeaderTypes
{
	private:
		///member: init genaral variables, parameters for mostly used properties
		void initVarHeader(	const int& x_pos, const int& y_pos, const int& w, const int& h,
					const std::string& caption,
					const std::string& icon_name,
					const int& buttons,
					CComponentsForm *parent,
					int shadow_mode,
					fb_pixel_t color_frame,
					fb_pixel_t color_body,
					fb_pixel_t color_shadow,
					int sizeMode
				);

	protected:
		///object: icon object, see also setIcon()
		CComponentsPicture * cch_icon_obj;
		///object: caption object, see also setCaption()
		CComponentsText * cch_text_obj;
		///object: context button object, see also addContextButton(), removeContextButtons()
		CComponentsIconForm * cch_btn_obj;
		///object: clock object
		CComponentsFrmClock * cch_cl_obj;
		///object: logo object
		CComponentsChannelLogo * cch_logo_obj;

		///attributes for logos
		cch_logo_t cch_logo;

		///property: caption text, see also setCaption()
		std::string cch_text;
		///property: icon name, see also setIcon()
		std::string  cch_icon_name;
		///property: caption text color, see also setCaptionColor()
		fb_pixel_t cch_col_text;
		///property: caption font, see also setCaptionFont()
		Font* cch_font, *l_font, *s_font;
		///reset font
		void resetFont();

		///property: internal y-position for all items
		int cch_items_y;
		///property: internal x-position for icon object
		int cch_icon_x;
		///property: internal width for icon object
		int cch_icon_w;
		///property: internal width for clock object
		int cch_clock_w;
		///property: internal x-position for caption object
		int cch_text_x;
		///property: internal offset of context button icons within context button object
		int cch_buttons_space;
		///property: internal offset for header items
		int cch_offset;
		///property: internal container of icon names for context button object, see also addContextButton()
		std::vector<std::string> v_cch_btn;
		///property: size of header, possible values are CC_HEADER_SIZE_LARGE, CC_HEADER_SIZE_SMALL
		int cch_size_mode;
		///property: alignment of caption within header, see also setCaptionAlignment()
		cc_title_alignment_t cch_caption_align;
		///property: enable/disable of clock, see also enableClock()
		bool cch_cl_enable;
		///property: clock format
		const char* cch_cl_format;
		///property: secondary clock format
		const char* cch_cl_sec_format;
		///property: enable running clock
		bool cch_cl_enable_run;

		///init font object and recalculates height if required
		void initCaptionFont();
		///init default fonts for size modes
		void initDefaultFonts();
		///init large or small mode considered assigned height
		void initSizeMode();
		///sub: init icon object
		void initIcon();
		///sub: init caption object
		void initCaption();
		///sub: init context button object
		void initButtons();
		///sub: init clock object
		void initClock();
		///sub: init logo object
		void initLogo();

		///int repaint slot
		void initRepaintSlot();

	public:
		enum
		{
			CC_HEADER_ITEM_ICON 	= 0,
			CC_HEADER_ITEM_TEXT 	= 1,
			CC_HEADER_ITEM_BUTTONS	= 2
		};

		enum
		{
			CC_HEADER_SIZE_LARGE 	= 0,
			CC_HEADER_SIZE_SMALL 	= 1
		};

		CComponentsHeader(CComponentsForm *parent = NULL);

		CComponentsHeader(	const int& x_pos, const int& y_pos, const int& w, const int& h = 0,
					int sizeMode = CC_HEADER_SIZE_LARGE,
					const std::string& caption = std::string(),
					CComponentsForm *parent = NULL
				);

		CComponentsHeader(	const int& x_pos, const int& y_pos, const int& w, const int& h = 0,
					int sizeMode = CC_HEADER_SIZE_LARGE,
					neutrino_locale_t caption_locale = NONEXISTANT_LOCALE,
					CComponentsForm *parent = NULL
				);

		CComponentsHeader(	const int& x_pos, const int& y_pos, const int& w, const int& h = 0,
					const std::string& caption = std::string(),
					const std::string& icon_name = std::string(),
					const int& buttons = 0,
					CComponentsForm *parent = NULL,
					int shadow_mode = CC_SHADOW_OFF,
					fb_pixel_t color_frame = COL_FRAME_PLUS_0,
					fb_pixel_t color_body = COL_MENUHEAD_PLUS_0,
					fb_pixel_t color_shadow = COL_SHADOW_PLUS_0,
					int sizeMode = CC_HEADER_SIZE_LARGE
				);

		CComponentsHeader(	const int& x_pos, const int& y_pos, const int& w, const int& h = 0,
					neutrino_locale_t caption_locale = NONEXISTANT_LOCALE,
					const std::string& icon_name = std::string(),
					const int& buttons = 0,
					CComponentsForm *parent = NULL,
					int shadow_mode = CC_SHADOW_OFF,
					fb_pixel_t color_frame = COL_FRAME_PLUS_0,
					fb_pixel_t color_body = COL_MENUHEAD_PLUS_0,
					fb_pixel_t color_shadow = COL_SHADOW_PLUS_0,
					int sizeMode = CC_HEADER_SIZE_LARGE
				);

		virtual ~CComponentsHeader();

		///set caption text, parameters: string, int align_mode (default left) 
		void setCaption(const std::string& caption, const cc_title_alignment_t& align_mode = DEFAULT_TITLE_ALIGN, const fb_pixel_t& text_color = COL_MENUHEAD_TEXT);
		///set caption text, parameters: loacle, int align_mode (default left)
		void setCaption(neutrino_locale_t caption_locale, const cc_title_alignment_t& align_mode = DEFAULT_TITLE_ALIGN, const fb_pixel_t& text_color = COL_MENUHEAD_TEXT);

		///set alignment of caption within header, possible paramters are CComponentsHeader::CC_TITLE_LEFT, CComponentsHeader::CC_TITLE_RIGHT, CComponentsHeader::CC_TITLE_CENTER
		void setCaptionAlignment(const cc_title_alignment_t& align_mode){cch_caption_align = align_mode;}

		/**Set text font for title.
		 * Internal default font is g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE] and
		 * default height of header object is calculated from this font type.
		 * Height can be changed with modes by setSizeMode(), setHeight() or constructor.
		 * @return void
		 *
		 * @param[in] font	expects font object, type Font*
		 * @see			getCaptionFont(), setSizeMode(),
		 * 			setCaptionColor(),
		 * 			setCaptionAlignment(),
		 * 			setCaption()
		*/
		void setCaptionFont(Font* font);
		///returns font object of title caption
		Font* getCaptionFont(){return cch_font;}
		///set text color for caption
		void setCaptionColor(fb_pixel_t text_color){cch_col_text = text_color;}

		///set offset between items
		void setOffset(const int offset){cch_offset = offset;};
		///set name of icon
		void setIcon(const char* icon_name);
		///set name of icon
		void setIcon(const std::string& icon_name);

		///context buttons are to find on the right part of header
		///add a single context button icon to the header object, arg as string, icon will just add, existing icons are preserved
		void addContextButton(const std::string& icon_name);
		///add a group of context button icons to the header object, arg as string vector, icons will just add, existing icons are preserved
		void addContextButton(const std::vector<std::string>& v_icon_names);
		///add a single context button icon or combined button icons to the header object, possible types are for example: CC_BTN_HELP, CC_BTN_INFO, CC_BTN_MENU, CC_BTN_EXIT
		///icons will just add, existing  icons are preserved
		void addContextButton(const int& buttons);
		///remove context buttons from context button object
		void removeContextButtons();
		///sets a single context button icon to the header object, arg as string, existing buttons are removed
		void setContextButton(const std::string& icon_name){removeContextButtons(); addContextButton(icon_name);};
		///sets a group of context button icons to the header object, arg as string vector, existing buttons are removed
		void setContextButton(const std::vector<std::string>& v_icon_names){removeContextButtons(); addContextButton(v_icon_names);};
		///sets a single context button icon or combined button icons to the header object, possible types are for example: CC_BTN_HELP, CC_BTN_INFO, CC_BTN_MENU, CC_BTN_EXIT
		///existing buttons are removed
		void setContextButton(const int& buttons){removeContextButtons(); addContextButton(buttons);};

		///gets the embedded context button object, so it's possible to get access directly to its methods and properties
		CComponentsIconForm* getContextBtnObject() { return cch_btn_obj;};

		enum
		{
			CC_BTN_HELP 			= 0x02,
			CC_BTN_INFO 			= 0x04,
			CC_BTN_MENU 			= 0x40,
			CC_BTN_EXIT 			= 0x80,
			CC_BTN_MUTE_ZAP_ACTIVE 		= 0x100,
			CC_BTN_MUTE_ZAP_INACTIVE 	= 0x200,
			CC_BTN_OKAY			= 0x400,
			CC_BTN_MUTE			= 0x800,
			CC_BTN_UP			= 0x1000,
			CC_BTN_DOWN			= 0x2000,
			CC_BTN_LEFT			= 0x4000,
			CC_BTN_RIGHT			= 0x8000,
			CC_BTN_FORWARD			= 0x10000,
			CC_BTN_BACKWARD			= 0x20000,
			CC_BTN_PAUSE			= 0x40000,
			CC_BTN_PLAY			= 0x80000,
			CC_BTN_RECORD_ACTIVE		= 0x100000,
			CC_BTN_RECORD_INACTIVE		= 0x200000,
			CC_BTN_RECORD_STOP		= 0x400000
		};

		///set offset between icons within context button object
		void setButtonsSpace(const int buttons_space){cch_buttons_space = buttons_space;}

		///init all items within header object
		void initCCItems();
		///returns the text object
		CComponentsText* getTextObject(){return cch_text_obj;}

		/**Member to modify background behavior of embeded title
		* @param[in]  mode
		* 	@li bool, default = true
		* @return
		*	void
		* @see
		* 	Parent member: CCTextScreen::enableTboxSaveScreen()
		* 	CTextBox::enableSaveScreen()
		* 	disableTboxSaveScreen()
		*/
		void enableTboxSaveScreen(bool mode)
		{
			cc_txt_save_screen = mode;
			if (cch_text_obj->getCTextBoxObject())
				cch_text_obj->getCTextBoxObject()->enableSaveScreen(cc_txt_save_screen);
		}

		///returns the clock object
		CComponentsFrmClock* getClockObject(){return cch_cl_obj;}

		///enable display of clock, parameter bool enable, const char* format, bool run
		void enableClock(bool enable = true, const char* format = "%H:%M", const char* sec_format_str = NULL, bool run = false);
		///disable clock, without parameter
		void disableClock();

		///paint header
		void paint(const bool &do_save_bg = CC_SAVE_SCREEN_YES);

		///hides item, arg: no_restore see hideCCItem()
		void hide(){disableClock(); CComponentsForm::hide();}
		///erase current screen without restore of background, it's similar to paintBackgroundBoxRel() from CFrameBuffer
		void kill(const fb_pixel_t& bg_color = COL_BACKGROUND_PLUS_0, const int& corner_radius = -1, const int& fblayer_type = ~CC_FBDATA_TYPES, bool disable_clock = true);

		///set color gradient on/off, returns true if gradient mode was changed
		bool enableColBodyGradient(const int& enable_mode, const fb_pixel_t& sec_color = 255 /*=COL_BACKGROUND*/, const int& direction = -1);

		/**Methode to set channel logo into header body via id and/or channel name
		* @param[in]  	channelId
		* 		@li required channel id as uint64_t
		* @param[in]  	channelIName
		* 		@li required channel name as std::string
		* @param[in]  	alignment
		* 		@li optional alingment parameter as cc_logo_alignment_t (enum)\n
		* 		Possible values are:\n
		* 		CC_LOGO_LEFT \n
		* 		CC_LOGO_CENTER \n
		* 		CC_LOGO_RIGHT (default)\n
		* @param[in]  	dy
		* 		@li optional logo height, default = -1 (auto)
		* @note 	In auto mode, logo use full height minus inner offset but not larger than original logo height.
		*/
		void setChannelLogo(	const uint64_t& channelId,
					const std::string& channelName,
					cc_logo_alignment_t alignment = DEFAULT_LOGO_ALIGN,
					const int& dy = -1)
					{cch_logo.Id = channelId; cch_logo.Name = channelName, cch_logo.Align = alignment, cch_logo.dy_max = dy; initCCItems();}
		/**Methode to get channel logo object for direct access to its properties and methodes
		* @return  	CComponentsChannelLogo*
		*/
		CComponentsChannelLogo* getChannelLogoObject(){return cch_logo_obj;}
};

#endif
