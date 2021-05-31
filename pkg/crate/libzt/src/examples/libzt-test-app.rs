use libzt;

use std::env;

use libzt::tcp::{TcpListener, TcpStream};
use std::io::{Read, Write};
use std::net::Shutdown;
use std::str::from_utf8;
use std::thread;

// (Optional) Notify application of ZeroTier events, some with context
fn user_event_handler(event_code: i16) -> () {
    println!("user_event {}", event_code);
}

fn handle_client(mut stream: TcpStream) {
    let mut data = [0 as u8; 50]; // using 50 byte buffer
    while match stream.read(&mut data) {
        Ok(size) => {
            // echo everything!
            stream.write(&data[0..size]).unwrap();
            true
        }
        Err(_) => {
            println!(
                "An error occurred, terminating connection with {}",
                stream.peer_addr().unwrap()
            );
            stream.shutdown(Shutdown::Both).unwrap();
            false
        }
    } {}
}

fn main() -> std::io::Result<()> {
    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);

    if args.len() != 5 && args.len() != 6 {
        println!("Incorrect number of arguments.");
        println!("  Usage: libzt-test-app server <storage_path> <net_id> <local_ip> <local_port>");
        println!("  Usage: libzt-test-app client <storage_path> <net_id> <remote_ip> <remote_port>");
        return Ok(())
    }

    let storage_path = &args[2];
    let net_id = u64::from_str_radix(&args[3], 16).unwrap();

    println!("path   = {}", storage_path);
    println!("net_id = {:x}", net_id);

    // SET UP ZEROTIER

    let node = libzt::node::ZeroTierNode {};
    // (Optional) initialization
    node.init_set_port(0);
    node.init_set_event_handler(user_event_handler);
    node.init_from_storage(&storage_path);
    // Start the node
    node.start();
    println!("Waiting for node to come online...");
    while !node.is_online() {
        node.delay(50);
    }
    println!("Node ID = {:#06x}", node.id());
    println!("Joining network");
    node.net_join(net_id);
    println!("Waiting for network to assign addresses...");
    while !node.net_transport_is_ready(net_id) {
        node.delay(50);
    }
    let addr = node.addr_get(net_id).unwrap();
    println!("Assigned addr = {}", addr);

    // Server

    if &args[1] == "server" {
        println!("server mode");
        let mut addr_str: String = "".to_owned();
        addr_str.push_str(&args[4]);
        addr_str.push_str(":");
        addr_str.push_str(&args[5]);

        let server: std::net::SocketAddr =
            addr_str.parse().expect("Unable to parse socket address");
        let listener = TcpListener::bind(&server).unwrap();

        for stream in listener.incoming() {
            match stream {
                Ok(stream) => {
                    println!("New connection: {}", stream.peer_addr().unwrap());
                    thread::spawn(move || {
                        // connection succeeded
                        handle_client(stream)
                    });
                }
                Err(e) => {
                    println!("Error: {}", e);
                    // connection failed
                }
            }
        }
        drop(listener);
    }

    // Client

    if &args[1] == "client" {
        println!("client mode");
        let mut addr_str: String = "".to_owned();
        addr_str.push_str(&args[4]);
        addr_str.push_str(":");
        addr_str.push_str(&args[5]);
        match TcpStream::connect(addr_str) {
            Ok(mut stream) => {
                println!("Successfully connected to server");

                let msg = b"Hello, network!";

                stream.write(msg).unwrap();

                let mut data = [0 as u8; 15];
                match stream.read_exact(&mut data) {
                    Ok(_) => {
                        if &data == msg {
                            println!("Reply is ok!");
                        } else {
                            let text = from_utf8(&data).unwrap();
                            println!("Unexpected reply: {}", text);
                        }
                    }
                    Err(e) => {
                        println!("Failed to receive data: {}", e);
                    }
                }
            }
            Err(e) => {
                println!("Failed to connect: {}", e);
            }
        }
        println!("Terminated.");
    }

    node.stop();
    Ok(())
}
