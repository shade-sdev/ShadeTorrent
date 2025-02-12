#include "http_server.hpp"

#include <ranges>
#include <vector>

HttpServer::HttpServer(TorrentManager &manager) : torrent_manager(manager) {
  configure_cors();
  setup_routes();
  setup_websocket();
}

void HttpServer::configure_cors() {
  auto &cors = app.get_middleware<crow::CORSHandler>();
  cors.global()
      .headers("*")
      .methods("POST"_method, "GET"_method, "PUT"_method, "DELETE"_method)
      .origin("*");
}

void HttpServer::setup_routes() {
  CROW_ROUTE(app, "/torrents")
      .methods("POST"_method)([this](const crow::request &req) {
        try {
          const std::string &magnet = req.body;
          if (magnet.empty()) {
            return crow::response(400, "Magnet link required");
          }

          const std::string id = torrent_manager.add_torrent(magnet);
          crow::json::wvalue response;
          response["id"] = id;
          return crow::response(200, response);
        } catch (const std::exception &e) {
          crow::json::wvalue response;
          response["error"] = e.what();
          return crow::response(500, response);
        }
      });

  // Get all torrents status
  CROW_ROUTE(app, "/torrents").methods("GET"_method)([this]() {
    return crow::response(200, torrent_manager.get_all_statuses());
  });

  // Delete torrent
  CROW_ROUTE(app, "/torrents/<string>")
      .methods("DELETE"_method)([this](const std::string &id) {
        torrent_manager.remove_torrent(id);
        return crow::response(200);
      });

  // Pause torrent
  CROW_ROUTE(app, "/torrents/<string>/pause")
      .methods("POST"_method)([this](const std::string &id) {
        torrent_manager.set_pause_state(id, true);
        return crow::response(200);
      });

  // Resume torrent
  CROW_ROUTE(app, "/torrents/<string>/resume")
      .methods("POST"_method)([this](const std::string &id) {
        torrent_manager.set_pause_state(id, false);
        return crow::response(200);
      });

  // Get single torrent status
  CROW_ROUTE(app, "/torrents/<string>")
      .methods("GET"_method)([this](const std::string &id) {
        auto status = torrent_manager.get_status(id);
        if (status.size() == 0) return crow::response(404);
        return crow::response(200, status);
      });
}

void HttpServer::setup_websocket() {
  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onaccept([&](const crow::request &req, void **userdata) {
        return true;
      })
      .onopen([&](crow::websocket::connection &conn) {
        std::lock_guard lock(ws_mutex);
        ws_connections[conn.get_remote_ip()] = &conn;
        conn.send_text(torrent_manager.get_all_statuses().dump());
      })
      .onclose([&](crow::websocket::connection &conn,
                   const std::string & /*reason*/) {
        std::lock_guard lock(ws_mutex);
        std::cout << conn.get_remote_ip() << std::endl;
        ws_connections.erase(conn.get_remote_ip());
      })
      .onmessage([&](crow::websocket::connection &conn,
                     const std::string &message, bool is_binary) {
      })
      .onerror([&](crow::websocket::connection &conn,
                   const std::string &error_message) {
        std::lock_guard lock(ws_mutex);
        ws_connections.erase(conn.get_remote_ip());
      });

  std::thread([this]() {
    while (true) {
      std::this_thread::sleep_for(
          std::chrono::seconds(5));
      broadcast_status_updates();
    }
  }).detach();
}

void HttpServer::broadcast_status_updates() {
  std::lock_guard lock(ws_mutex);
  const crow::json::wvalue statuses = torrent_manager.get_all_statuses();
  for (const auto &conn : ws_connections | std::views::values) {
    conn->send_text(statuses.dump());
  }
}

void HttpServer::start() { app.port(8080).multithreaded().run(); }
