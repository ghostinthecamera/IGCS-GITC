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
using System.Threading;
using IGCSClient.Classes;

namespace IGCSClient.NamedPipeSubSystem
{
	/// <summary>
	/// Class which represents a named pipe client. It is able to send messages to the named pipe defined for this application
	/// </summary>
	public class NamedPipeClient : IDisposable
	{
		#region Class Member Declarations
		private readonly object _queueLock, _streamLock;
		private PipeStream _stream;
		private bool _isDisposed, _messageSendingEnabled;
		private volatile bool _isFlushing;		// always modified within sections locking on _streamLock
		private Queue<IGCSMessage> _messageQueue;
		private string _pipeName;
		private AutoResetEvent _flushSemaphore;
		private int _messagesFlushedCounter; // always modified / examined within sections locking on _streamLock.

		public event EventHandler ConnectedToPipe;
		#endregion

		#region Constants
		/// <summary>
		/// A constant which is used to decide when messages should be dropped by the send thread in case no connection is available.
		/// See <see cref="SendMessageToPipeThreadBody"/> for details. Every occurrence is 100 ms.
		/// </summary>
		private const int MaxSendOccurrencesBeforeMessageDrop = 50; // 50 means 5000ms at least elapses before messages are dropped due to no connected pipe.
		/// <summary>
		/// A constant which is used by the connect thread to increase the delay between attempts to limit CPU usage caused by the .NET framework's named pipe
		/// connect method which uses a tight loop.
		/// </summary>
		private const int MaxNoConnectionLoopIterationsBeforeDelayIncrease = 10; // 10 means 10 seconds till wait drops to twice the delay.
		#endregion


		/// <summary>
		/// Initializes a new instance of the <see cref="NamedPipeClient"/> class.
		/// </summary>
		public NamedPipeClient(string pipeName) : base()
		{
			_pipeName = pipeName;
			_messageSendingEnabled = true;
			_messageQueue = new Queue<IGCSMessage>(1024);
			_streamLock = new object();
			_queueLock = new object();
			_flushSemaphore = new AutoResetEvent(false);
			StartConnectThread();
			StartSendMessagesThread();
		}


		/// <summary>
		/// Releases unmanaged resources and performs other cleanup operations before the
		/// <see cref="NamedPipeClient"/> is reclaimed by garbage collection.
		/// </summary>
		~NamedPipeClient()
		{
			Dispose(false);
		}


		/// <summary>
		/// Sends the specified message over the channel.
		/// </summary>
		/// <param name="message">The message.</param>
		public void Send(IGCSMessage message)
		{
			if(message == null)
			{
				return;
			}
			lock(_queueLock)
			{
				_messageQueue.Enqueue(message);
			}
		}


		/// <summary>
		/// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
		/// </summary>
		public void Dispose()
		{
			Dispose(true);
		}


		/// <summary>
		/// Waits before all messages currently in the queue for this channel for sending are indeed sent, before it returns. It waits for 3 seconds maximum.
		/// </summary>
		public void Flush()
		{
			lock(_streamLock)
			{
				if(_isFlushing)
				{
					// already flushing, nothing to do
					return;
				}
				_isFlushing = true;
				Interlocked.Exchange(ref _messagesFlushedCounter, 0);
				lock(_queueLock)
				{
					if(_messageQueue.Count <= 0)
					{
						// nothing new in the queue to send, we can assume everything is flushed.
						return;
					}
				}
			}
			// wait for the semaphore to be receive its signal set, or 3000 ms, whichever comes first. 
			_flushSemaphore.WaitOne(3000);
		}


		/// <summary>
		/// Signals the channel that message sending was disabled.
		/// </summary>
		public void DisableMessageSending()
		{
			_messageSendingEnabled = false;
		}


		/// <summary>
		/// Signals the channel that message sending was enabled.
		/// </summary>
		public void EnableMessageSending()
		{
			_messageSendingEnabled = true;
		}
		

		/// <summary>
		/// Disconnects from the named pipe
		/// </summary>
		private void Disconnect()
		{
			lock(_streamLock)
			{
				if((_stream!=null) && (_stream.IsConnected))
				{
					_stream.Close();
				}
			}
		}


		/// <summary>
		/// Async callback for when this client was done sending a message.
		/// </summary>
		/// <param name="result">The result.</param>
		private void EndSendMessage(IAsyncResult result)
		{
			try
			{
				lock(_streamLock)
				{
					_stream.EndWrite(result);
					_stream.Flush();
					bool flushStateFlag = (bool)result.AsyncState;
					if(flushStateFlag)
					{
						int newValue = Interlocked.Decrement(ref _messagesFlushedCounter);
						if(newValue <= 0)
						{
							// setting the semaphore will make the WaitOne in Flush() continue.
							_flushSemaphore.Set();
							_isFlushing = false;
						}
					}
				}
			}
			catch
			{
				this.Disconnect();
			}
		}
		

		/// <summary>
		/// Releases unmanaged and - optionally - managed resources
		/// </summary>
		/// <param name="disposing"><c>true</c> to release both managed and unmanaged resources; <c>false</c> to release only unmanaged resources.</param>
		private void Dispose(bool disposing)
		{
			if(disposing)
			{
				GC.SuppressFinalize(this);
			}
			if(!_isDisposed)
			{
				Disconnect();
				if(_stream != null)
				{
					_stream.Dispose();
				}
				_isDisposed = true;

			}
		}


		/// <summary>
		/// Starts the connect thread.
		/// </summary>
		private void StartConnectThread()
		{
			Thread thread = new Thread(new ThreadStart(this.ConnectToPipeThreadBody));
			thread.Name = "ConnectToPipeThread";
			thread.IsBackground = true;
			thread.Start();
		}


		/// <summary>
		/// Starts the send messages thread.
		/// </summary>
		private void StartSendMessagesThread()
		{
			Thread thread = new Thread(new ThreadStart(this.SendMessageToPipeThreadBody));
			thread.Name = "SendMessagesToPipeThread";
			thread.IsBackground = true;
			thread.Start();
		}


		/// <summary>
		/// Sends the message to pipe thread body. This thread body sends all messages in the queue, and otherwise sleeps for a given interval.
		/// </summary>
		private void SendMessageToPipeThreadBody()
		{
			int occurenceCounter = 0;
			while(true)
			{
				lock(_streamLock)
				{
					occurenceCounter++;
					List<byte[]> messagesToSend = null;
					if(occurenceCounter > MaxSendOccurrencesBeforeMessageDrop)
					{
						// obtain messages now, as it has taken MaxSendOccurrencesBeforeMessageDrop occurrences and if no messages were send
						// because of no listener/connected name pipe, we have to dequeue the messages to avoid memory problems. 
						messagesToSend = GetMessagesToSend();
					}
					if(_messageSendingEnabled && ((_stream != null) && (_stream.IsConnected)))
					{
						if(messagesToSend == null)
						{
							messagesToSend = GetMessagesToSend();
						}
						object state = _isFlushing;
						foreach(var message in messagesToSend)
						{
							if(message == null)
							{
								continue;
							}
							try
							{
								this._stream.BeginWrite(message, 0, message.Length, new AsyncCallback(this.EndSendMessage), state);
								//this._stream.Flush();
								if(_isFlushing)
								{
									Interlocked.Increment(ref _messagesFlushedCounter);
								}
							}
							catch
							{
								// stream operation caused an exception. We can't deal with the exception here, nor can we bubble it upwards as nothing higher up 
								// the call chain can handle it. -> swallow
								break;
							}
						}
					}
				}
				Thread.Sleep(100);
			}
		}


		/// <summary>
		/// Connects to pipe thread body. This thread body tries to connect to a named pipe, if not connected. It tries to reconnect if it detects
		/// a disconnect.
		/// </summary>
		private void ConnectToPipeThreadBody()
		{
			string serverName = ".";
			string pipeName = _pipeName;
			if(pipeName.Contains("\\"))
			{
				serverName = pipeName.Substring(pipeName.IndexOf("\\") + 1, pipeName.LastIndexOf("\\") - 1);
				pipeName = pipeName.Substring(pipeName.LastIndexOf("\\") + 1);
			}
			int iterationCounter = 0;
			// initially set to 1000, will increase to 2000 if iteration counter increases MaxNoConnectionLoopIterationsBeforeDelayIncrease
			int loopDelayMS = 1000;
			bool connectEventRaised = false;
			while(true)
			{
				lock(_streamLock)
				{
					if(_messageSendingEnabled)
					{
						if(_stream == null)
						{
							_stream = new NamedPipeClientStream(serverName, pipeName, PipeDirection.Out, PipeOptions.Asynchronous);
						}
						if(_stream.IsConnected)
						{
							if(!connectEventRaised)
							{
								this.ConnectedToPipe.RaiseEvent(this);
								connectEventRaised = true;
							}
						}
						else
						{
							connectEventRaised = false;
							try
							{
								// Connect is a tight loop over the unsafe Connect method call. it always makes at least 1 call, so the timeout
								// is really for defining a period in which the unsafe connect method is called over and over again. 1ms is therefore
								// enough as we also use a loop. Setting this to a higher number will lead to 100% cpu usage within that period of time.
								((NamedPipeClientStream)_stream).Connect(1);
							}
							catch(TimeoutException)
							{
								// no pipe open at this point.
							}
							catch
							{
								// stream operation caused an exception. We can't deal with the exception here, nor can we bubble it upwards as nothing higher up 
								// the call chain can handle it. -> swallow
							}
						}
					}
				}
				iterationCounter++;
				if(iterationCounter > MaxNoConnectionLoopIterationsBeforeDelayIncrease)
				{
					loopDelayMS = 2000;	// 2000ms, to increase delay to limit cpu usage.
				}
				Thread.Sleep(loopDelayMS);
			}
		}

		
		/// <summary>
		/// Gets the messages to send. It simply dequeues all messages in the queue and effectively clears the queue. 
		/// </summary>
		/// <returns></returns>
		private List<byte[]> GetMessagesToSend()
		{
			List<byte[]> toReturn = new List<byte[]>();
			Queue<IGCSMessage> queuedMessages = null;
			lock(_queueLock)
			{
				queuedMessages = _messageQueue;
				_messageQueue = new Queue<IGCSMessage>(1024);
			}
			if(queuedMessages != null)
			{
				foreach(var message in queuedMessages)
				{
					toReturn.Add(message.GetPayloadAsByteArray());
				}
			}
			return toReturn;
		}
	}
}
