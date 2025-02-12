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

  crow::App<crow::CORSHandler> app;
  TorrentManager& torrent_manager;
};