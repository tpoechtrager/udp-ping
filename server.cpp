#include <iostream>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_PORT 7777

std::vector<std::string> allowed_subnets;

// Function to check if the client's IP is allowed
bool is_allowed_ip(const std::string &ip) {
    if (allowed_subnets.empty()) {
        return true;  // Allow all if no subnets are specified
    }
    for (const auto &subnet : allowed_subnets) {
        if (ip.rfind(subnet, 0) == 0) {
            return true;
        }
    }
    return false;
}

// Parse command-line arguments for port and allowed subnets
void parse_arguments(int argc, char *argv[], int &port) {
    port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-p" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "-a" && i + 1 < argc) {
            allowed_subnets.push_back(argv[++i]);
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            exit(1);
        }
    }
}

int main(int argc, char *argv[]) {
    int port;
    parse_arguments(argc, argv, port);

    int sockfd;
    sockaddr_in server_addr{}, client_addr{};
    char buffer[1024];
    socklen_t addr_len = sizeof(client_addr);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Set up server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket to port
    if (bind(sockfd, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    std::cout << "Server listening on port " << port << "...\n";

    // Main server loop
    while (true) {
        memset(buffer, 0, sizeof(buffer));

        // Receive UDP packet
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                      (sockaddr *)&client_addr, &addr_len);
        if (bytes_received < 0) {
            perror("Receive failed");
            continue;
        }

        // Get client IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        // Check if client IP is allowed
        if (is_allowed_ip(client_ip)) {
            // Echo the packet back to the sender
            sendto(sockfd, buffer, bytes_received, 0, (sockaddr *)&client_addr, addr_len);
        } else {
            std::cout << "Rejected packet from unauthorized IP: " << client_ip << "\n";
        }
    }

    // Close socket (this line will not normally be reached)
    close(sockfd);
    return 0;
}
