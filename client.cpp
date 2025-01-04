#include <iostream>
#include <iomanip>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <csignal>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <unordered_set>
#include <poll.h>

#define PORT 7777
#define DEFAULT_INTERVAL 1.0
#define DEFAULT_PAYLOAD_SIZE 32
#define UNLIMITED -1

using namespace std::chrono;

// Global Variables
std::vector<double> rtt_values;
std::unordered_set<uint32_t> sent_packets;
std::unordered_set<uint32_t> received_packets;
uint32_t packets_sent = 0;
uint32_t packets_received = 0;
uint32_t high_latency_count = 0;
bool running = true;
double interval_s = DEFAULT_INTERVAL;
int max_count = UNLIMITED;
int max_duration = UNLIMITED;
int payload_size = DEFAULT_PAYLOAD_SIZE;
steady_clock::time_point start_time;
int port = PORT;  // Default port

// Signal handler for Ctrl+C
void handle_signal(int) {
    running = false;
}

// Print final statistics
void print_statistics() {
    uint32_t packets_lost = packets_sent - packets_received;
    double loss_percent = (packets_lost * 100.0) / packets_sent;

    std::cout << "\n--- Ping Statistics ---\n";
    std::cout << "Packets sent: " << packets_sent
              << ", Received: " << packets_received
              << ", Lost: " << packets_lost
              << " (" << std::fixed << std::setprecision(2) << loss_percent << "% loss)\n";

    if (packets_received > 0) {
        double min_rtt = *std::min_element(rtt_values.begin(), rtt_values.end());
        double max_rtt = *std::max_element(rtt_values.begin(), rtt_values.end());
        double sum_rtt = std::accumulate(rtt_values.begin(), rtt_values.end(), 0.0);
        double avg_rtt = sum_rtt / packets_received;

        double variance = 0.0;
        for (auto &rtt : rtt_values) {
            variance += (rtt - avg_rtt) * (rtt - avg_rtt);
        }
        double stddev = std::sqrt(variance / packets_received);
        double high_latency_threshold = avg_rtt * 2;

        std::cout << "RTT min: " << min_rtt
                  << " ms, max: " << max_rtt
                  << " ms, avg: " << avg_rtt
                  << " ms, stddev: " << stddev << " ms\n";
        std::cout << "High latency count (> " << high_latency_threshold
                  << " ms): " << high_latency_count << "\n";
    }
}

// Receive responses (non-blocking)
void receive_responses(int sockfd) {
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    char buffer[2048];
    pollfd pfd = {sockfd, POLLIN, 0};

    while (running) {
        int ret = poll(&pfd, 1, 10);  // Poll every 10 ms
        if (ret > 0 && (pfd.revents & POLLIN)) {
            int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                          (sockaddr *)&client_addr, &addr_len);

            // Ensure the packet contains at least seq_number + timestamp
            if (bytes_received >= static_cast<int>(sizeof(uint32_t) + sizeof(int64_t))) {
                auto end = steady_clock::now();
                
                // Safely extract sequence number and timestamp
                uint32_t received_seq;
                int64_t received_timestamp;
                std::memcpy(&received_seq, buffer, sizeof(received_seq));
                std::memcpy(&received_timestamp, buffer + sizeof(received_seq), sizeof(received_timestamp));
                
                auto now_ns = duration_cast<nanoseconds>(end.time_since_epoch()).count();
                auto rtt = (now_ns - received_timestamp) / 1'000'000.0;

                rtt_values.push_back(rtt);
                packets_received++;
                received_packets.insert(received_seq);

                char sender_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);

                double avg_rtt = std::accumulate(rtt_values.begin(), rtt_values.end(), 0.0) / packets_received;
                double high_latency_threshold = avg_rtt * 2;

                std::cout << "Response from " << sender_ip
                          << " - size: " << bytes_received
                          << ", seq: " << received_seq
                          << ", rtt: " << std::fixed << std::setprecision(2) << rtt << " ms";
                if (rtt > high_latency_threshold) {
                    high_latency_count++;
                    std::cout << " [HIGH LATENCY]";
                }
                std::cout << "\n";
            } else if (bytes_received > 0) {
                // Handle malformed packets
                std::cout << "Received malformed packet (size " << bytes_received << " bytes)\n";
            }
        }
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip_or_hostname> [-p port] [-i interval] [-c count] [-d duration] [-s size]\n";
        return 1;
    }

    const char *server_host = argv[1];

    // Lambda to check if the next argument exists and optionally validate as a number
    auto has_next_arg = [&](int current, int total, const std::string &arg_name, bool check_number = false) -> bool {
        if ((current + 1) < total) {
            if (check_number) {
                char* endptr = nullptr;
                std::strtod(argv[current + 1], &endptr);
                if (endptr == nullptr || *endptr != '\0') {
                    std::cerr << "Error: Invalid value for " << arg_name << " (expected a number).\n";
                    return false;
                }
            }
            return true;
        }
        std::cerr << "Error: Missing value for " << arg_name << ".\n";
        return false;
    };

    // Parse optional arguments
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-p") {
            if (!has_next_arg(i, argc, "-p", true)) {
                return 1;
            }
            port = std::stoi(argv[++i]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Invalid port number. Valid range is 1-65535.\n";
                return 1;
            }
        }
        else if (arg == "-i") {
            if (!has_next_arg(i, argc, "-i", true)) {
                return 1;
            }
            interval_s = std::stod(argv[++i]);
            if (interval_s < 0.001) {
                std::cerr << "Error: Interval (-i) cannot be below 0.001 seconds.\n";
                return 1;
            }
        } 
        else if (arg == "-c") {
            if (!has_next_arg(i, argc, "-c", true)) {
                return 1;
            }
            max_count = std::stoi(argv[++i]);
        } 
        else if (arg == "-d") {
            if (!has_next_arg(i, argc, "-d", true)) {
                return 1;
            }
            max_duration = std::stoi(argv[++i]);
        } 
        else if (arg == "-s") {
            if (!has_next_arg(i, argc, "-s", true)) {
                return 1;
            }
            payload_size = std::stoi(argv[++i]);
        } 
        else {
            std::cerr << "Error: Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    int sockfd;
    sockaddr_in server_addr{};
    addrinfo hints{}, *res;
    socklen_t addr_len = sizeof(server_addr);
    uint32_t seq_number = 0;

    std::signal(SIGINT, handle_signal);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(server_host, nullptr, &hints, &res) != 0) {
        perror("Hostname resolution failed");
        return 1;
    }

    server_addr = *((sockaddr_in *)res->ai_addr);
    server_addr.sin_port = htons(port);  // Use the specified port
    freeaddrinfo(res);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    start_time = steady_clock::now();

    // Start receiver thread for non-blocking response handling
    std::thread receiver_thread(receive_responses, sockfd);

    while (running) {
        auto now = steady_clock::now();

        // Stop if max_duration is reached
        if (max_duration != UNLIMITED &&
            duration_cast<seconds>(now - start_time).count() >= max_duration) {
            break;
        }

        // Stop if max_count is reached
        if (max_count != UNLIMITED && packets_sent >= static_cast<uint32_t>(max_count)) {
            break;
        }

        // Prepare packet with sequence number and timestamp
        char buffer[payload_size];
        memset(buffer, 0, payload_size);
        auto timestamp = duration_cast<nanoseconds>(now.time_since_epoch()).count();

        memcpy(buffer, &seq_number, sizeof(seq_number));
        memcpy(buffer + sizeof(seq_number), &timestamp, sizeof(timestamp));

        // Send packet to server
        sendto(sockfd, buffer, payload_size, 0, (const sockaddr *)&server_addr, addr_len);
        packets_sent++;
        sent_packets.insert(seq_number);

        seq_number++;

        // Wait for the interval before sending the next packet
        std::this_thread::sleep_for(duration<double>(interval_s));
    }

    // Signal receiver thread to stop and wait for it to finish
    running = false;
    if (receiver_thread.joinable()) {
        receiver_thread.join();
    }

    // Print statistics on exit
    print_statistics();
    
    // Close socket
    close(sockfd);
    return 0;
}
