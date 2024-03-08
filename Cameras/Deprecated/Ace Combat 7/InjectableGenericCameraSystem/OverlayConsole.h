#pragma once

#include "stdafx.h"
#include "ImGui.h"

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

		void clear() { _buf.clear(); _lineOffsets.clear(); }
		void logDebug(const char* fmt, ...) IM_PRINTFARGS(2);
		void logError(const char* fmt, ...) IM_PRINTFARGS(2);
		void logLine(const char* fmt, ...)  IM_PRINTFARGS(2);
		void draw();

		OverlayConsole(OverlayConsole const&) = delete;			// see: https://stackoverflow.com/a/1008289/44991
		void operator=(OverlayConsole const&) = delete;

	private:
		OverlayConsole() {}
		void logLinev(const char* fmt, va_list args);

		ImGuiTextBuffer     _buf;
		ImGuiTextFilter     _filter;
		ImVector<int>       _lineOffsets;        // Index to lines offset
		bool                _scrollToBottom;
	};
}

