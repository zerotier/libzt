package com.zerotier.one;

import android.app.Application;
import android.content.SharedPreferences;

import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;

import java.util.UUID;

/**
 * Created by Grant on 8/25/2015.
 */
public class AnalyticsApplication extends Application {
    private Tracker mTracker;

    /**
     * Gets the default {@link Tracker} for this {@link Application}.
     * @return tracker
     */
    synchronized public Tracker getDefaultTracker() {
        if (mTracker == null) {
            SharedPreferences prefs = getSharedPreferences("user", MODE_PRIVATE);

            String uuid = UUID.randomUUID().toString();
            String savedUUID = prefs.getString("uuid", UUID.randomUUID().toString());

            if(savedUUID.equals(uuid)) {
                // this is a newly generated UUID.  Save it
                SharedPreferences.Editor e = prefs.edit();
                e.putString("uuid", savedUUID);
                e.apply();
            }

            GoogleAnalytics analytics = GoogleAnalytics.getInstance(this);
            // To enable debug logging use: adb shell setprop log.tag.GAv4 DEBUG
            mTracker = analytics.newTracker(R.xml.app_tracker);

            mTracker.set("&uid", savedUUID);

            mTracker.send(new HitBuilders.ScreenViewBuilder()
                    .setNewSession()
                    .build());
        }
        return mTracker;
    }
}
