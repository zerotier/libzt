package com.zerotier.one.ui;

import android.app.Fragment;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;

import com.zerotier.one.AnalyticsApplication;
import com.zerotier.one.R;
import com.zerotier.one.events.JoinNetworkEvent;
import com.zerotier.one.util.NetworkIdUtils;

import org.json.JSONArray;

import java.util.ArrayList;

import de.greenrobot.event.EventBus;

/**
 * Created by Grant on 5/20/2015.
 */
public class JoinNetworkFragment extends Fragment {
    public final static String TAG = "JoinNetwork";

    private Tracker tracker = null;

    private Button mJoinButton;
    private EditText mNetworkIdTextView;
    private ListView mRecentNetworksList;

    EventBus eventBus = EventBus.getDefault();

    HexKeyboard mHexKeyboard;

    public JoinNetworkFragment() {

    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if(tracker == null) {
            tracker = ((AnalyticsApplication) getActivity().getApplication()).getDefaultTracker();
        }

        tracker.setScreenName("Join  Network");
        tracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    @Override
    public void onResume() {
        super.onResume();

        if(tracker == null) {
            tracker = ((AnalyticsApplication) getActivity().getApplication()).getDefaultTracker();
        }

        tracker.setScreenName("Join  Network");
        tracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mHexKeyboard = new HexKeyboard(getActivity(), R.id.join_network_keyboard, R.xml.hex_keyboard);
        mHexKeyboard.registerEditText(R.id.join_network_edit_text);
        mHexKeyboard.hideCustomKeyboard();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup parent, Bundle savedInstanceState) {
        super.onCreateView(inflater, parent, savedInstanceState);

        View v = inflater.inflate(R.layout.fragment_join_network, parent, false);

        mNetworkIdTextView = (EditText)v.findViewById(R.id.join_network_edit_text);
        mNetworkIdTextView.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                if (s.toString().length() == 16) {
                    mJoinButton.setEnabled(true);
                } else {
                    mJoinButton.setEnabled(false);
                }
            }

            @Override
            public void afterTextChanged(Editable s) {

            }
        });

        mJoinButton = (Button)v.findViewById(R.id.button_join_network);
        mJoinButton.setEnabled(false);
        mJoinButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    Log.d(TAG, "Joining network " + mNetworkIdTextView.getText().toString());
                    String netIdString = mNetworkIdTextView.getText().toString();
                    long networkId = NetworkIdUtils.hexStringToLong(netIdString);

                    SharedPreferences prefs = getActivity().getSharedPreferences("recent_networks", Context.MODE_PRIVATE);
                    try {
                        String nws = prefs.getString("recent_networks", (new JSONArray()).toString());
                        JSONArray jArray = new JSONArray(nws);
                        ArrayList<String> array = new ArrayList<>();

                        // convert the JSON array to an actual array for ease of modification
                        for(int i = 0; i < jArray.length(); ++i) {
                            array.add(jArray.getString(i));
                        }

                        boolean containsValue = false;

                        for(String id : array) {
                            if(id.equals(netIdString)) {
                                containsValue = true;
                                break;
                            }
                        }

                        if(containsValue) {
                            // remove the item
                            array.remove(netIdString);
                        } else {
                            // pop off the last item
                            if (array.size() > 4) {
                                array.remove(4);
                            }

                        }

                        // insert ID at the beginning of the list
                        array.add(0, mNetworkIdTextView.getText().toString());

                        // convert the list back to a JSON array
                        jArray = new JSONArray();
                        for(String id : array) {
                            jArray.put(id);
                        }

                        // write JSON array to preferences
                        SharedPreferences.Editor e = prefs.edit();
                        e.putString("recent_networks", jArray.toString());
                        e.apply();
                    } catch (Exception e) {
                        Log.e(TAG, "Exception setting recent networks: " + e.getMessage());
                    }
                    eventBus.post(new JoinNetworkEvent(networkId));
                } catch (Throwable t) {
                    t.printStackTrace();
                    Log.d(TAG, "Join Network:  Error parsing network ID");
                } finally {
                    getActivity().onBackPressed();
                }
            }
        });

        mRecentNetworksList = (ListView) v.findViewById(R.id.recent_networks_list);

        SharedPreferences prefs = getActivity().getSharedPreferences("recent_networks", Context.MODE_PRIVATE);
        try {
            String nws = prefs.getString("recent_networks", new JSONArray().toString());
            JSONArray networks = new JSONArray(nws);


            TextView recentNetworksText = (TextView) v.findViewById(R.id.recent_networks);

            if (networks.length() == 0) {
                mRecentNetworksList.setVisibility(View.GONE);
                recentNetworksText.setVisibility(View.GONE);
            } else {
                mRecentNetworksList.setVisibility(View.VISIBLE);
                recentNetworksText.setVisibility(View.VISIBLE);

                ArrayList<String> recentNetworks = new ArrayList<>();
                for(int i = 0; i < networks.length(); ++i) {
                    recentNetworks.add(networks.getString(i));
                }

                final NetworkIDAdapter adapter = new NetworkIDAdapter(recentNetworks);
                mRecentNetworksList.setAdapter(adapter);

                mRecentNetworksList.setOnItemClickListener(new ListView.OnItemClickListener() {
                    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                        mNetworkIdTextView.setText(adapter.getItem(position));
                    }
                });
            }
        } catch (Exception e) {
            Log.e(TAG, "JSON Error: " + e.getMessage());
        }

        return v;
    }


    private class NetworkIDAdapter extends ArrayAdapter<String> {
        public NetworkIDAdapter(ArrayList<String> config) {
            super(getActivity(), 0, config);
            Log.d(TAG, "Created network list item adapter");

        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if(convertView == null) {
                convertView = getActivity().getLayoutInflater().inflate(R.layout.list_item_recent_network, null);
            }

            String networkId = getItem(position);
            TextView network = (TextView) convertView.findViewById(R.id.list_recent_network_id);
            network.setText(networkId);

            return convertView;
        }
    }
}
