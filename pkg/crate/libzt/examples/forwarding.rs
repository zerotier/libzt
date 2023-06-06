use std::{
    error::Error,
    fmt, fs,
    io::{self, Read, Write},
    net::{self, Shutdown},
    sync::Arc,
    thread,
    time::Duration,
};

use anyhow::Result;
use clap::Parser;
use parking_lot::FairMutex;

use libzt::{node::ZeroTierNode, tcp as zt_tcp};

const BUF_SIZE: usize = 1024;
const SOCKET_IO_TIMEOUT: Duration = Duration::new(1, 0);
const DEFAULT_LISTEN_PORT: u16 = 9080;

#[derive(Debug)]
enum ForwardingError {
    BindFailed(String),
    ConnectFailed(String),
    CopyFailed(String),
}

impl Error for ForwardingError {}

#[derive(Parser, Debug)]
struct Args {
    #[arg(short, long, value_parser=clap_num::maybe_hex::<u64>)]
    network_id: u64,
    #[arg(short, long)]
    connect: String,
    #[arg(short, long, default_value_t = DEFAULT_LISTEN_PORT)]
    port: u16,
}

fn setup_node(network_id: u64) -> Result<ZeroTierNode> {
    log::info!("joining network: {:#x}", network_id);

    let mut storage_path = dirs::data_local_dir().unwrap();
    storage_path.push("libzt");
    storage_path.push("forwarding");

    fs::create_dir_all(&storage_path)?;

    log::debug!(
        "initializing from state dir: {}",
        storage_path.to_string_lossy()
    );

    let node = ZeroTierNode {};

    node.init_set_port(0);
    node.init_from_storage(&storage_path.join("libzt-examples").to_string_lossy());
    node.start();

    log::debug!("waiting for node to come online...");
    while !node.is_online() {
        node.delay(250);
    }

    log::info!("node id: {:#x}", node.id());

    node.net_join(network_id);

    log::debug!("waiting for transport...");
    while !node.net_transport_is_ready(network_id) {
        node.delay(250);
    }

    let addr = node.addr_get(network_id).unwrap();
    log::info!("got ZT addr: {}", addr.to_string());

    Ok(node)
}

impl fmt::Display for ForwardingError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

fn main() -> Result<()> {
    env_logger::init();

    let Args {
        connect: remote_addr,
        network_id,
        port: local_port,
    } = Args::parse();

    let _node = setup_node(network_id)?;

    let local_addr = format!("localhost:{}", local_port);
    let listener = net::TcpListener::bind(&local_addr)
        .map_err(|_| ForwardingError::BindFailed(local_addr.clone()))?;

    log::info!("listener bound to {}", local_addr);

    for conn in listener.incoming() {
        log::debug!("incoming: {:?}", conn);

        if let Ok(client) = conn {
            client.set_read_timeout(Some(SOCKET_IO_TIMEOUT))?;
            client.set_write_timeout(Some(SOCKET_IO_TIMEOUT))?;

            let client = Arc::new(FairMutex::new(client));

            log::info!("connecting to remote: {}", remote_addr);

            let remote = zt_tcp::TcpStream::connect(&remote_addr)
                .map_err(|_| ForwardingError::ConnectFailed(remote_addr.clone()))?;

            remote.set_read_timeout(Some(SOCKET_IO_TIMEOUT))?;
            remote.set_write_timeout(Some(SOCKET_IO_TIMEOUT))?;

            let remote = Arc::new(FairMutex::new(remote));

            log::info!("connected");

            let client_ = client.clone();
            let remote_ = remote.clone();

            thread::spawn(move || {
                let mut buf = [0u8; BUF_SIZE];
                let mut running = true;

                while running {
                    let mut client = client.lock();
                    let mut remote = remote.lock();

                    match client.read(&mut buf) {
                        Ok(n) if n > 0 => {
                            log::debug!("got {} bytes from remote", n);
                            running = remote
                                .write(&buf)
                                .map(|_| {
                                    remote.flush().unwrap();
                                    true
                                })
                                .unwrap_or_else(|e| {
                                    let msg = format!("failed to write to remote: {:?}", e);
                                    log::error!("{}", msg);
                                    false
                                });
                        }
                        Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
                            thread::sleep(Duration::new(0, 5000));
                        }
                        other => {
                            log::debug!("closing client connection: {:?}", other);
                            let _ = client.shutdown(Shutdown::Read);
                            let _ = remote.shutdown(Shutdown::Write);
                            break;
                        }
                    }

                    buf.fill(0);
                }
            });
            thread::spawn(move || {
                let mut buf = [0u8; BUF_SIZE];
                let mut running = true;

                while running {
                    let mut client = client_.lock();
                    let mut remote = remote_.lock();

                    match remote.read(&mut buf) {
                        Ok(n) if n > 0 => {
                            log::debug!("got {} bytes from remote", n);
                            running = client
                                .write(&buf)
                                .map(|_| {
                                    client.flush().unwrap();
                                    true
                                })
                                .unwrap_or_else(|e| {
                                    let msg = format!("failed to write to remote: {:?}", e);
                                    log::error!("{}", msg);
                                    false
                                });
                        }
                        other => {
                            log::debug!("closing remote connection: {:?}", other);
                            let _ = remote.shutdown(Shutdown::Read);
                            let _ = client.shutdown(Shutdown::Write);
                            break;
                        }
                    }

                    buf.fill(0);
                }
            });
        }
    }

    Ok(())
}
