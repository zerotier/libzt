package com.zerotier.one.ui;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceFragment;

import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;
import com.zerotier.one.AnalyticsApplication;
import com.zerotier.one.R;
import com.zerotier.one.service.ZeroTierOneService;

/**
 * Created by Grant on 7/7/2015.
 */
public class PrefsFragment extends PreferenceFragment implements SharedPreferences.OnSharedPreferenceChangeListener {
    private Tracker tracker = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.preferences);

        if(tracker == null) {
            tracker = ((AnalyticsApplication) getActivity().getApplication()).getDefaultTracker();
        }

        tracker.setScreenName("Preferences");
        tracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    @Override
    public void onResume() {
        super.onResume();
        if(tracker == null) {
            tracker = ((AnalyticsApplication) getActivity().getApplication()).getDefaultTracker();
        }

        tracker.setScreenName("Preferences");
        tracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if(key.equals("network_use_cellular_data")) {
            if(sharedPreferences.getBoolean("network_use_cellular_data", false)) {
                Intent i = new Intent(getActivity(), ZeroTierOneService.class);
                getActivity().startService(i);
            }
        }
    }
}
