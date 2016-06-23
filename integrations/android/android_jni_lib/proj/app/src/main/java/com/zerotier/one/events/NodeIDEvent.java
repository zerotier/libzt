package com.zerotier.one.events;

/**
 * Created by Grant on 7/9/2015.
 */
public class NodeIDEvent {
    private long nodeId;

    public NodeIDEvent(long nodeId) {
        this.nodeId = nodeId;
    }

    public long getNodeId() {
        return nodeId;
    }
}
