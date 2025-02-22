/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
	Boston, MA  02110-1301, USA.
*/

#ifndef __framebuffer__
#define __framebuffer__
#include <config.h>

#include <stdint.h>
#include <linux/fb.h>
#include <linux/vt.h>

#include <string>
#include <vector>
#include <map>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <sigc++/signal.h>
// #define fb_pixel_t uint32_t

typedef struct fb_var_screeninfo t_fb_var_screeninfo;

typedef struct osd_resolution_t
{
	uint32_t yRes;
	uint32_t xRes;
	uint32_t bpp;
	uint32_t mode;
} osd_resolution_struct_t;

typedef struct gradientData_t
{
	fb_pixel_t* gradientBuf;
	fb_pixel_t* boxBuf;
	bool direction;
	int mode;
	int x;
	int dx;
} gradientData_struct_t;

#define CORNER_NONE		0x0
#define CORNER_TOP_LEFT		0x1
#define CORNER_TOP_RIGHT	0x2
#define CORNER_TOP		0x3
#define CORNER_BOTTOM_RIGHT	0x4
#define CORNER_RIGHT		0x6
#define CORNER_BOTTOM_LEFT	0x8
#define CORNER_LEFT		0x9
#define CORNER_BOTTOM		0xC
#define CORNER_ALL		0xF

#define FADE_TIME 5000
#define FADE_STEP 5
#define FADE_RESET 0xFFFF

#define WINDOW_SIZE_MAX		100 // %
#define WINDOW_SIZE_MIN		50 // %
#define WINDOW_SIZE_SMALL	80 // %

#define FB_DEVICE		"/dev/fb0"

/** Ausfuehrung als Singleton */
class CFrameBuffer : public sigc::trackable
{
	protected:

		CFrameBuffer();
		OpenThreads::Mutex mutex;

		struct rgbData
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
		} __attribute__ ((packed));

		struct rawHeader
		{
			uint8_t width_lo;
			uint8_t width_hi;
			uint8_t height_lo;
			uint8_t height_hi;
			uint8_t transp;
		} __attribute__ ((packed));

		struct rawIcon
		{
			uint16_t width;
			uint16_t height;
			uint8_t transp;
			fb_pixel_t * data;
		};

		std::string     iconBasePath;

		int             fd, tty;
		fb_pixel_t *    lfb;
		fb_pixel_t *    lbb;
		int		available;
		fb_pixel_t *    background;
		fb_pixel_t *    backupBackground;
		fb_pixel_t      backgroundColor;
		std::string     backgroundFilename;
		bool            useBackgroundPaint;
		unsigned int	xRes, yRes, stride, swidth, bpp;
		t_fb_var_screeninfo screeninfo;
		fb_cmap cmap;
		__u16 red[256], green[256], blue[256], trans[256];

		void paletteFade(int i, __u32 rgb1, __u32 rgb2, int level);

		int 	kd_mode;
		struct	vt_mode vt_mode;
		bool	active;
		bool fb_no_check;
		static	void switch_signal (int);
		fb_fix_screeninfo fix;
		bool locked;
		std::map<std::string, rawIcon> icon_cache;
		int cache_size;

		int *q_circle;
		bool corner_tl, corner_tr, corner_bl, corner_br;

		void * int_convertRGB2FB(unsigned char *rgbbuff, unsigned long x, unsigned long y, int transp, bool alpha);
		int m_transparent_default, m_transparent;
		// Unlocked versions (no mutex)
		void paintHLineRelInternal(int x, int dx, int y, const fb_pixel_t col);
		void paintVLineRelInternal(int x, int y, int dy, const fb_pixel_t col);

		inline void paintHLineRelInternal2Buf(const int& x, const int& dx, const int& y, const int& box_dx, const fb_pixel_t& col, fb_pixel_t* buf);
		void paintShortHLineRelInternal(const int& x, const int& dx, const int& y, const fb_pixel_t& col);
		int  limitRadius(const int& dx, const int& dy, int& radius);
		void setCornerFlags(const int& type);
		void initQCircle();
		inline int calcCornersOffset(const int& dy, const int& line, const int& radius, const int& type) { int ofs = 0; calcCorners(&ofs, NULL, NULL, dy, line, radius, type); return ofs; }
		bool calcCorners(int *ofs, int *ofl, int *ofr, const int& dy, const int& line, const int& radius, const int& type);

	public:
		///gradient direction
		enum {
			gradientHorizontal,
			gradientVertical
		};

		enum {
			pbrg_noOption = 0x00,
			pbrg_noPaint  = 0x01,
			pbrg_noFree   = 0x02
		};

		fb_pixel_t realcolor[256];

		virtual ~CFrameBuffer();

		static CFrameBuffer* getInstance();

		virtual void init(const char * const fbDevice = FB_DEVICE);
		virtual int setMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);


		int getFileHandle() const; //only used for plugins (games) !!
		t_fb_var_screeninfo *getScreenInfo();

		fb_pixel_t * getFrameBufferPointer() const; // pointer to framebuffer
		virtual fb_pixel_t * getBackBufferPointer() const;  // pointer to backbuffer
		virtual unsigned int getStride() const;             // size of a single line in the framebuffer (in bytes)
		unsigned int getScreenWidth(const bool& real = false) const;
		unsigned int getScreenHeight(const bool& real = false) const;
		unsigned int getWindowWidth(bool force_small = false);
		unsigned int getWindowHeight(bool force_small = false);
		unsigned int getScreenX();
		unsigned int getScreenY();

		bool getActive() const;                     // is framebuffer active?
		void setActive(bool enable);                     // is framebuffer active?
		virtual void setupGXA() { return; };             // reinitialize stuff
		virtual void add_gxa_sync_marker() { return; };
		virtual bool needAlign4Blit() { return false; };
		virtual uint32_t getWidth4FB_HW_ACC(const uint32_t x, const uint32_t w, const bool max=true);

		void setTransparency( int tr = 0 );
		virtual void setBlendMode(uint8_t mode = 1);
		virtual void setBlendLevel(int level);

		//Palette stuff
		void setAlphaFade(int in, int num, int tr);
		void paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
		void paletteSetColor(int i, __u32 rgb, int tr);
		void paletteSet(struct fb_cmap *map = NULL);

		//paint functions
		inline void paintPixel(fb_pixel_t * const dest, const uint8_t color) const
			{
				*dest = realcolor[color];
			};
		virtual void paintPixel(int x, int y, const fb_pixel_t col);

		fb_pixel_t* paintBoxRel2Buf(const int dx, const int dy, const int w_align, const int offs_align, const fb_pixel_t col, fb_pixel_t* buf = NULL, int radius = 0, int type = CORNER_ALL);
		fb_pixel_t* paintBoxRel(const int x, const int y, const int dx, const int dy, const fb_pixel_t col, gradientData_t *gradientData, int radius = 0, int type = CORNER_ALL);

		virtual void paintBoxRel(const int x, const int y, const int dx, const int dy, const fb_pixel_t col, int radius = 0, int type = CORNER_ALL);
		inline void paintBox(int xa, int ya, int xb, int yb, const fb_pixel_t col) { paintBoxRel(xa, ya, xb - xa, yb - ya, col); }
		inline void paintBox(int xa, int ya, int xb, int yb, const fb_pixel_t col, int radius, int type) { paintBoxRel(xa, ya, xb - xa, yb - ya, col, radius, type); }

		void paintBoxFrame(const int x, const int y, const int dx, const int dy, const int px, const fb_pixel_t col, int radius = 0, int type = CORNER_ALL);
		virtual void paintLine(int xa, int ya, int xb, int yb, const fb_pixel_t col);

		inline void paintVLine(int x, int ya, int yb, const fb_pixel_t col) { paintVLineRel(x, ya, yb - ya, col); }
		virtual void paintVLineRel(int x, int y, int dy, const fb_pixel_t col);

		inline void paintHLine(int xa, int xb, int y, const fb_pixel_t col) { paintHLineRel(xa, xb - xa, y, col); }
		virtual void paintHLineRel(int x, int dx, int y, const fb_pixel_t col);

		void setIconBasePath(const std::string & iconPath);
		std::string getIconBasePath(){return iconBasePath;};
		std::string getIconPath(std::string icon_name, std::string file_type = "");

		void getIconSize(const char * const filename, int* width, int *height);
		/* h is the height of the target "window", if != 0 the icon gets centered in that window */
		bool paintIcon (const std::string & filename, const int x, const int y,
				const int h = 0, const unsigned char offset = 1, bool paint = true, bool paintBg = false, const fb_pixel_t colBg = 0);
		bool paintIcon8(const std::string & filename, const int x, const int y, const unsigned char offset = 0);
		void loadPal   (const std::string & filename, const unsigned char offset = 0, const unsigned char endidx = 255);
		void clearIconCache();

		bool loadPicture2Mem        (const std::string & filename, fb_pixel_t * const memp);
		bool loadPicture2FrameBuffer(const std::string & filename);
		bool loadPictureToMem       (const std::string & filename, const uint16_t width, const uint16_t height, const uint16_t stride, fb_pixel_t * const memp);
		bool savePictureFromMem     (const std::string & filename, const fb_pixel_t * const memp);

		int getBackgroundColor() { return backgroundColor;}
		void setBackgroundColor(const fb_pixel_t color);
		bool loadBackground(const std::string & filename, const unsigned char col = 0);
		void useBackground(bool);
		bool getuseBackground(void);

		void saveBackgroundImage(void);  // <- implies useBackground(false);
		void restoreBackgroundImage(void);

		void paintBackgroundBoxRel(int x, int y, int dx, int dy);
		inline void paintBackgroundBox(int xa, int ya, int xb, int yb) { paintBackgroundBoxRel(xa, ya, xb - xa, yb - ya); }

		void paintBackground();

		void SaveScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp);
		void RestoreScreen(const int& x, const int& y, const int& dx, const int& dy, fb_pixel_t * const memp);

		void Clear();

		enum
			{
				SHOW_FRAME_FALLBACK_MODE_OFF		= 0,
				SHOW_FRAME_FALLBACK_MODE_IMAGE		= 1,
				SHOW_FRAME_FALLBACK_MODE_IMAGE_UNSCALED	= 2,
				SHOW_FRAME_FALLBACK_MODE_BLACKSCREEN	= 4,
				SHOW_FRAME_FALLBACK_MODE_CALLBACK	= 8
			};
		bool showFrame(const std::string & filename, int fallback_mode = SHOW_FRAME_FALLBACK_MODE_OFF);
		void stopFrame();
		bool loadBackgroundPic(const std::string & filename, bool show = true);
		bool Lock(void);
		void Unlock(void);
		bool Locked(void) { return locked; };
		virtual void waitForIdle(const char* func=NULL);
		void* convertRGB2FB(unsigned char *rgbbuff, unsigned long x, unsigned long y, int transp = 0xFF);
		void* convertRGBA2FB(unsigned char *rgbbuff, unsigned long x, unsigned long y);
		void displayRGB(unsigned char *rgbbuff, int x_size, int y_size, int x_pan, int y_pan, int x_offs, int y_offs, bool clearfb = true, int transp = 0xFF);
		virtual void fbCopyArea(uint32_t width, uint32_t height, uint32_t dst_x, uint32_t dst_y, uint32_t src_x, uint32_t src_y);
		virtual void blit2FB(void *fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp = 0, uint32_t yp = 0, bool transp = false, uint32_t unscaled_w = 0, uint32_t unscaled_h = 0); //NI
		virtual void blitBox2FB(const fb_pixel_t* boxBuf, const uint32_t& width, const uint32_t& height, const uint32_t& xoff, const uint32_t& yoff);

		virtual void mark(int x, int y, int dx, int dy);
/* Remove this when pu/fb-setmode branch is merged to master */
#define SCALE2RES_DEFINED
		virtual int scale2Res(int size) { return size; };
		virtual bool fullHdAvailable() { return false; };
		virtual void setOsdResolutions();
		std::vector<osd_resolution_t> osd_resolutions;
		size_t getIndexOsdResolution(uint32_t mode);

		enum
			{
				TM_EMPTY  = 0,
				TM_NONE   = 1,
				TM_BLACK  = 2,
				TM_INI    = 3
			};
		void SetTransparent(int t){ m_transparent = t; }
		void SetTransparentDefault(){ m_transparent = m_transparent_default; }

// ## AudioMute / Clock ######################################
	private:
		enum {
			FB_PAINTAREA_MATCH_NO,
			FB_PAINTAREA_MATCH_OK
		};

		typedef struct fb_area_t
		{
			int x;
			int y;
			int dx;
			int dy;
			int element;
		} fb_area_struct_t;

		bool fbAreaActiv;
		typedef std::vector<fb_area_t> v_fbarea_t;
		typedef v_fbarea_t::iterator fbarea_iterator_t;
		v_fbarea_t v_fbarea;
		bool do_paint_mute_icon;

		bool _checkFbArea(const int& _x, const int& _y, const int& _dx, const int& _dy, const bool& prev);
		int checkFbAreaElement(const int& _x, const int& _y, const int& _dx, const int& _dy, const fb_area_t *area);

	public:
		enum {
			FB_PAINTAREA_INFOCLOCK,
			FB_PAINTAREA_MUTEICON1,
			FB_PAINTAREA_MUTEICON2,

			FB_PAINTAREA_MAX
		};

		inline bool checkFbArea(const int& _x, const int& _y, const int& _dx, const int& _dy, const bool& prev) { return (fbAreaActiv && !fb_no_check) ? _checkFbArea(_x, _y, _dx, _dy, prev) : true; }
		void setFbArea(int element, int _x=0, int _y=0, int _dx=0, int _dy=0);
		void fbNoCheck(bool noCheck) { fb_no_check = noCheck; }
		void doPaintMuteIcon(bool mode) { do_paint_mute_icon = mode; }
		void blit();
		sigc::signal<void> OnAfterSetPallette;
		sigc::signal<void> OnFallbackShowFrame;
		const char *fb_name;
};

#endif
