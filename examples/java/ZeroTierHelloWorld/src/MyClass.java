// Hello World example for the ZeroTierSDK

import zerotier.*;

public class MyClass {
	
    public native int loadsymbols();
    public native void startOneService();
    
	static {
        System.loadLibrary("zt");
    }

	public static void main(String[] args) {
		
        System.out.println("Welcome to the Machine");
       
        final ZeroTier z = new ZeroTier();
        
        new Thread(new Runnable() {
            public void run() {
                // Calls to JNI code
                z.start("/Users/Joseph/op/code/zerotier/ZeroTierSDK/zt1");
            }
        }).start();
                
        //while(!z.running()) { }

        while(true)
        {
        	try {
        		System.out.println("Welcome to the Machine");
                Thread.sleep(3000);
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }
}