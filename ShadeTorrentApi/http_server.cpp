#include "http_server.hpp"

#include <vector>

HttpServer::HttpServer(TorrentManager &manager) : torrent_manager(manager) {
  configure_cors();
  setup_routes();
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

void HttpServer::start() { app.port(8080).multithreaded().run(); }
