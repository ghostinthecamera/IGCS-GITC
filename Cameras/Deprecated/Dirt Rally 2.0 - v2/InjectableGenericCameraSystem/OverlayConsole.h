#pragma once

#include "stdafx.h"
#include "imgui.h"

namespace IGCS
{

	class OverlayConsole
	{
	public:
		// singleton implementation. 
		static OverlayConsole& instance()
		{
			static OverlayConsole instance;
			return instance;
		}
		void clear();
		void logDebug(const char* fmt, ...) IM_FMTARGS(2);
		void logError(const char* fmt, ...) IM_FMTARGS(2);
		void logLine(const char* fmt, ...)  IM_FMTARGS(2);
		void draw();

		OverlayConsole(OverlayConsole const&) = delete;			// see: https://stackoverflow.com/a/1008289/44991
		void operator=(OverlayConsole const&) = delete;

	private:
		OverlayConsole() 
		{
			InitializeCriticalSectionAndSpinCount(&_contentCriticalSection, 0x400);
		}
		void logLinev(const char* fmt, va_list args);

		CRITICAL_SECTION	_contentCriticalSection;
		ImGuiTextBuffer     _buf;
		ImGuiTextFilter     _filter;
		ImVector<int>       _lineOffsets;        // Index to lines offset
		bool                _scrollToBottom;
	};
}

