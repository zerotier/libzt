package com.zerotier.one.ui;

import android.app.Activity;
import android.app.Fragment;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;
import com.zerotier.one.AnalyticsApplication;
import com.zerotier.one.R;
import com.zerotier.one.events.LeaveNetworkEvent;
import com.zerotier.one.events.ManualDisconnectEvent;
import com.zerotier.one.events.NetworkInfoReplyEvent;
import com.zerotier.one.events.NetworkListReplyEvent;
import com.zerotier.one.events.NetworkRemovedEvent;
import com.zerotier.one.events.NodeDestroyedEvent;
import com.zerotier.one.events.NodeIDEvent;
import com.zerotier.one.events.NodeStatusEvent;
import com.zerotier.one.events.RequestNetworkListEvent;
import com.zerotier.one.events.RequestNodeStatusEvent;
import com.zerotier.one.events.StopEvent;
import com.zerotier.one.service.RuntimeService;
import com.zerotier.one.service.ZeroTierOneService;
import com.zerotier.sdk.NodeStatus;
import com.zerotier.sdk.VirtualNetworkConfig;

import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Collections;

import de.greenrobot.event.EventBus;

public class NetworkListFragment extends Fragment {
    enum ConnectStatus {
        CONNECTED,
        DISCONNECTED
    }

    private Tracker tracker = null;

    public static final String TAG = "NetworkListFragment";

    public static final int START_VPN = 2;

    private ArrayList<VirtualNetworkConfig> mNetworkConfigs;

    private EventBus eventBus;
    private NetworkAdapter adapter;
    private ListView listView;
    private TextView nodeIdView;
    private TextView nodeStatusView;
    private Button connectButton;
    private MenuItem joinNetworkMenu;

    private ConnectStatus cstatus = ConnectStatus.DISCONNECTED;

    /**
     * Mandatory empty constructor for the fragment manager to instantiate the
     * fragment (e.g. upon screen orientation changes).
     */
    public NetworkListFragment() {
        Log.d(TAG, "Network List Fragment created");
        mNetworkConfigs = new ArrayList<VirtualNetworkConfig>();
        eventBus = EventBus.getDefault();
    }

    @Override
    public void onStart() {
        super.onStart();
        eventBus.register(this);
        eventBus.post(new RequestNetworkListEvent());
        eventBus.post(new RequestNodeStatusEvent());
    }

    public void onStop() {
        super.onStop();
        eventBus.unregister(this);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);


    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup parent, Bundle savedInstanceState) {
        super.onCreateView(inflater, parent, savedInstanceState);

        View v = inflater.inflate(R.layout.network_list_fragment, parent, false);

        listView = (ListView)v.findViewById(R.id.joined_networks_list);

        adapter = new NetworkAdapter(mNetworkConfigs);
        listView.setAdapter(adapter);

        nodeIdView = (TextView) v.findViewById(R.id.node_id);
        nodeStatusView = (TextView) v.findViewById(R.id.node_status);
        connectButton = (Button) v.findViewById(R.id.connect_button);

        if(cstatus == ConnectStatus.CONNECTED) {
            connectButton.setText(getResources().getText(R.string.button_disconnect));
        } else {
            connectButton.setText(getResources().getText(R.string.button_connect));
        }

        connectButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                if (cstatus == ConnectStatus.CONNECTED) {
                    Intent i = new Intent(getActivity(), RuntimeService.class);
                    i.putExtra(RuntimeService.START_TYPE, RuntimeService.STOP_USER_INTERFACE);
                    getActivity().startService(i);
                    cstatus = ConnectStatus.DISCONNECTED;
                    nodeStatusView.setText("OFFLINE");
                    connectButton.setText(getResources().getText(R.string.button_connect));

                } else {
                    sendStartServiceIntent();
                }
            }
        });

        return v;
    }

    private void sendStartServiceIntent() {
        Intent i = ZeroTierOneService.prepare(getActivity());
        if(i != null) {
            startActivityForResult(i, START_VPN);
        } else {
            Log.d(TAG, "Intent is NULL.  Already approved.");
            startService();
        }
    }


    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "NetworkListFragment.onCreate");
        super.onCreate(savedInstanceState);
        PreferenceManager.setDefaultValues(getActivity(), R.xml.preferences, false);

        setHasOptionsMenu(true);

        if(tracker == null) {
            tracker = ((AnalyticsApplication) getActivity().getApplication()).getDefaultTracker();
        }

        tracker.setScreenName("Network List");
        tracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    @Override
    public void onResume() {
        super.onResume();

        cstatus = ConnectStatus.DISCONNECTED;
        connectButton.setText(getResources().getText(R.string.button_connect));
        nodeStatusView.setText("OFFLINE");
        mNetworkConfigs.clear();
        sortNetworkListAndNotify();

        eventBus.post(new RequestNetworkListEvent());
        eventBus.post(new RequestNodeStatusEvent());

        if(tracker == null) {
            tracker = ((AnalyticsApplication) getActivity().getApplication()).getDefaultTracker();
        }
        tracker.setScreenName("Network List");
        tracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        Log.d(TAG, "NetworkListFragment.onCreateOptionsMenu");
        inflater.inflate(R.menu.menu_network_list, menu);
        joinNetworkMenu = menu.findItem(R.id.menu_item_join_network);
        joinNetworkMenu.setEnabled(false);
        super.onCreateOptionsMenu(menu, inflater);
        eventBus.post(new RequestNodeStatusEvent());
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_item_join_network: {
                Log.d(TAG, "Selected Join Network");
                Intent i = new Intent(getActivity(), JoinNetworkActivity.class);
                startActivity(i);
                return true;
            }
            case R.id.menu_item_prefs: {
                Log.d(TAG, "Selected Preferences");
                Intent i = new Intent(getActivity(), PrefsActivity.class);
                startActivity(i);
                return true;
            }
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {

        switch(requestCode)
        {
            case START_VPN:
            {
                // Start ZeroTierOneService
                startService();
            }
        }
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        Log.d(TAG, "NetworkListFragment.onAttach");
    }

    @Override
    public void onDetach() {
        Log.d(TAG, "NetworkListFragment.onDetach");
        super.onDetach();
    }

    public void onEventMainThread(NetworkListReplyEvent e) {
        Log.d(TAG, "Got network list");
        mNetworkConfigs.clear();
        for(int i = 0; i < e.getNetworkList().length; ++i) {
            mNetworkConfigs.add(e.getNetworkList()[i]);
        }

        sortNetworkListAndNotify();
    }

    public void onEventMainThread(NetworkInfoReplyEvent e) {
        Log.d(TAG, "Got Network Info");
        VirtualNetworkConfig vnc = e.getNetworkInfo();
        boolean hasNetworkWithId = false;

        for(VirtualNetworkConfig c : mNetworkConfigs) {
            if(c.networkId() == vnc.networkId()) {
                hasNetworkWithId = true;
                int index = mNetworkConfigs.indexOf(c);
                mNetworkConfigs.set(index, vnc);
                break;
            }
        }

        if(!hasNetworkWithId) {
            mNetworkConfigs.add(vnc);
        }

        sortNetworkListAndNotify();
    }

    public void onEventMainThread(NetworkRemovedEvent e) {
        Log.d(TAG, "Removing network: " + Long.toHexString(e.getNetworkId()));
        for(VirtualNetworkConfig c : mNetworkConfigs) {
            if(c.networkId() == e.getNetworkId()) {
                mNetworkConfigs.remove(c);
                break;
            }
        }

        sortNetworkListAndNotify();
    }

    public void onEventMainThread(NodeIDEvent e) {
        long nodeId = e.getNodeId();
        String nodeHex = Long.toHexString(nodeId);
        while(nodeHex.length() < 10) {
            nodeHex = "0" + nodeHex;
        }
        nodeIdView.setText(nodeHex);
    }

    public void onEventMainThread(NodeStatusEvent e) {
        NodeStatus status = e.getStatus();
        if(status.isOnline()) {
            nodeStatusView.setText("ONLINE");
            if(joinNetworkMenu != null) {
                joinNetworkMenu.setEnabled(true);
            }
            cstatus = ConnectStatus.CONNECTED;
            if(connectButton != null) {
                connectButton.setText(getResources().getText(R.string.button_disconnect));
            }
            if(nodeIdView != null) {
                nodeIdView.setText(Long.toHexString(status.getAddres()));
            }
        } else {
            setOfflineState();
        }
    }

    public void onEventMainThread(NodeDestroyedEvent e) {
        setOfflineState();
    }

    private void setOfflineState() {
        if(nodeStatusView != null) {
            nodeStatusView.setText("OFFLINE");
        }
        if(joinNetworkMenu != null) {
            joinNetworkMenu.setEnabled(false);
        }
        cstatus = ConnectStatus.DISCONNECTED;
        if(connectButton != null) {
            connectButton.setText(getResources().getText(R.string.button_connect));
        }
    }

    private void sortNetworkListAndNotify() {
        Collections.sort(mNetworkConfigs);
        adapter.notifyDataSetChanged();
        listView.invalidateViews();
    }

    private void startService() {
        Intent i = new Intent(getActivity(), RuntimeService.class);
        i.putExtra(RuntimeService.START_TYPE, RuntimeService.START_USER_INTERFACE);
        getActivity().startService(i);
    }

    private class NetworkAdapter extends ArrayAdapter<VirtualNetworkConfig> {

        public NetworkAdapter(ArrayList<VirtualNetworkConfig> config) {
            super(getActivity(), 0, config);
            Log.d(TAG, "Created network list item adapter");

        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if(!listView.getItemsCanFocus()) {
                listView.setItemsCanFocus(true);
            }

            if(convertView == null) {
                convertView = getActivity().getLayoutInflater().inflate(R.layout.list_item_network, null);
            }

            VirtualNetworkConfig vnc = getItem(position);
            TextView networkId = (TextView)convertView.findViewById(R.id.network_list_network_id);
            networkId.setText(Long.toHexString(vnc.networkId()));

            TextView networkName = (TextView) convertView.findViewById(R.id.network_list_network_name);
            networkName.setText(vnc.name());


            TextView networkStatus = (TextView)convertView.findViewById(R.id.network_status_textview);

            CharSequence statusText;
            switch(vnc.networkStatus()) {
                case NETWORK_STATUS_OK:
                    statusText = getResources().getText(R.string.network_status_ok);
                    break;
                case NETWORK_STATUS_ACCESS_DENIED:
                    statusText = getResources().getText(R.string.network_status_access_denied);
                    break;
                case NETWORK_STATUS_CLIENT_TOO_OLD:
                    statusText = getResources().getText(R.string.network_status_client_too_old);
                    break;
                case NETWORK_STATUS_NOT_FOUND:
                    statusText = getResources().getText(R.string.network_status_not_found);
                    break;
                case NETWORK_STATUS_PORT_ERROR:
                    statusText = getResources().getText(R.string.network_status_port_error);
                    break;
                case NETWORK_STATUS_REQUESTING_CONFIGURATION:
                    statusText = getResources().getText(R.string.network_status_requesting_configuration);
                    break;
                default:
                    statusText = getResources().getText(R.string.network_status_unknown);
                    break;
            }
            networkStatus.setText(statusText);

            TextView networkType = (TextView)convertView.findViewById(R.id.network_type_textview);
            switch(vnc.networkType()) {
                case NETWORK_TYPE_PUBLIC:
                    networkType.setText(getResources().getText(R.string.network_type_public));
                    break;
                case NETWORK_TYPE_PRIVATE:
                    networkType.setText(getResources().getText(R.string.network_type_private));
                    break;
                default:
                    networkType.setText(getResources().getText(R.string.network_type_unknown));
            }

            String mac = Long.toHexString(vnc.macAddress());
            while(mac.length() < 12) {
                mac = "0" + mac;
            }

            StringBuilder displayMac = new StringBuilder();
            displayMac.append(mac.charAt(0));
            displayMac.append(mac.charAt(1));
            displayMac.append(':');
            displayMac.append(mac.charAt(2));
            displayMac.append(mac.charAt(3));
            displayMac.append(':');
            displayMac.append(mac.charAt(4));
            displayMac.append(mac.charAt(5));
            displayMac.append(':');
            displayMac.append(mac.charAt(6));
            displayMac.append(mac.charAt(7));
            displayMac.append(':');
            displayMac.append(mac.charAt(8));
            displayMac.append(mac.charAt(9));
            displayMac.append(':');
            displayMac.append(mac.charAt(10));
            displayMac.append(mac.charAt(11));

            TextView macView = (TextView)convertView.findViewById(R.id.network_mac_textview);
            macView.setText(displayMac.toString());

            TextView mtuView = (TextView) convertView.findViewById(R.id.network_mtu_textview);
            mtuView.setText(Integer.toString(vnc.mtu()));

            TextView broadcastEnabledView = (TextView) convertView.findViewById(R.id.network_broadcast_textview);
            broadcastEnabledView.setText(vnc.broadcastEnabled() ? "Enabled" : "Disabled");

            TextView bridgingEnabledView = (TextView) convertView.findViewById(R.id.network_bridging_textview);
            bridgingEnabledView.setText(vnc.isBridgeEnabled() ? "Enabled" : "Disabled");

            InetSocketAddress[] addresses = vnc.assignedAddresses();
            StringBuilder addrText = new StringBuilder();
            for(InetSocketAddress addr : addresses) {
                InetAddress ipaddr = addr.getAddress();
                if(ipaddr instanceof Inet6Address) {
                    continue;
                }
                String address = addr.toString();
                if(address.startsWith("/")) {
                    address = address.substring(1);
                }
                int lastSlashPosition = address.lastIndexOf(':');
                address = address.substring(0, lastSlashPosition) + "/" + address.substring(lastSlashPosition+1, address.length());
                addrText.append(address);
                addrText.append('\n');
            }

            TextView addressesView = (TextView) convertView.findViewById(R.id.network_ipaddresses_textview);
            addressesView.setText(addrText.toString());

            Button leaveButton = (Button) convertView.findViewById(R.id.network_leave_button);
            leaveButton.setOnClickListener(new LeaveButtonClickListener(vnc.networkId()));

            return convertView;
        }
    }

    private class LeaveButtonClickListener implements Button.OnClickListener {

        private long nwid;

        public LeaveButtonClickListener(long nwid) {
            this.nwid = nwid;
        }

        @Override
        public void onClick(View v) {
            Log.d(TAG, "Leave Button Pressed for network: " + Long.toHexString(nwid));
            EventBus.getDefault().post(new LeaveNetworkEvent(nwid));
        }
    }
}
