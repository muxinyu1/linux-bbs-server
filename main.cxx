#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <boost/beast.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <memory>
#include <sstream>

#define PORT 40535
#define MAX_HTTP_LEN 2048

void* handle_connection(void* arg) {
  const int new_socket_fd = (long long)arg;
  namespace beast = boost::beast;
  namespace http = beast::http;
  using u8 = unsigned char;
  const auto buffer = std::make_unique<u8[]>(MAX_HTTP_LEN);
  std::fill(buffer.get(), buffer.get() + MAX_HTTP_LEN, (u8)0);
  while (true) {
    const size_t bytes_recieved =
        recv(new_socket_fd, buffer.get(), MAX_HTTP_LEN, 0);
    // 客户端关闭了tcp连接
    if (bytes_recieved == 0) {
      pthread_exit(NULL);
    }
    http::request_parser<http::string_body> parser{};
    beast::error_code ec;
    parser.put({buffer.get(), bytes_recieved}, ec);
    if (parser.is_done()) {
      auto req = parser.get();
      std::cout << "HTTP Method: " << req.method_string() << std::endl;
      std::cout << "Request Target: " << req.target() << std::endl;
      std::cout << "HTTP Version: " << req.version() << std::endl;

      // 打印HTTP头部字段
      for (const auto& field : req.base()) {
        std::cout << field.name_string() << ": " << field.value() << std::endl;
      }

      // 打印HTTP消息体
      std::cout << "Message Body: " << req.body() << std::endl;

      // 返回http response
      const std::string body{"Hello from Boost & Linux Socket!\n"};
      http::response<http::string_body> response{};
      response.version(req.version());
      response.result(http::status::ok);
      response.set(http::field::server, "MXY SERVER");
      response.set(http::field::content_length, std::to_string(body.size()));
      response.set(http::field::content_type, "text/html");
      response.keep_alive(true);
      
      const std::string headers = boost::lexical_cast<std::string>(response.base());
      
      const auto response_str = headers + body;
      write(new_socket_fd, response_str.c_str(), response_str.size());
    } else {
      std::cerr << "Failed to parse HTTP request" << std::endl;
    }
    std::fill(buffer.get(), buffer.get() + MAX_HTTP_LEN, (u8)0);
  }
}

int main(int argc, char const* argv[]) {
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

  if (bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
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
        accept(socket_fd, (sockaddr*)&client_addr, (socklen_t*)&addr_len);
    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_connection,
                       (void*)new_socket_fd) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}
