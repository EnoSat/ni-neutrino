/*
	lcd4l

	Copyright (C) 2012 'defans'
	Homepage: http://www.bluepeercrew.us/

	Copyright (C) 2012-2018 'vanhofen'
	Homepage: http://www.neutrino-images.de/

	Copyright (C) 2016-2019 'TangoCash'
	Copyright (C) 2021, Thilo Graf 'dbt'

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
	along with this program. If not, see <http://www.gnu.org/licenses/>.


*/

#ifndef __lcd4l__
#define __lcd4l__

#include <string>
#include <thread>
#include <sigc++/signal.h>

class CLCD4l
{
	public:
		CLCD4l();
		~CLCD4l();
		static CLCD4l *getInstance();

		// Displays
		enum
		{
			DPF320x240	= 0,
			SPF800x480	= 1,
			SPF800x600	= 2,
			SPF1024x600	= 3
		};

		// Functions
		void	InitLCD4l();
		void	StartLCD4l();
		void	StopLCD4l();
		void	SwitchLCD4l();
		void	ForceRun() { wait4daemon = false; }

		int	CreateFile(const char *file, std::string content = "", bool convert = false);
		int	RemoveFile(const char *file);

		int	CreateMenuFile(std::string content = "", bool convert = false);
		int	RemoveMenuFile();

		int	GetMaxBrightness();

		void	ResetParseID() { m_ParseID = 0; }

		// use signal/slot handlers
		// That is helping to keep the GUI code away from code inside ./src/driver.
		sigc::signal<void>	OnBeforeStart,
					OnAfterStart,
					OnBeforeStop,
					OnAfterStop,
					OnBeforeRestart,
					OnAfterRestart,
					OnError;

	private:
		std::thread	*thrLCD4l;
		static void	*LCD4lProc(void *arg);
		bool		exit_proc;

		struct tm	*tm_struct;
		bool		wait4daemon;

		// Functions
		void		Init();
		void		ParseInfo(uint64_t parseID, bool newID, bool firstRun = false);

		uint64_t	GetParseID();
		bool		CompareParseID(uint64_t &i_ParseID);

		std::string	hexStr(unsigned char data);
		std::string	hexStrA2A(unsigned char data);
		void		strReplace(std::string &orig, const std::string &fstr, const std::string &rstr);
		bool		WriteFile(const char *file, std::string content = "", bool convert = false);

		void		SetWaitStatus(bool wait) { wait4daemon = wait; }
		bool		GetWaitStatus() { return wait4daemon; }

		// Variables
		uint64_t	m_ParseID;
		int		m_Mode;
		int		m_ModeChannel;

		int		m_Brightness;
		int		m_Brightness_standby;
		std::string	m_Resolution;
		std::string	m_AspectRatio;
		int		m_Videotext;
		int		m_Radiotext;
		std::string	m_DolbyDigital;
		int		m_Tuner;
		int		m_Tuner_sig;
		int		m_Tuner_snr;
		int		m_Tuner_ber;
		int		m_Volume;
		int		m_ModeRec;
		int		m_RecordCount;
		int		m_ModeTshift;
		int		m_ModeTimer;
		int		m_ModeEcm;
		bool		m_ModeCamPresent;
		int		m_ModeCam;

		std::string	m_Service;
		int		m_ChannelNr;
		std::string	m_Logo;
		int		m_ModeLogo;

		std::string	m_Layout;

		std::string	m_Event;
		std::string	m_Info1;
		std::string	m_Info2;
		int		m_Progress;
		char		m_Duration[128];
		std::string	m_Start;
		std::string	m_End;

		std::string	m_font;
		std::string	m_fgcolor;
		std::string	m_bgcolor;
		std::string	m_fcolor1;
		std::string	m_fcolor2;
		std::string	m_pbcolor;

		std::string	m_wcity;
		std::string	m_wtimestamp;
		std::string	m_wtemp;
		std::string	m_wwind;
		std::string	m_wicon;
};

#endif
