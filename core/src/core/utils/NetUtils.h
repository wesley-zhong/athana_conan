//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef ATHENA_NETUTILS_H
#define ATHENA_NETUTILS_H
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

class NetUtils {
public:
    static std::vector<std::string> getLocalIPs() {
        std::vector<std::string> ips;

#ifdef _WIN32
        // Windows 实现
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return ips;

        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            ADDRINFO hints = {0}, *res = nullptr;
            hints.ai_family = AF_INET; // 仅 IPv4
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(hostname, nullptr, &hints, &res) == 0) {
                for (auto p = res; p != nullptr; p = p->ai_next) {
                    char ipStr[INET_ADDRSTRLEN];
                    auto* ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
                    inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
                    ips.push_back(ipStr);
                }
                freeaddrinfo(res);
            }
        }
        WSACleanup();

#else
        // Linux / macOS 实现
        struct ifaddrs* interfaces = nullptr;
        if (getifaddrs(&interfaces) == 0) {
            struct ifaddrs* temp = interfaces;
            while (temp != nullptr) {
                // 检查是否为 IPv4 地址 且 接口已启动
                if (temp->ifa_addr && temp->ifa_addr->sa_family == AF_INET) {
                    char ipStr[INET_ADDRSTRLEN];
                    auto* addr = reinterpret_cast<struct sockaddr_in*>(temp->ifa_addr);
                    inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN);

                    std::string ip(ipStr);
                    // 过滤掉回环地址 127.0.0.1
                    if (ip != "127.0.0.1") {
                        ips.push_back(ip);
                    }
                }
                temp = temp->ifa_next;
            }
            freeifaddrs(interfaces);
        }
#endif
        return ips;
    }
};
#endif //ATHENA_NETUTILS_H