package com.zerotier.one.service;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * Broadcast receiver that listens for system bootup and starts
 * the ZeroTier service
 */
public class StartupReceiver extends BroadcastReceiver {
    private static final String TAG = "StartupReceiver";

    @Override
    public void onReceive(Context ctx, Intent intent) {
        Log.i(TAG, "Received: " + intent.getAction()+ ". Starting ZeroTier One service.");

        // TODO: Start service
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(ctx);

        boolean shouldStart = prefs.getBoolean("general_start_zerotier_on_boot", true);
        if(shouldStart) {
            Log.i(TAG, "Preferences set to start ZeroTier on boot");
            Intent i = new Intent(ctx, RuntimeService.class);
            i.putExtra(RuntimeService.START_TYPE, RuntimeService.START_BOOT);
            ctx.startService(i);
        } else {
            Log.i(TAG, "Preferences set to not start ZeroTier on boot");
        }
    }
}
