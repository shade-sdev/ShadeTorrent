#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>

#include "torrent_manager.hpp"

class HttpServer {
public:
  explicit HttpServer(TorrentManager& manager);
  void start();

private:
  void setup_routes();
  void configure_cors();
  void setup_websocket();
  void broadcast_status_updates();

  crow::App<crow::CORSHandler> app;
  TorrentManager& torrent_manager;
  std::unordered_map<std::string, crow::websocket::connection*> ws_connections;
  std::mutex ws_mutex;
};