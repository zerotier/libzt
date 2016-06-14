package com.zerotier.one.events;

import com.zerotier.sdk.NodeStatus;

/**
 * Created by Grant on 7/9/2015.
 */
public class NodeStatusEvent {
    private NodeStatus status;

    public NodeStatusEvent(NodeStatus status) {
        this.status = status;
    }

    public NodeStatus getStatus() {
        return status;
    }
}
