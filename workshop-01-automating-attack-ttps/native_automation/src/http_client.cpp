#include "http_client.hpp"

#include <curl/curl.h>
#include <sstream>

namespace teamtnt {

namespace {

size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* out = static_cast<std::string*>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}

HttpResponse perform(const std::string& method, const std::string& url,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers) {
  HttpResponse resp;
  CURL* curl = curl_easy_init();
  if (!curl) {
    resp.error = "curl_easy_init failed";
    return resp;
  }

  std::string header_list;
  curl_slist* hdrs = nullptr;
  for (const auto& [k, v] : headers) {
    header_list = k + ": " + v;
    hdrs = curl_slist_append(hdrs, header_list.c_str());
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "TeamTNT/44con-native");

  if (method == "POST") {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
  } else if (method == "DELETE") {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  }

  if (hdrs) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

  const CURLcode rc = curl_easy_perform(curl);
  if (rc != CURLE_OK) {
    resp.error = curl_easy_strerror(rc);
  } else {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status);
  }

  curl_slist_free_all(hdrs);
  curl_easy_cleanup(curl);
  return resp;
}

}  // namespace

HttpResponse HttpClient::get(const std::string& url,
                             const std::map<std::string, std::string>& headers) const {
  return perform("GET", url, "", headers);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                              const std::map<std::string, std::string>& headers) const {
  return perform("POST", url, body, headers);
}

HttpResponse HttpClient::post_empty(const std::string& url,
                                    const std::map<std::string, std::string>& headers) const {
  return perform("POST", url, "", headers);
}

HttpResponse HttpClient::del(const std::string& url,
                             const std::map<std::string, std::string>& headers) const {
  return perform("DELETE", url, "", headers);
}

}  // namespace teamtnt
