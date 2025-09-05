////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020, Frans Bouma
// All rights reserved.
// https://github.com/FransBouma/InjectableGenericCameraSystem
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////

using IGCSClient.Controls;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Singleton class to channel text messages from every part of the app to the central application log output
	/// </summary>
	public static class LogHandlerSingleton
	{
		private static LogHandler _instance = new LogHandler();
		
		/// <summary>Dummy static constructor to make sure threadsafe initialization is performed.</summary>
		static LogHandlerSingleton() { }

		/// <summary>
		/// Gets the single instance in use in this application
		/// </summary>
		/// <returns></returns>
		public static LogHandler Instance() => _instance;
	}


	/// <summary>
	/// The actual implementation
	/// </summary>
	public sealed class LogHandler
	{
		#region Members
		private ApplicationOutputPage _outputControl;
		#endregion

		internal LogHandler() { }


		/// <summary>
		/// Sets the output control to log the messages on we're receiving.
		/// </summary>
		/// <param name="outputControl"></param>
		public void Setup(ApplicationOutputPage outputControl)
		{
			ArgumentVerifier.CantBeNull(outputControl, nameof(outputControl));
			_outputControl = outputControl;
		}


		/// <summary>
		/// Logs the line in LineToLog in the output window, based on the verbose setting isVerboseMessage, which means that the line is not logged
		/// when the verbose checkbox is disabled if isVerboseMessage is set to true.
		/// </summary>
		/// <param name="lineToLog">Line to log which can contain format characters</param>
		/// <param name="source">Source description of the line</param>
		/// <param name="args">The args to pass to the string formatter.</param>
		public void LogLine(string lineToLog, string source, params object[] args)
		{
			LogLine(lineToLog, source, false, false, args);
		}
		

		/// <summary>
		/// Logs the given line to the output window. Based on the verbose checkbox and the VerboseMessage flag the message is logged or not.
		/// If the verbose checkbox is set, also lines with VerboseMessage=true will be logged, otherwise these messages will be suppressed.
		/// </summary>
		/// <param name="lineToLog">Line to log which can contain format characters</param>
		/// <param name="source">Source description of the line</param>
		/// <param name="isDebug">Flag to signal that the message is a debug message. Debug messages are only shown in debug builds.</param>
		/// <param name="args">The args to pass to the string formatter.</param>
		public void LogLine(string lineToLog, string source, bool isDebug, params object[] args)
		{
			LogLine(lineToLog, source, isDebug, false, args);
		}


		/// <summary>
		/// Logs the given line to the output window. Based on the verbose checkbox and the VerboseMessage flag the message is logged or not.
		/// If the verbose checkbox is set, also lines with VerboseMessage=true will be logged, otherwise these messages will be suppressed.
		/// </summary>
		/// <param name="lineToLog">Line to log which can contain format characters</param>
		/// <param name="source">Source description of the line</param>
		/// <param name="isDebug">Flag to signal that the message is a debug message. Debug messages are only shown in debug builds.</param>
		/// <param name="isError">if set to <c>true</c> [is error].</param>
		/// <param name="args">The args to pass to the string formatter.</param>
		public void LogLine(string lineToLog, string source, bool isDebug, bool isError, params object[] args)
		{
			if(_outputControl == null)
			{
				return;
			}
			if(_outputControl.CheckAccess())
			{
				_outputControl.LogLine(lineToLog, source, isDebug, isError, args);
			}
			else
			{
				_outputControl.Dispatcher?.Invoke(_outputControl.LogLineFunc, new object[] {lineToLog, source, isDebug, isError, args});
			}
		}

	}
}
