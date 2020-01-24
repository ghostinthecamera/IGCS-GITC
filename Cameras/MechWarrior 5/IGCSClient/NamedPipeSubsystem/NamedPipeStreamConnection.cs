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
using System.IO.Pipes;
using IGCSClient.Classes;

namespace IGCSClient.NamedPipeSubSystem
{
	/// <summary>
	/// Connection implementation around a named pipe.
	/// </summary>
	/// <remarks>Roughly based on Peter Bromberg's AsyncPipes library.</remarks>
	public class NamedPipeStreamConnection : NamedPipeStreamBase
	{
		#region Class Member Declarations
		private readonly object _instanceLock;
		private PipeStream _stream;
		/// <summary>
		/// Raised when the client disconnects from this stream
		/// </summary>
		public event EventHandler Disconnected;
		#endregion

		/// <summary>
		/// Initializes a new instance of the <see cref="NamedPipeStreamConnection"/> class.
		/// </summary>
		/// <param name="stream">The stream.</param>
		/// <param name="pipeName">The name of the named pipe used</param>
		public NamedPipeStreamConnection(PipeStream stream, string pipeName) : base(pipeName)
		{
			_instanceLock = new object();
			_stream = stream;
			byte[] buffer = new byte[ConstantsEnums.BufferLength];
			_stream.BeginRead(buffer, 0, ConstantsEnums.BufferLength, new AsyncCallback(this.EndRead), buffer);
		}


		/// <summary>
		/// Releases unmanaged resources and performs other cleanup operations before the
		/// <see cref="NamedPipeStreamConnection"/> is reclaimed by garbage collection.
		/// </summary>
		~NamedPipeStreamConnection()
		{
			this.Dispose(false);
		}


		/// <inheritdoc/>
		public override void Disconnect()
		{
			lock(_instanceLock)
			{
				base.Disconnect();
				_stream.Close();
			}
		}


		/// <summary>
		/// The callback of the read async method.
		/// </summary>
		/// <param name="result">The result.</param>
		private void EndRead(IAsyncResult result)
		{
			int length = _stream.EndRead(result);
			// asyncState is the buffer used in the ctor call which does the first read.
			byte[] asyncState = (byte[])result.AsyncState;
			if(length > 0)
			{
				byte[] destinationArray = new byte[length];
				Array.Copy(asyncState, 0, destinationArray, 0, length);
				OnMessageReceived(new ContainerEventArgs<byte[]>(destinationArray));
			}

			lock(_instanceLock)
			{
				if(_stream.IsConnected)
				{
					_stream.BeginRead(asyncState, 0, ConstantsEnums.BufferLength, new AsyncCallback(EndRead), asyncState);
				}
				else
				{
					this.Disconnected.RaiseEvent(this);
				}
			}
		}


		#region Class Property Declarations
		/// <summary>
		/// Gets a value indicating whether this instance is connected.
		/// </summary>
		public bool IsConnected
		{
			get { return _stream.IsConnected; }
		}
		#endregion
	}
}
