#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

#pragma pack(push, 1)
struct IcmpHeader {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
};
#pragma pack(pop)

static uint16_t checksum16(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2) {
        sum += (static_cast<uint16_t>(data[i]) << 8) | data[i + 1];
    }
    if (len % 2 == 1) sum += static_cast<uint16_t>(data[len - 1]) << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: ttlprobe <host> [max-hops]" << std::endl;
        return 1;
    }
    std::string target = argv[1];
    int max_hops = argc > 2 ? std::stoi(argv[2]) : 30;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    addrinfo* res = nullptr;
    if (getaddrinfo(target.c_str(), nullptr, &hints, &res) != 0 || !res) {
        std::cerr << "cant resolve " << target << std::endl;
        return 1;
    }
    sockaddr_in dest = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
    freeaddrinfo(res);

    char dest_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest.sin_addr, dest_ip, sizeof(dest_ip));
    std::cout << "probing " << target << " (" << dest_ip << "), " << max_hops << " hops max" << std::endl;

    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "raw socket failed, run as administrator" << std::endl;
        WSACleanup();
        return 1;
    }

    DWORD timeout = 1500;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    for (int ttl = 1; ttl <= max_hops; ++ttl) {
        setsockopt(sock, IPPROTO_IP, IP_TTL, reinterpret_cast<const char*>(&ttl), sizeof(ttl));

        uint8_t packet[64] = {};
        IcmpHeader* icmp = reinterpret_cast<IcmpHeader*>(packet);
        icmp->type = 8;
        icmp->code = 0;
        icmp->id = static_cast<uint16_t>(GetCurrentProcessId());
        icmp->seq = static_cast<uint16_t>(ttl);
        icmp->checksum = 0;
        icmp->checksum = htons(checksum16(packet, sizeof(packet)));

        auto start = std::chrono::steady_clock::now();
        sendto(sock, reinterpret_cast<char*>(packet), sizeof(packet), 0,
            reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

        uint8_t reply[512];
        sockaddr_in from{};
        int from_len = sizeof(from);
        int n = recvfrom(sock, reinterpret_cast<char*>(reply), sizeof(reply), 0,
            reinterpret_cast<sockaddr*>(&from), &from_len);

        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        std::cout << ttl << "\t";
        if (n <= 0) {
            std::cout << "*" << std::endl;
            continue;
        }

        char from_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, from_ip, sizeof(from_ip));
        std::cout << from_ip << "\t" << ms << " ms" << std::endl;

        int ip_header_len = (reply[0] & 0x0F) * 4;
        if (n > ip_header_len) {
            uint8_t icmp_type = reply[ip_header_len];
            if (icmp_type == 0 || from.sin_addr.S_un.S_addr == dest.sin_addr.S_un.S_addr) {
                std::cout << "reached destination" << std::endl;
                break;
            }
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
