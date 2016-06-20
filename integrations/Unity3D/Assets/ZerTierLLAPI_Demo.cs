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

// Demonstrates the usage of the ZeroTier LLAPI (modeled after the Unity LLAPI)
public class ZeroTierLLAPI_Demo : MonoBehaviour
{
	public float speed = 300f;

	private ZeroTierLLAPI zt;
	string nwid = "";

	int server_connection_socket; // The "connection id"
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
		
	public void Connect()
	{
		GameObject addr_go = GameObject.Find ("inputServerAddress"); 
		GameObject port_go = GameObject.Find ("inputServerPort"); 
		InputField addr = addr_go.GetComponents<InputField> () [0];
		InputField port = port_go.GetComponents<InputField> () [0];
		Debug.Log ("Connecting to: " + addr.text + ":" + port.text);

		Thread connectThread = new Thread(() => { 
			byte error = 0;
			server_connection_socket = zt.Connect (0, addr.text, int.Parse (port.text), out error);
			Debug.Log ("server_connection_socket = " + server_connection_socket);
			Debug.Log ("Connect(): " + error);
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

		Thread connectThread = new Thread(() => { 
			byte error = 0;
			host_socket = zt.AddHost (int.Parse (port.text));
			Debug.Log ("host_socket = " + host_socket);
			Debug.Log ("Bind(): " + error);
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
		Debug.Log ("Disconnect(): " + zt.Disconnect (server_connection_socket));
	}
		
	public void SendMessage()
	{
		//zt_test_network ();
		GameObject go = GameObject.Find ("inputMessage"); 
		InputField msg = go.GetComponents<InputField> () [0];

		Thread sendThread = new Thread(() => { 
			Debug.Log ("Sending Message: " + msg.text);
			byte error = 0;
			zt.Send (server_connection_socket, msg.text.ToCharArray (), msg.text.ToCharArray ().Length, out error);
			Debug.Log ("Send(): " + error);
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
		input.text = "565799d8f6e1c11a";
		go = GameObject.Find ("inputServerAddress"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "172.22.211.245";
		go = GameObject.Find ("inputServerPort"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "8887";
		go = GameObject.Find ("inputMessage"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "Welcome to the machine";

		// Create new instance of ZeroTier in separate thread
		zt = new ZeroTierLLAPI ("/Users/Joseph/utest2/nc_565799d8f6e1c11a");

		/* This new instance will communicate via a named pipe, so any 
		 * API calls (ZeroTier.Connect(), ZeroTier.Send(), etc) will be sent to the service
		 * via this pipe.
		 */
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