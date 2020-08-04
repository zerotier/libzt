var nbind = require('@mcesystems/nbind')
var ZeroTier = nbind.init().lib.ZeroTier

// Start ZeroTier service
ZeroTier.start(".zerotier", 9994);

// Join virtual network
ZeroTier.join("8056c2e21c000001");

// Open the socket
let fd = ZeroTier.connectStream("29.49.7.203", 4444);

// Send some data
ZeroTier.send(fd, Buffer.from("Name?\n", 'utf8'), 0)

// Set blocking read mode
// ZeroTier.fcntlSetBlocking(fd, true);
let heartbeat = setInterval(() => process.stderr.write('.'), 100)

// Receive some data
const _read = () => {
    const buf = Buffer.alloc(32)
    let bytes = -1
    do {
        bytes = ZeroTier.recv(fd, buf, 0)
        if (bytes > 0) { process.stdout.write(buf.toString('utf8')) }
    } while (bytes > 0);

    if (!ZeroTier.getMyNode().online || buf.toString('utf8').includes("exit")) {
        // Close the socket
        ZeroTier.close(fd)
        // Stop ZeroTier service
        ZeroTier.stop()
        // Clear the interval
        clearInterval(heartbeat)
    } else {
        setTimeout(_read, 500)
    }
}
_read()

