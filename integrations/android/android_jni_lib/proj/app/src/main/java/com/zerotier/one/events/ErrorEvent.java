package com.zerotier.one.events;

import com.zerotier.sdk.ResultCode;

/**
 * Created by Grant on 6/23/2015.
 */
public class ErrorEvent {
    ResultCode result;

    public ErrorEvent(ResultCode rc) {
        result = rc;
    }

    public String getError() {
        return result.toString();
    }
}
