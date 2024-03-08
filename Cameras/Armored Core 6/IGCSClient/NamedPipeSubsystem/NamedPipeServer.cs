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
using System.Collections.Generic;
using System.IO.Pipes;
using System.Security.AccessControl;
using System.Security.Principal;
using IGCSClient.Classes;

namespace IGCSClient.NamedPipeSubSystem
{
	/// <summary>
	/// Class which represents the server side of a named pipe.
	/// </summary>
	/// <remarks>Roughly based on Peter Bromberg's AsyncPipes library.</remarks>
	public class NamedPipeServer : NamedPipeStreamBase
	{
		#region Class Member Declarations
		private List<NamedPipeStreamConnection> _connections;
		private object _connectionsSemaphore;
		private PipeSecurity _pipeSecurity;
		private NamedPipeServerStream _connector;

		public event EventHandler ClientConnectionEstablished;
		#endregion


		/// <summary>
		/// Initializes a new instance of the <see cref="NamedPipeServer"/> class.
		/// </summary>
		/// <param name="pipeName">The name of the named pipe used</param>
		public NamedPipeServer(string pipeName) : base(pipeName)
		{
			_connectionsSemaphore = new object();

			_connections = new List<NamedPipeStreamConnection>();
			// set up security so everyone can write to the pipe.
			using(NamedPipeServerStream seedPipe = new NamedPipeServerStream("seedPipe", PipeDirection.In, 1, PipeTransmissionMode.Message, PipeOptions.None, 1000, 1000))
			{
				// just get a copy of the security roles and close this pipe.
				_pipeSecurity = seedPipe.GetAccessControl();
			}
			var sid = new SecurityIdentifier(WellKnownSidType.WorldSid, null);
			_pipeSecurity.AddAccessRule(new PipeAccessRule(sid, PipeAccessRights.ReadWrite, AccessControlType.Allow));
			// close it for network access.
			sid = new SecurityIdentifier(WellKnownSidType.NetworkSid, null);
			_pipeSecurity.AddAccessRule(new PipeAccessRule(sid, PipeAccessRights.ReadWrite, AccessControlType.Deny));

			_connector = CreateNamedPipeServerStream();
			_connector.BeginWaitForConnection(new AsyncCallback(this.ClientConnected), _connector);
		}


		/// <summary>
		/// Releases unmanaged resources and performs other cleanup operations before the
		/// <see cref="NamedPipeServer"/> is reclaimed by garbage collection.
		/// </summary>
		~NamedPipeServer()
		{
			Dispose(false);
		}

		
		/// </inheritdoc>
		public override void Disconnect()
		{
			lock(_connectionsSemaphore)
			{
				foreach(var connection in _connections)
				{
					try
					{
						if(connection.IsConnected)
						{
							connection.Disconnect();
						}
					}
					catch
					{
						// nothing we can do about it anyway.
					}
				}
				_connections.Clear();
			}
		}


		/// <summary>
		/// Callback method called when a pipe client is connected. 
		/// </summary>
		/// <param name="result"></param>
		private void ClientConnected(IAsyncResult result)
		{
			NamedPipeServerStream asyncState = result.AsyncState as NamedPipeServerStream;
			if(asyncState == null)
			{
				return;
			}
			asyncState.EndWaitForConnection(result);
			if(asyncState.IsConnected)
			{
				NamedPipeStreamConnection clientConnection = new NamedPipeStreamConnection(asyncState, this.PipeName);
				clientConnection.MessageReceived += Connection_MessageReceived;
				clientConnection.Disconnected += Connection_Disconnected;

				lock(_connectionsSemaphore)
				{
					_connections.Add(clientConnection);
				}
				this.ClientConnectionEstablished.RaiseEvent(this);
			}
		}


		/// <summary>
		/// Creates the named pipe server stream.
		/// </summary>
		/// <returns></returns>
		private NamedPipeServerStream CreateNamedPipeServerStream()
		{
			return new NamedPipeServerStream(this.PipeName, PipeDirection.In, 1, PipeTransmissionMode.Message, PipeOptions.Asynchronous, ConstantsEnums.BufferLength, 0, _pipeSecurity);
		}


		/// <summary>
		/// Handles the MessageReceived event of the Connection object.
		/// </summary>
		/// <param name="sender">The source of the event.</param>
		/// <param name="args">The <see cref="SD.Tools.Algorithmia.GeneralDataStructures.EventArguments.ContainerEventArgs&lt;System.Byte[]&gt;"/> instance containing the event data.</param>
		private void Connection_MessageReceived(object sender, ContainerEventArgs<byte[]> args)
		{
			this.OnMessageReceived(args);
		}


		/// <summary>
		/// Handles the disconnect event of the client connection object
		/// </summary>
		/// <param name="sender">The source of the event.</param>
		/// <param name="args">The <see cref="SD.Tools.Algorithmia.GeneralDataStructures.EventArguments.ContainerEventArgs&lt;System.Byte[]&gt;"/> instance containing the event data.</param>
		private void Connection_Disconnected(object sender, EventArgs args)
		{
			lock(_connectionsSemaphore)
			{
				var connection = sender as NamedPipeStreamConnection;
				if(connection != null)
				{
					_connections.Remove(connection);
				}
			}

			// wait for a new client to connect
			_connector.Dispose();
			_connector = CreateNamedPipeServerStream();
			_connector.BeginWaitForConnection(new AsyncCallback(this.ClientConnected), _connector);
		}
	}
}
