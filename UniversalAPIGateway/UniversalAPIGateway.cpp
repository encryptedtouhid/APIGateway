#include <crow.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

json forward_request(const std::string& url, const json& request_json)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json.dump().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
            return json::parse("{\"error\":\"Request failed\"}");
    }
    return json::parse(readBuffer);
}

int main()
{
    crow::SimpleApp app;

    // Define the routing table
    std::unordered_map<std::string, std::string> routing_table = {
        {"/api/service1", "http://backend_service1/api/endpoint"},
        {"/api/service2", "http://backend_service2/api/endpoint"}
    };

    CROW_ROUTE(app, "/api/<path>")
        ([&routing_table](const crow::request& req, const std::string& path) {
        auto iter = routing_table.find("/api/" + path);
        if (iter != routing_table.end()) {
            std::string backend_url = iter->second;
            json request_json = json::parse(req.body);
            json response_json = forward_request(backend_url, request_json);
            return crow::response(response_json.dump());
        }
        else {
            return crow::response(404, "Not Found");
        }
            });

    app.port(8080).multithreaded().run();
}
