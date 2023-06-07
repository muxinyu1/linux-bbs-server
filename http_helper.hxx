#pragma once

#include <algorithm>
#include <boost/beast.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/smart_ptr/make_shared_array.hpp>
#include <cstddef>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mxy {
namespace http = boost::beast::http;
class http_helper {
private:
  static std::pair<std::vector<char>, bool>
  create_body_from_path(const std::string &path) {
    auto file_path = "." + (path == "/" ? "/index.html" : path);
    if (!std::filesystem::exists(file_path)) {
      file_path = "./404.html";
    }
    const auto file_size = std::filesystem::file_size(file_path);
    std::vector<char> buf{};
    buf.reserve(file_size);
    std::ifstream ifs{file_path};
    std::copy(std::istream_iterator<char>(ifs), std::istream_iterator<char>(),
              std::back_inserter(buf));
    return {buf, true};
  }
  static std::vector<char> add(const std::string &headers,
                               const std::vector<char> &body) {
    if (body.empty()) {
      return {headers.begin(), headers.end()};
    } else {
      std::vector<char> headers_vec{headers.begin(), headers.end()};
      headers_vec.reserve(headers_vec.size() + body.size());
      headers_vec.insert(headers_vec.end(), body.begin(), body.end());
      return headers_vec;
    }
  }

public:
  static std::vector<char> make_response(
      decltype(http::request_parser<http::string_body>{}.get()) &request) {
    http::response<http::string_body> response{};
    response.version(request.version());
    response.set(http::field::server, "MXY SERVER");
    response.set(http::field::content_type, "text/html");
    response.keep_alive(true);
    if (request.method_string() == "GET") {
      const auto path = request.target();
      const auto [body, ok] = create_body_from_path(path);
      response.result(ok ? http::status::ok : http::status::not_found);
      response.set(http::field::content_length, std::to_string(body.size()));
      const std::string headers =
          boost::lexical_cast<std::string>(response.base());
      const auto response_vec = add(headers, body);
      return response_vec;
    } else {
      // TODO
      return {};
    }
  }
};
} // namespace mxy