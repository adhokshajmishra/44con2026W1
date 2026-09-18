#pragma once

#include <map>
#include <string>
#include <vector>

namespace teamtnt {

struct HttpResponse {
  long status{0};
  std::string body;
  std::string error;
  bool ok() const { return status >= 200 && status < 300; }
};

class HttpClient {
 public:
  HttpResponse get(const std::string& url,
                   const std::map<std::string, std::string>& headers = {}) const;
  HttpResponse post(const std::string& url, const std::string& body,
                    const std::map<std::string, std::string>& headers = {}) const;
  HttpResponse post_empty(const std::string& url,
                          const std::map<std::string, std::string>& headers = {}) const;
  HttpResponse del(const std::string& url,
                   const std::map<std::string, std::string>& headers = {}) const;
};

}  // namespace teamtnt
