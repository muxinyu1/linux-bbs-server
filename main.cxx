#include <algorithm>
#include <arpa/inet.h>
#include <cstddef>
#include <limits.h>
#include <netinet/in.h>
#include <ostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#define PORT 8082
#define MAX_HTTP_LEN 2048

void *handle_connection(void *arg) {
  const int new_socket_fd = (long long)arg;
  auto buffer = std::string(MAX_HTTP_LEN, 0);
  while (true) {
    const size_t bytes_recieved =
        recv(new_socket_fd, buffer.data(), MAX_HTTP_LEN, 0);
    // 客户端关闭了tcp连接
    std::cout << "recv: " << buffer.substr(0, bytes_recieved) << std::endl;
    if (bytes_recieved == 0) {
      std::cout << "closed" << std::endl;
      pthread_exit(NULL);
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "8.130.13.195", &(addr.sin_addr));
    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) == -1) {
      perror("connect");
      return nullptr;
    }
    
    send(fd, buffer.data(), bytes_recieved, 0);

    std::cout << "write to data-server: " << buffer.substr(0, bytes_recieved) << std::endl;

    std::fill(buffer.begin(), buffer.end(), 0);
    const size_t bytes_from_data_server = recv(fd, buffer.data(), MAX_HTTP_LEN, 0);

    std::cout << "recv from data-server: " << buffer.substr(0, bytes_from_data_server) << std::endl;

    // write(new_socket_fd, buffer.data(), bytes_from_data_server);
    send(new_socket_fd, buffer.data(), bytes_from_data_server, 0);
    std::cout << "write back: " << buffer.substr(0, bytes_from_data_server) << std::endl;
    shutdown(fd, SHUT_WR);
    shutdown(new_socket_fd, SHUT_WR);
    close(fd);
    close(new_socket_fd);
    std::fill(buffer.begin(), buffer.end(), 0);
  }
}

int main(int argc, char const *argv[]) {
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  const auto socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (bind(socket_fd, (sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(socket_fd, INT_MAX) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  while (true) {
    sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    const auto addr_len = sizeof(client_addr);
    const auto new_socket_fd =
        accept(socket_fd, (sockaddr *)&client_addr, (socklen_t *)&addr_len);
    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_connection,
                       (void *)new_socket_fd) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}
