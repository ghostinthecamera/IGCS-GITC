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

using System.Windows.Controls;

namespace IGCSClient.Interfaces
{
	/// <summary>
	/// Interface for a setting instance
	/// </summary>
	public interface ISetting
	{
		/// <summary>
		/// Sets up the setting with the control specified
		/// </summary>
		/// <param name="inputControl"></param>
		void Setup(Control inputControl);
		/// <summary>
		/// Method which sends the value of the setting as a message to the other side of the named pipe.
		/// </summary>
		void SendValueAsMessage();

		/// <summary>
		/// Gets the control associated with the setting in a generic form
		/// </summary>
		Control InputControl { get; }
		/// <summary>
		/// Gets the ID of the setting, which is the id used on both client and camera system
		/// </summary>
		byte ID { get; }
		/// <summary>
		/// Gets the name of the setting
		/// </summary>
		string Name { get; }
		/// <summary>
		/// Gets the value of the setting as string for persistence purposes
		/// </summary>
		/// <returns></returns>
		string GetValueAsString();
		/// <summary>
		/// Sets the value of the setting from the value as string specified. 
		/// </summary>
		/// <param name="valueAsString"></param>
		void SetValueFromString(string valueAsString);
	}
}