package com.zerotier.one.ui;

import android.app.Fragment;

/**
 * Created by Grant on 7/7/2015.
 */
public class PrefsActivity  extends SingleFragmentActivity {
    @Override
    public Fragment createFragment() {
        return new PrefsFragment();
    }
}
