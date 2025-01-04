# UDP Ping Client and Server  

This project consists of a **UDP ping client and server** written in C++.  
The client sends UDP packets to the server, which echoes them back.  
Useful for measuring round-trip time (RTT) and network diagnostics.

---

## Build Instructions  

### Requirements:  
- **g++** (GNU C++ compiler)  
- **Make**  

### Build:  
1. Run the following command to compile both the client and the server:  
   ```bash
   make
   ```

2. To clean up object files and binaries:  
   ```bash
   make clean
   ```

---

## Usage  

### 1. UDP Ping Server  
The server listens for incoming UDP packets and echoes them back to the sender.  

#### Start the Server:  
```bash
./udp-ping-server [-p port] [-a subnet]
```

#### Options:  
- **`-p <port>`** – (Optional) Port to listen on. Default is **7777**.  
- **`-a <subnet>`** – (Optional) Allow packets from specific subnets.  
   - Can be used multiple times to add multiple subnets.  
   - If no subnet is specified, **all IPs are allowed**.  

#### Examples:  
- Start the server on the default port (7777) and allow all IPs:  
  ```bash
  ./udp-ping-server
  ```  
- Start the server on port 9000, allowing packets from `192.168.1.x` and `10.0.0.x`:  
  ```bash
  ./udp-ping-server -p 9000 -a 192.168.1. -a 10.0.0.
  ```

---

### 2. UDP Ping Client  
The client sends UDP packets to the server and calculates the round-trip time (RTT).  

#### Start the Client:  
```bash
./udp-ping-client <server_ip_or_hostname> [-i interval] [-c count] [-d duration] [-s payload_size]
```

#### Options:  
- **`-p <port>`** – (Optional) Port. Default is **7777**.  
- **`-i <interval>`** – (Optional) Interval between packets in seconds (supports fractions, e.g., `0.05`). Default is **1 second**.  
- **`-c <count>`** – (Optional) Number of packets to send. Default is **infinite**.  
- **`-d <duration>`** – (Optional) Stop after a specified duration (in seconds).  
- **`-s <payload_size>`** – (Optional) Size of the payload in bytes. Default is **64 bytes**.  

#### Examples:  
- Send packets to `192.168.0.10` every second, indefinitely:  
  ```bash
  ./udp-ping-client 192.168.0.10
  ```  
- Send packets every 50 ms for 30 seconds:  
  ```bash
  ./udp-ping-client 192.168.0.10 -i 0.05 -d 30
  ```  
- Send 100 packets with 512-byte payloads:  
  ```bash
  ./udp-ping-client 192.168.0.10 -c 100 -s 512
  ```

---

## How It Works  
- The **client** sends UDP packets containing a sequence number and timestamp.  
- The **server** echoes the packets back.  
- The client calculates **round-trip time (RTT)** for each packet and prints the result.  
- High latency and packet loss are reported in the statistics.

---

## Example Output  
```bash
Response - seq: 1, rtt: 16.35 ms
Response - seq: 2, rtt: 32.50 ms [HIGH LATENCY]
Response - seq: 3, rtt: 15.28 ms
```

---

## Notes:  
- **Performance Optimization**: Logging for accepted packets is disabled to improve server performance.  
- Only **rejected packets** from unauthorized IPs are logged.  
