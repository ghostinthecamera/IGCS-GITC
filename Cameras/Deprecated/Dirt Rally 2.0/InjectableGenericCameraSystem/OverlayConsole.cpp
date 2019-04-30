#include "stdafx.h"
#include "OverlayConsole.h"
#include "imgui.h"
#include "Utils.h"

using namespace std;

namespace IGCS
{
	#define CONSOLE_DEBUG_PREFIX "[DEBUG]"
	#define CONSOLE_ERROR_PREFIX ">> ERROR <<"


	void OverlayConsole::clear()
	{
		EnterCriticalSection(&_contentCriticalSection);
			_buf.clear();
			_lineOffsets.clear();
		LeaveCriticalSection(&_contentCriticalSection);
	}


	void OverlayConsole::logDebug(const char* fmt, ...) IM_FMTARGS(2)
	{
#ifdef _DEBUG
		va_list args;
		va_start(args, fmt);
		string formattedArgs = Utils::formatString(fmt, args);
		va_end(args);

		logLine("%s %s", CONSOLE_DEBUG_PREFIX, formattedArgs.c_str());
#endif
	}


	void OverlayConsole::logError(const char* fmt, ...) IM_FMTARGS(2)
	{
		va_list args;
		va_start(args, fmt);
		string formattedArgs = Utils::formatString(fmt, args);
		va_end(args);

		logLine("%s %s", CONSOLE_ERROR_PREFIX, formattedArgs.c_str());
	}


	void OverlayConsole::logLine(const char* fmt, ...) IM_FMTARGS(2)
	{
		va_list args;
		va_start(args, fmt);
		string format(fmt);
		format += '\n';
		logLinev(format.c_str(), args);
		va_end(args);
	}


	void OverlayConsole::logLinev(const char* fmt, va_list args)
	{
		EnterCriticalSection(&_contentCriticalSection);
			int old_size = _buf.size();
			va_list args_copy;
			va_copy(args_copy, args);
			_buf.appendv(fmt, args_copy);

			for (int new_size = _buf.size(); old_size < new_size; old_size++)
			{
				if (_buf[old_size] == '\n')
				{
					_lineOffsets.push_back(old_size);
				}
			}
			_scrollToBottom = true;
		LeaveCriticalSection(&_contentCriticalSection);
	}


	void OverlayConsole::draw()
	{
		if (ImGui::Button("Clear"))
		{
			clear();
		}
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		_filter.Draw("Filter", -100.0f);
		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		EnterCriticalSection(&_contentCriticalSection);
			if (copy)
			{
				ImGui::LogToClipboard();
			}
			const char* buf_begin = _buf.begin();
			const char* line = buf_begin;
			for (int line_no = 0; line != NULL; line_no++)
			{
				const char* line_end = (line_no < _lineOffsets.Size) ? buf_begin + _lineOffsets[line_no] : NULL;
				if (_filter.PassFilter(line, line_end))
				{
					ImVec4 col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); 
					if (Utils::stringStartsWith(line, CONSOLE_ERROR_PREFIX))
					{
						col = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
					}
					else if (Utils::stringStartsWith(line, CONSOLE_DEBUG_PREFIX))
					{
						col = ImVec4(1.0f, 1.0f, 1.0f, 0.6f);
					}
					ImGui::PushStyleColor(ImGuiCol_Text, col);
					ImGui::TextUnformatted(line, line_end);
					ImGui::PopStyleColor();
				}
				line = line_end && line_end[1] ? line_end + 1 : NULL;
			}
			if (copy)
			{
				ImGui::LogFinish();
			}
		LeaveCriticalSection(&_contentCriticalSection);
		if (_scrollToBottom)
		{
			ImGui::SetScrollHere(1.0f);
		}
		_scrollToBottom = false;
		ImGui::EndChild();
	}
}
