#include "http_server.hpp"

int main() {
  TorrentManager torrent_manager;
  HttpServer server(torrent_manager);
  server.start();
  return 0;
}