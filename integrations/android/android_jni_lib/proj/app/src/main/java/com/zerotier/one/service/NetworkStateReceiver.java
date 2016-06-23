package com.zerotier.one.service;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

import com.zerotier.one.events.StopEvent;

import de.greenrobot.event.EventBus;

/**
 * Created by Grant on 7/7/2015.
 */
public class NetworkStateReceiver extends BroadcastReceiver {
    private static String TAG = "NetworkStateReceiver";

    @Override
    public void onReceive(Context ctx, Intent intent) {
        ConnectivityManager cm =
                (ConnectivityManager)ctx.getSystemService(Context.CONNECTIVITY_SERVICE);

        NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
        boolean isConnected = (activeNetwork != null &&
                activeNetwork.isConnectedOrConnecting());

        Intent i = new Intent(ctx, RuntimeService.class);
        if(isConnected) {
            Log.d(TAG, "Network State: Connected");
            i.putExtra(RuntimeService.START_TYPE, RuntimeService.START_NETWORK_CHANGE);
            ctx.startService(i);
        } else {
            Log.d(TAG, "Network State: Disconnected");
            i.putExtra(RuntimeService.START_TYPE, RuntimeService.STOP_NETWORK_CHANGE);
            ctx.startService(i);
        }
    }
}
