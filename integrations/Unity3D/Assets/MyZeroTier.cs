using UnityEngine;
using System.Collections;

public class MyZeroTier : MonoBehaviour {
	void Start() {
		Application.OpenURL("https://my.zerotier.com");
	}
}
