package com.zerotier.one.ui;

import android.app.Fragment;

/**
 * Created by Grant on 5/20/2015.
 */
public class NetworkListActivity extends SingleFragmentActivity {
    @Override
    public Fragment createFragment() {
        return new NetworkListFragment();
    }
}
