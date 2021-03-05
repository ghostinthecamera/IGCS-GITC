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

using System;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Simple message to wrap a payload in to be send to the other end of our named pipe. 
	/// </summary>
	/// <remarks>Will produce a byte stream with the format: messageType | id | payload.
	/// Messages are simple as the scope of the messages are pretty simple: the message type specifies what type of message it is, the id specifies
	/// the specific message, and the payload then is the data for the message.</remarks> 
	public class IGCSMessage
	{
		#region Members
		private byte _messageType, _id;
		private byte[] _payload;
		#endregion

		/// <summary>
		/// CTor
		/// </summary>
		/// <param name="messageType">The type of the message to send</param>
		/// <param name="id">the id of the message</param>
		/// <param name="payload">The data for the message to send</param>
		public IGCSMessage(byte messageType, byte id, byte[] payload)
		{
			_id = id;
			_messageType = messageType;
			_payload = payload;
		}


		public byte[] GetPayloadAsByteArray()
		{
			var messageBytes = _payload==null || _payload.Length==0 ? new byte[2] : new byte[_payload.Length + 2];
			messageBytes[0] = _messageType;
			messageBytes[1] = _id;
			if(_payload?.Length > 0)
			{
				Array.Copy(_payload, 0, messageBytes, 2, _payload.Length);
			}
			return messageBytes;
		}
	}
}
