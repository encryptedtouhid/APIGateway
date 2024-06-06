#include <crow.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>

using json = nlohmann::json;

// Function to handle writing CURL response to a string buffer
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to make HTTP request and fetch response
json forward_request(const std::string& url, const json& request_json, const std::string& method)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json.dump().c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }
        else if (method == "GET") {
            std::string url_with_params = url + "?" + request_json.dump();
            curl_easy_setopt(curl, CURLOPT_URL, url_with_params.c_str());
        }
        else {
            return json({ {"error", "Unsupported HTTP method"} });
        }

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return json({ {"error", "Request failed"} });
        }
    }
    else {
        return json({ {"error", "Failed to initialize CURL"} });
    }

    try {
        return json::parse(readBuffer);
    }
    catch (json::parse_error& e) {
        return json({ {"error", "Failed to parse JSON response from server"} });
    }
}

int main()
{
    crow::SimpleApp app;

    // Define the routing table with method type
    std::unordered_map<std::string, std::pair<std::string, std::string>> routing_table = {
        {"/api/service1", {"http://backend_service1/api/endpoint", "GET"}},
        {"/api/service2", {"http://backend_service2/api/endpoint", "POST"}}
    };

    CROW_ROUTE(app, "/api/<path>")
        ([&routing_table](const crow::request& req, const std::string& path) {
        auto iter = routing_table.find("/api/" + path);
        if (iter != routing_table.end()) {
            std::string backend_url = iter->second.first;
            std::string method = iter->second.second;

            json request_json;
            if (!req.body.empty()) {
                try {
                    request_json = json::parse(req.body);
                }
                catch (json::parse_error& e) {
                    return crow::response(400, e.what());
                }
            }

            json response_json = forward_request(backend_url, request_json, method);
            if (response_json.contains("error")) {
                return crow::response(500, response_json.dump());
            }
            else {
                return crow::response(response_json.dump());
            }
        }
        else {
            return crow::response(404, "Not Found");
        }
            });

    app.port(8080).multithreaded().run();
}
