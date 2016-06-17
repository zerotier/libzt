using UnityEngine;
using System.Collections;
using System.Threading;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using UnityEngine.UI;

public class Demo : MonoBehaviour
{
	public float speed = 300f;

	private ZeroTierNetworkInterface zt;
	string nwid = "";

	int server_connection_socket; // The "connection id"

	private void zt_sample_network_test_thread()
	{
		print("test_network");

		byte error;
		// Prepare sample data buffer
		/*
		byte[] buffer = new byte[1024];
		Stream stream = new MemoryStream(buffer);
		BinaryFormatter f = new BinaryFormatter();
		f.Serialize ( stream , "Welcome to the machine! (from Unity3D)" );
		int error;
		*/

		// Connect to server
		int connfd = zt.Connect (0, "172.22.211.245", 8888, out error);
		print(connfd);

		// Send sample data to server
		//int bytes_written = zt.Send(connfd,buffer,0, out error);
		//print(bytes_written);

		//char[] buffer = new char[1024];
		//buffer = "hello".ToCharArray();
		//print (buffer);
		//Stream stream = new MemoryStream(buffer);
		//BinaryFormatter formatter = new BinaryFormatter();
		//formatter.Serialize(stream, "HelloServer");
		//int bufferSize = 1024;

		Debug.Log ("Sending...");
		int bytes_written = zt.Send(connfd, "hello".ToCharArray(),4, out error);
		print(bytes_written);
	}

	public void zt_test_network()
	{
		Thread networkTestThread = new Thread(() => { zt_sample_network_test_thread();});
		networkTestThread.IsBackground = true;
		networkTestThread.Start();
	}

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
		input.text = "8888";
		go = GameObject.Find ("inputMessage"); 
		input = go.GetComponents<InputField> () [0];
		input.text = "Welcome to the machine";

		// Create new instance of ZeroTier in separate thread
		zt = new ZeroTierNetworkInterface ("/Users/Joseph/utest2");

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
		*/

		/*
		GameObject go = GameObject.Find("ZTCube"); 
		Text text = go.GetComponents<Text> ()[0];
		if (text) {
			text.text = IsRunning() ? "ZeroTier Status: Online" : "ZeroTier Status: Offline";
		}
		*/
	}
}