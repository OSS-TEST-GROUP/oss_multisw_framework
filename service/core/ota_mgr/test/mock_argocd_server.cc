/**
 * @file mock_argocd_server.cc
 * @brief Mock ArgoCD API server for testing OtamServerService
 *
 * This mock server simulates ArgoCD API endpoints to test OtamServerService
 * without requiring an actual ArgoCD instance.
 */

#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

/**
 * @class MockArgoCDServer
 * @brief Simple HTTP server that mocks ArgoCD API endpoints
 */
class MockArgoCDServer {
 public:
  MockArgoCDServer(int port = 8080) : port_(port), running_(false) {
    std::cout << "=== Mock ArgoCD Server ===" << std::endl;
    std::cout << "Port: " << port_ << std::endl;

    // Initialize mock application data
    InitializeMockData();
  }

  ~MockArgoCDServer() {
    Stop();
  }

  /**
   * @brief Start the HTTP server
   */
  void Start() {
    if (running_) {
      std::cout << "Server already running" << std::endl;
      return;
    }

    running_ = true;
    server_thread_ = std::thread(&MockArgoCDServer::ServerLoop, this);
    std::cout << "Mock ArgoCD server started on port " << port_ << std::endl;
  }

  /**
   * @brief Stop the HTTP server
   */
  void Stop() {
    if (!running_) return;

    running_ = false;
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
    std::cout << "Mock ArgoCD server stopped" << std::endl;
  }

 private:
  int port_;
  std::atomic<bool> running_;
  std::thread server_thread_;
  std::map<std::string, json> applications_;
  std::string auth_token_;
  json session_response_template_;
  json sync_response_template_;

  /**
   * @brief Initialize mock application data from JSON files
   */
  void InitializeMockData() {
    // Load application data from JSON files in mock_data directory
    std::vector<std::string> app_files = {
      "mock_data/sample-app.json",
      "mock_data/test-app.json"
    };

    for (const auto& file : app_files) {
      try {
        std::ifstream ifs(file);
        if (ifs.good()) {
          json app_data;
          ifs >> app_data;
          std::string app_name = app_data["metadata"]["name"].get<std::string>();
          applications_[app_name] = app_data;
          std::cout << "Loaded application: " << app_name << " from " << file << std::endl;
        } else {
          std::cerr << "Warning: Could not open " << file << std::endl;
        }
      } catch (const std::exception& e) {
        std::cerr << "Error loading " << file << ": " << e.what() << std::endl;
      }
    }

    // Load session response template
    try {
      std::ifstream ifs("mock_data/session_response.json");
      if (ifs.good()) {
        ifs >> session_response_template_;
        std::cout << "Loaded session response template" << std::endl;
      }
    } catch (const std::exception& e) {
      std::cerr << "Error loading session response template: " << e.what() << std::endl;
      // Use default template
      session_response_template_["token"] = "mock_token_default";
    }

    // Load sync response template
    try {
      std::ifstream ifs("mock_data/sync_response.json");
      if (ifs.good()) {
        ifs >> sync_response_template_;
        std::cout << "Loaded sync response template" << std::endl;
      }
    } catch (const std::exception& e) {
      std::cerr << "Error loading sync response template: " << e.what() << std::endl;
      // Use default template
      sync_response_template_["status"] = "success";
      sync_response_template_["message"] = "Sync operation started";
    }

    std::cout << "Initialized " << applications_.size() << " mock applications" << std::endl;
  }

  /**
   * @brief Main server loop
   */
  void ServerLoop() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
      std::cerr << "Failed to create socket" << std::endl;
      return;
    }

    // Set socket options
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
      std::cerr << "Failed to bind to port " << port_ << std::endl;
      close(server_fd);
      return;
    }

    if (listen(server_fd, 10) < 0) {
      std::cerr << "Failed to listen on port " << port_ << std::endl;
      close(server_fd);
      return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);

      // Set timeout for accept
      struct timeval tv;
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

      int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
      if (client_fd < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          continue;  // Timeout, check running_ flag
        }
        continue;
      }

      // Handle the request in a separate thread
      std::thread(&MockArgoCDServer::HandleClient, this, client_fd).detach();
    }

    close(server_fd);
  }

  /**
   * @brief Handle a client connection
   */
  void HandleClient(int client_fd) {
    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read <= 0) {
      close(client_fd);
      return;
    }

    std::string request(buffer);
    std::cout << "\n--- Received Request ---" << std::endl;
    std::cout << request.substr(0, request.find("\r\n\r\n")) << std::endl;

    // Parse HTTP request
    HttpRequest req = ParseHttpRequest(request);

    // Generate response
    HttpResponse resp = HandleRequest(req);

    // Send response
    std::string response_str = BuildHttpResponse(resp);
    write(client_fd, response_str.c_str(), response_str.length());

    close(client_fd);
  }

  /**
   * @brief HTTP request structure
   */
  struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
  };

  /**
   * @brief HTTP response structure
   */
  struct HttpResponse {
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;
  };

  /**
   * @brief Parse HTTP request
   */
  HttpRequest ParseHttpRequest(const std::string& request) {
    HttpRequest req;
    std::istringstream stream(request);
    std::string line;

    // Parse request line
    std::getline(stream, line);
    std::istringstream line_stream(line);
    line_stream >> req.method >> req.path;

    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
      size_t colon_pos = line.find(':');
      if (colon_pos != std::string::npos) {
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 2);
        // Remove trailing \r if present
        if (!value.empty() && value.back() == '\r') {
          value.pop_back();
        }
        req.headers[key] = value;
      }
    }

    // Parse body
    std::string body_str;
    while (std::getline(stream, line)) {
      body_str += line;
    }
    req.body = body_str;

    return req;
  }

  /**
   * @brief Handle HTTP request and generate response
   */
  HttpResponse HandleRequest(const HttpRequest& req) {
    HttpResponse resp;
    resp.status_code = 200;
    resp.status_message = "OK";
    resp.headers["Content-Type"] = "application/json";

    std::cout << "Handling: " << req.method << " " << req.path << std::endl;

    try {
      // POST /api/v1/session - Create session/token
      if (req.method == "POST" && req.path == "/api/v1/session") {
        resp.body = HandleCreateSession(req);
      }
      // GET /api/v1/applications/{app_name} - Get application
      else if (req.method == "GET" && req.path.find("/api/v1/applications/") == 0) {
        std::string app_name = ExtractAppName(req.path);
        resp.body = HandleGetApplication(app_name);
      }
      // POST /api/v1/applications/{app_name}/sync - Trigger sync
      else if (req.method == "POST" && req.path.find("/sync") != std::string::npos) {
        std::string app_name = ExtractAppName(req.path);
        resp.body = HandleSyncApplication(app_name);
      }
      // PATCH /api/v1/applications/{app_name} - Update application
      else if (req.method == "PATCH" && req.path.find("/api/v1/applications/") == 0) {
        std::string app_name = ExtractAppName(req.path);
        resp.body = HandlePatchApplication(app_name, req.body);
      }
      // Unknown endpoint
      else {
        resp.status_code = 404;
        resp.status_message = "Not Found";
        json error_json;
        error_json["error"] = "Endpoint not found";
        resp.body = error_json.dump();
      }
    } catch (const std::exception& e) {
      resp.status_code = 500;
      resp.status_message = "Internal Server Error";
      json error_json;
      error_json["error"] = e.what();
      resp.body = error_json.dump();
    }

    std::cout << "Response: " << resp.status_code << " " << resp.status_message << std::endl;
    std::cout << "Body: " << resp.body << std::endl;

    return resp;
  }

  /**
   * @brief Extract application name from path
   */
  std::string ExtractAppName(const std::string& path) {
    // /api/v1/applications/APP_NAME or /api/v1/applications/APP_NAME/sync
    size_t start = path.find("/api/v1/applications/") + 21;
    size_t end = path.find('/', start);
    if (end == std::string::npos) {
      return path.substr(start);
    }
    return path.substr(start, end - start);
  }

  /**
   * @brief Handle POST /api/v1/session
   */
  std::string HandleCreateSession(const HttpRequest& req) {
    json request_json = json::parse(req.body);
    std::string username = request_json.value("username", "");
    std::string password = request_json.value("password", "");

    std::cout << "Create session request: username=" << username << std::endl;

    // Generate mock token
    auth_token_ = "mock_token_" + std::to_string(std::time(nullptr));

    // Use template from JSON file
    json response = session_response_template_;
    response["token"] = auth_token_;
    return response.dump();
  }

  /**
   * @brief Handle GET /api/v1/applications/{app_name}
   */
  std::string HandleGetApplication(const std::string& app_name) {
    auto it = applications_.find(app_name);
    if (it != applications_.end()) {
      return it->second.dump(2);
    }

    // Return 404 if application not found
    json error_response;
    error_response["error"] = "Application not found";
    error_response["code"] = 5;  // NotFound
    return error_response.dump();
  }

  /**
   * @brief Handle POST /api/v1/applications/{app_name}/sync
   */
  std::string HandleSyncApplication(const std::string& app_name) {
    auto it = applications_.find(app_name);
    if (it == applications_.end()) {
      json error_response;
      error_response["error"] = "Application not found";
      return error_response.dump();
    }

    // Update application status to Running
    applications_[app_name]["status"]["operationState"]["phase"] = "Running";

    std::cout << "Triggering sync for " << app_name << std::endl;

    // Use template from JSON file
    json response = sync_response_template_;
    return response.dump();
  }

  /**
   * @brief Handle PATCH /api/v1/applications/{app_name}
   */
  std::string HandlePatchApplication(const std::string& app_name, const std::string& body) {
    auto it = applications_.find(app_name);
    if (it == applications_.end()) {
      json error_response;
      error_response["error"] = "Application not found";
      return error_response.dump();
    }

    try {
      json patch_request = json::parse(body);
      std::string patch_str = patch_request.value("patch", "");

      if (!patch_str.empty()) {
        json patch_json = json::parse(patch_str);

        // Apply patch to application
        if (patch_json.contains("spec")) {
          applications_[app_name]["spec"].update(patch_json["spec"]);
        }

        std::cout << "Patched application " << app_name << std::endl;
      }

      return applications_[app_name].dump(2);
    } catch (const std::exception& e) {
      json error_response;
      error_response["error"] = std::string("Failed to apply patch: ") + e.what();
      return error_response.dump();
    }
  }

  /**
   * @brief Build HTTP response string
   */
  std::string BuildHttpResponse(const HttpResponse& resp) {
    std::ostringstream stream;

    // Status line
    stream << "HTTP/1.1 " << resp.status_code << " " << resp.status_message << "\r\n";

    // Headers
    for (const auto& [key, value] : resp.headers) {
      stream << key << ": " << value << "\r\n";
    }
    stream << "Content-Length: " << resp.body.length() << "\r\n";
    stream << "Connection: close\r\n";
    stream << "\r\n";

    // Body
    stream << resp.body;

    return stream.str();
  }
};

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
  int port = 8080;

  // Parse command line arguments
  if (argc > 1) {
    port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
      std::cerr << "Invalid port number. Using default port 8080." << std::endl;
      port = 8080;
    }
  }

  try {
    MockArgoCDServer server(port);
    server.Start();

    std::cout << "\nPress Ctrl+C to stop the server..." << std::endl;
    std::cout << "\nAvailable endpoints:" << std::endl;
    std::cout << "  POST   /api/v1/session" << std::endl;
    std::cout << "  GET    /api/v1/applications/{app_name}" << std::endl;
    std::cout << "  POST   /api/v1/applications/{app_name}/sync" << std::endl;
    std::cout << "  PATCH  /api/v1/applications/{app_name}" << std::endl;
    std::cout << "\nMock applications: sample-app, test-app" << std::endl;

    // Keep running until Ctrl+C
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
