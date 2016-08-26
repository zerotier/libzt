/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

using UnityEngine;
using System.Collections;
using System.Threading;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using UnityEngine.UI;
using UnityEngine.Networking;

// Demonstrates the usage of bare-bones ZeroTier-administered sockets
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System;


public class ZeroTierSockets_Demo : MonoBehaviour
{
	public float speed = 300f;

	private ZTSDK zt;
	string nwid = "";

	int sock; // The "connection id"
	int host_socket;

	// Demo button methods
	public void Join()
	{
		GameObject go = GameObject.Find ("inputNetworkID"); 
		InputField input = go.GetComponents<InputField> () [0];
		Debug.Log ("Joining: " + input.text);
		zt.JoinNetwork (input.text);
	}

	public void Leave()
	{
		GameObject go = GameObject.Find ("inputNetworkID"); 
		InputField input = go.GetComponents<InputField> () [0];
		Debug.Log ("Leaving: " + input.text);
		zt.LeaveNetwork (input.text);
	}

	// 1. Create ZeroTier-socket
	// 2. Connect to remote host (on ZeroTier network) via socket
	public void Connect()
	{
		GameObject addr_go = GameObject.Find ("inputServerAddress"); 
		GameObject port_go = GameObject.Find ("inputServerPort"); 
		InputField addr = addr_go.GetComponents<InputField> () [0];
		InputField port = port_go.GetComponents<InputField> () [0];
		Debug.Log ("Connecting to: " + addr.text + ":" + port.text);

		Thread connectThread = new Thread(() => {
			int sockfd = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
			Debug.Log ("sockfd = " + sockfd);
			int port_num;
			int.TryParse(port.text,out port_num);
			zt.Connect (sockfd, addr.text,port_num);
			Debug.Log ("sock = " + sock);
		});
		connectThread.IsBackground = true;
		connectThread.Start();
	}

	public void Bind()
	{
		GameObject addr_go = GameObject.Find ("inputServerAddress"); 
		GameObject port_go = GameObject.Find ("inputServerPort"); 
		InputField addr = addr_go.GetComponents<InputField> () [0];
		InputField port = port_go.GetComponents<InputField> () [0];
		Debug.Log ("Binding to: " + addr.text + ":" + port.text);

		// Get protocol type from GUI
		GameObject go = GameObject.Find ("dropdownProtocol"); 
		Dropdown dd = go.GetComponents<Dropdown> () [0];
		Debug.Log("Protocol selected: " + dd.captionText.text);
		SocketType type = dd.captionText.text == "UDP" ? SocketType.Dgram : SocketType.Stream;

		Thread connectThread = new Thread(() => { 

			// TCP
			if(type == SocketType.Stream) {
				// Socket()
				sock = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Stream, (int)ProtocolType.Unspecified);
				Debug.Log ("sock = " + sock);

				// Bind()
				int port_num;
				int.TryParse(port.text,out port_num);
				int bind_res = zt.Bind(sock, "0.0.0.0", port_num);
				Debug.Log ("bind_res = " + bind_res);

				// Listen()
				int listen_res = zt.Listen(sock, 1);
				Debug.Log ("listen_res = " + listen_res);

				// Accept() loop
				Debug.Log("entering accept() loop");
				int accept_res = -1;
				while(accept_res < 0)
				{
					accept_res = zt.Accept(sock);
					Debug.Log ("accept_res = " + accept_res);
				}

				// RX message
				char[] msg = new char[1024];
				int bytes_read = 0;
				while(bytes_read >= 0)
				{
					bytes_read = zt.Read(accept_res, ref msg, 80);
					string msgstr = new string(msg);
					Debug.Log("MSG (" + bytes_read + "):" + msgstr);
				}
			}

			// UDP
			else if(type == SocketType.Dgram)
			{
				// Socket()
				sock = zt.Socket ((int)AddressFamily.InterNetwork, (int)SocketType.Dgram, (int)ProtocolType.Unspecified);
				Debug.Log ("sock = " + sock);

				// Bind()
				int port_num;
				int.TryParse(port.text,out port_num);
				int bind_res = zt.Bind(sock, "0.0.0.0", port_num);
				Debug.Log ("bind_res = " + bind_res);

				// RX message
				int bytes_read = 0, flags = 0, msg_len = 1024;
				char[] msg = new char[msg_len];
				while(bytes_read >= 0)
				{
					bytes_read = zt.RecvFrom(sock, ref msg, msg_len, flags, "0.0.0.0", port_num);
					string msgstr = new string(msg);
					Debug.Log("MSG (" + bytes_read + "):" + msgstr);
				}
			}
		});
		connectThread.IsBackground = true;
		connectThread.Start();
	}

	public void Disconnect()
	{
		GameObject addr_go = GameObject.Find ("inputServerAddress"); 
		GameObject port_go = GameObject.Find ("inputServerAddress"); 
		InputField addr = addr_go.GetComponents<InputField> () [0];
		InputField port = port_go.GetComponents<InputField> () [0];
		Debug.Log ("Disconnecting from: " + addr.text + ":" + port.text);
		Debug.Log ("Disconnect(): " + zt.Close (sock));
	}

	public void SendMessage()
	{
		// Get info from GUI
		GameObject go = GameObject.Find ("dropdownProtocol"); 
		Dropdown dd = go.GetComponents<Dropdown> () [0];
		Debug.Log("Protocol selected: " + dd.captionText.text);
		SocketType type = dd.captionText.text == "UDP" ? SocketType.Dgram : SocketType.Stream;

		GameObject input = GameObject.Find ("inputMessage"); 
		InputField msg = input.GetComponents<InputField> () [0];

		// Get port number from UI
		GameObject port_go = GameObject.Find ("inputServerPort"); 
		InputField port = port_go.GetComponents<InputField> () [0];
		int port_num;
		int.TryParse(port.text,out port_num);

		int bytes_written = 0;

		Thread sendThread = new Thread(() => { 
			// TCP
			if(type == SocketType.Stream) {
				bytes_written = zt.Write (sock, msg.text.ToCharArray(), msg.text.Length);
			}

			// UDP
			else if(type == SocketType.Dgram) {
				int flags = 0;
				string addr = "0.0.0.0";
				bytes_written = zt.SendTo(sock, msg.text.ToCharArray(), msg.text.Length, flags, addr, port_num);
			}
			Debug.Log ("bytes_written = " + bytes_written);
		});
		sendThread.IsBackground = true;
		sendThread.Start();
	}

	void Start()
	{
		// Set defaults
		InputField input;
		GameObject go;
		go = GameObject.Find ("inputNetworkID"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "8056c2e21c000001";
		go = GameObject.Find ("inputServerAddress"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "10.9.9.203";
		go = GameObject.Find ("inputServerPort"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "8080";
		go = GameObject.Find ("inputMessage"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "Welcome to the machine";

		// Create new instance of ZeroTier in separate thread
		zt = new ZTSDK ("zerotier/", "8056c2e21c000001");
	}

	// Terminate the ZeroTier service when the application quits
	void OnApplicationQuit() { 
		Debug.Log ("OnApplicationQuit()");
		zt.Terminate ();
	} 

	// Update is called once per frame
	void Update () {
		/*
		if (text) {
			text.text = IsRunning() ? "ZeroTier Status: Online" : "ZeroTier Status: Offline";
		}
		*/

		// ---
		/*
		int recHostId; 
		int connectionId; 
		int channelId; 
		byte[] recBuffer = new byte[1024]; 
		int bufferSize = 1024;
		int dataSize;
		byte error;
		NetworkEventType recData = zt.Receive(out recHostId, out connectionId, out channelId, recBuffer, bufferSize, out dataSize, out error);
		switch (recData)
		{
		case NetworkEventType.Nothing:
			break;
		case NetworkEventType.ConnectEvent:
			Debug.Log("NetworkEventType.ConnectEvent");
			break;
		case NetworkEventType.DataEvent:
			Debug.Log("NetworkEventType.DataEvent");
			break;
		case NetworkEventType.DisconnectEvent:
			Debug.Log("NetworkEventType.DisconnectEvent");
			break;
		}
	*/
		// ---

		/*
		GameObject go = GameObject.Find ("_txtStatusIndicator"); 
		Text text = go.GetComponents<Text> () [0];
		text.text = zt.IsRunning () ? "ZeroTier Service: ONLINE" : "ZeroTier Service: OFFLINE";
		*/

		// Rotate ZTCube when ZT is running
		/*
		if (zt.IsRunning ()) {


			go = GameObject.Find ("ZTCube"); 
			Vector3 rotvec = new Vector3 (10f, 10f, 10f);
			go.transform.Rotate (rotvec, speed * Time.deltaTime);
		}
		GameObject go = GameObject.Find("ZTCube"); 
		Text text = go.GetComponents<Text> ()[0];
		*/
	}
}