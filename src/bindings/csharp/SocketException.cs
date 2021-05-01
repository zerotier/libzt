/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

using System;

namespace ZeroTier.Sockets
{
    /// <summary>Exception class for ZeroTier service and low-level socket errors</summary>
    public class SocketException : Exception {
        public SocketException(int _serviceErrorCode)
            : base(String.Format("ServiceErrorCode={0} (See Constants.cs for error code meanings)", _serviceErrorCode))
        {
            ServiceErrorCode = _serviceErrorCode;
        }
        public SocketException(int _serviceErrorCode, int _socketErrorCode)
            : base(String.Format(
                "ServiceErrorCode={0}, SocketErrorCode={1} (See Constants.cs for error code meanings)",
                _serviceErrorCode,
                _socketErrorCode))
        {
            ServiceErrorCode = _serviceErrorCode;
            SocketErrorCode = _socketErrorCode;
        }
        /// <value>High-level service error code. See Constants.cs</value>
        public int ServiceErrorCode { get; private set; }

        /// <value>Low-level socket error code. See Constants.cs</value>
        public int SocketErrorCode { get; private set; }
    }
}
