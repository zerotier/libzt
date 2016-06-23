package com.zerotier.one.service;

import android.app.Service;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.util.Log;

import com.zerotier.one.events.ManualDisconnectEvent;
import com.zerotier.one.events.StopEvent;

import de.greenrobot.event.EventBus;

public class RuntimeService extends Service {
    public static final String START_TYPE = "com.zerotier.one.service.RuntimeService.start_type";
    public static final int START_NETWORK_CHANGE = 1;
    public static final int START_BOOT = 2;
    public static final int START_USER_INTERFACE = 3;
    public static final int STOP_NETWORK_CHANGE = 4;
    public static final int STOP_USER_INTERFACE = 5;

    private static final String TAG = "RuntimeService";
    private EventBus eventBus = EventBus.getDefault();

    private boolean serviceStarted = false;

    private NetworkStateReceiver nsReceiver = null;

    public RuntimeService() {

    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "RuntimeService started");

        if(nsReceiver == null) {
            IntentFilter filter = new IntentFilter();
            filter.addAction("android.net.conn.CONNECTIVITY_CHANGE");
            nsReceiver = new NetworkStateReceiver();
            registerReceiver(nsReceiver, filter);
        }

        if(intent == null) {
            return START_STICKY;
        }

        int startMode = intent.getIntExtra(START_TYPE, 0);
        // in each of the following cases, Prepare should always return null as the prepare call
        // is called the first time the UI is started granting permission for ZeroTier to use
        // the VPN API
        switch (startMode) {
            case START_NETWORK_CHANGE: {
                Log.d(TAG, "Start Network change");
                if(serviceStarted) {
                    // start ZeroTierOne service.
                    Log.d(TAG, "Start Network Change");
                    Intent i = ZeroTierOneService.prepare(this);
                    if(i == null) {
                        i = new Intent(this, ZeroTierOneService.class);
                        startService(i);
                        serviceStarted = true;
                    }
                } else {
                    Log.d(TAG, "Ignore Start Network Change: Service has not been manually started.");
                }
                break;
            }
            case START_BOOT: {
                // if start on boot, start service
                Log.d(TAG, "Start Boot");
                Intent i = ZeroTierOneService.prepare(this);
                if(i == null) {
                    i = new Intent(this, ZeroTierOneService.class);
                    startService(i);
                    serviceStarted = true;
                }
                break;
            }
            case START_USER_INTERFACE: {
                Log.d(TAG, "Start User Interface");
                Intent i = ZeroTierOneService.prepare(this);
                if(i == null) {
                    i = new Intent(this, ZeroTierOneService.class);
                    startService(i);
                    serviceStarted = true;
                }
                break;
            }
            case STOP_NETWORK_CHANGE: {
                Log.d(TAG, "Stop Network Change.  Ignored.");
//                Intent i = new Intent(this, ZeroTierOneService.class);
//                stopService(i);
//                EventBus.getDefault().post(new StopEvent());
                break;
            }
            case STOP_USER_INTERFACE: {
                Log.d(TAG, "Stop User Interface");
                Intent i = new Intent(this, ZeroTierOneService.class);
                stopService(i);
                EventBus.getDefault().post(new ManualDisconnectEvent());
                break;
            }
            default:
                Log.e(TAG, "Unknown start ID: " + startId);
                break;
        }
        return START_STICKY;
    }
}
