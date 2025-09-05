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
using System.Text;
using System.Windows;
using System.Windows.Forms;
using IGCSClient.NamedPipeSubSystem;
using IGCSClient.Forms;
using IGCSClient.Helpers;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Singleton which handles all messages either received or to be send. 
	/// </summary>
	public static class MessageHandlerSingleton
	{
		private static MessageHandler _instance = new MessageHandler();

		/// <summary>Dummy static constructor to make sure threadsafe initialization is performed.</summary>
		static MessageHandlerSingleton() {}

		/// <summary>
		/// Gets the single instance in use in this application
		/// </summary>
		/// <returns></returns>
		public static MessageHandler Instance() => _instance;

	}


	public class MessageHandler
	{
		#region Members
		private NamedPipeServer _pipeServer;
		private NamedPipeClient _pipeClient;
		#endregion

		internal MessageHandler()
		{
			_pipeServer = new NamedPipeServer(ConstantsEnums.DllToClientNamedPipeName);			// for connection from dll to this client. We create and own the pipe
			_pipeClient = new NamedPipeClient(ConstantsEnums.ClientToDllNamedPipeName);			// for connection from this client to dll. Dll creates and owns the pipe
			_pipeServer.MessageReceived += _pipeServer_MessageReceived;
			_pipeServer.ClientConnectionEstablished += _pipeServer_ClientConnectionEstablished;
			_pipeClient.ConnectedToPipe += _pipeClient_ConnectedToPipe;
			LogHandlerSingleton.Instance().LogLine("Named pipe enabled.", "System", true);
		}


		/// <summary>
		/// Sends a setting message. Setting messages have as format: MessageType.Setting | ID | payload
		/// </summary>
		/// <param name="id"></param>
		/// <param name="payload"></param>
		public void SendSettingMessage(byte id, byte[] payload)
		{
			_pipeClient.Send(new IGCSMessage(MessageType.Setting, id, payload));
		}


		/// <summary>
		/// Sends a keybinding message. Setting messages have as format: MessageType.KeyBinding | ID | payload
		/// </summary>
		/// <param name="id"></param>
		/// <param name="payload"></param>
		public void SendKeyBindingMessage(byte id, byte[] payload)
		{
			_pipeClient.Send(new IGCSMessage(MessageType.KeyBinding, id, payload));
		}
		

		/// <summary>
		/// Sends a 2-byte message to signal the dll that it should re-hook the xinput.
		/// </summary>
		public void SendRehookXInputAction()
		{
			// send a message of 2 bytes, first byte is 'Action', second byte, the id, is the action type, RehookXInput. No payload required. 
			_pipeClient.Send(new IGCSMessage(MessageType.Action, ActionType.RehookXInput, null));
		}

		//To handle custom resolution if necessary
        public void SendResolution(byte[] wpayload, byte[] hpayload)
        {
            // send a message of 2 bytes, first byte is 'Action', second byte, the id, is the action type, RehookXInput. No payload required. 
            _pipeClient.Send(new IGCSMessage(MessageType.Action, ActionType.setWidth, wpayload));
            _pipeClient.Send(new IGCSMessage(MessageType.Action, ActionType.setheight, hpayload));
            _pipeClient.Send(new IGCSMessage(MessageType.Action, ActionType.setResolution, null));
        }

        //To handle custom resolution if necessary
        public void SendPathAction(byte actionId, byte[] payload)
		{
			//LogHandlerSingleton.Instance().LogLine("Sending path Action","CLIENT::MessageHandler");
			_pipeClient.Send(new IGCSMessage(MessageType.CameraPathData, actionId, payload));
		}

        private void HandleNamedPipeMessageReceived(ContainerEventArgs<byte[]> e)
		{
			if(e.Value.Length < 2)
			{
				// not usable
				return;
			}

			// data starts at offset 1 or later, as the first byte is the message type.
			var asciiEncoding = new ASCIIEncoding();
            var utf8Encoding = new UTF8Encoding();
            var pathControllerWindow = MainWindow.CurrentPathControllerWindow;
            switch (e.Value[0])
			{
				case MessageType.Action:
					break;
				case MessageType.Notification:
					var notificationText = asciiEncoding.GetString(e.Value, 1, e.Value.Length - 1);
					LogHandlerSingleton.Instance().LogLine(notificationText, string.Empty);
					this.NotificationLogFunc?.Invoke(notificationText);
					break;
                case MessageType.NotificationOnly:
                    var notificationTextOnly = asciiEncoding.GetString(e.Value, 1, e.Value.Length - 1);
                    this.NotificationLogFunc?.Invoke(notificationTextOnly);
                    break;
                case MessageType.NormalTextMessage:
					LogHandlerSingleton.Instance().LogLine(asciiEncoding.GetString(e.Value, 1, e.Value.Length - 1), string.Empty);
					break;
				case MessageType.DebugTextMessage:
					LogHandlerSingleton.Instance().LogLine(asciiEncoding.GetString(e.Value, 1, e.Value.Length - 1), "Camera dll", true);
					break;
				case MessageType.ErrorTextMessage:
					LogHandlerSingleton.Instance().LogLine(asciiEncoding.GetString(e.Value, 1, e.Value.Length - 1), "Camera dll", false, true);
					break;
				case MessageType.CameraPathData:
					break;
				case MessageType.UpdatePathPlaying:
                    pathControllerWindow?.PathControllerInstance.UpdatePathPlayingFlag(e.Value[1]);
                    break;
                case MessageType.CameraEnabled:
                    pathControllerWindow?.PathControllerInstance.UpdateCameraEnabledFlag(e.Value[1]);
                    break;
                case MessageType.CameraPathBinaryData:
					BinaryCameraPathProcessor.HandleBinaryPathData(e.Value);
                    break;
                case MessageType.UpdateVisualisation:
                    pathControllerWindow?.PathControllerInstance.UpdateVisualisationFlag(e.Value[1]);
                    break;
                case MessageType.UpdateSelectedPath:
                    pathControllerWindow?.PathControllerInstance.setIncomingPath(asciiEncoding.GetString(e.Value, 1, e.Value.Length - 1));
                    break;
                case MessageType.CyclePath:
                    pathControllerWindow?.PathControllerInstance.SelectNextPath();
                    break;
                case MessageType.PathProgress:
                    pathControllerWindow?.PathControllerInstance.UpdatePathProgress(GeneralUtils.checkFloat(e.Value, 0.0f, 1.0f));
                    break;
            }
		}

		
		private void _pipeClient_ConnectedToPipe(object sender, EventArgs e)
		{
			this.ConnectedToNamedPipeFunc?.Invoke();
		}


		private void _pipeServer_ClientConnectionEstablished(object sender, EventArgs e)
		{
			this.ClientConnectionReceivedFunc?.Invoke();
		}


		private void _pipeServer_MessageReceived(object sender, ContainerEventArgs<byte[]> e)
		{
			HandleNamedPipeMessageReceived(e);
		}


		#region Properties
		/// <summary>
		/// Func which is called when a connection from the dll to our named pipe has been established
		/// </summary>
		public Action ClientConnectionReceivedFunc { get; set; }
		/// <summary>
		/// Func which is called when a connection to the dll's named pipe has been established
		/// </summary>
		public Action ConnectedToNamedPipeFunc { get; set; }
		public Action<string> NotificationLogFunc { get; set; }
		#endregion
	}
}
