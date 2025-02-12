#pragma once

#include <crow/json.h>

#include <libtorrent/session.hpp>
#include <libtorrent/torrent_status.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace lt = libtorrent;

class TorrentManager {
 public:
  struct TorrentHandle {
    lt::torrent_handle handle;
    std::string name;
    bool paused = false;
  };

  TorrentManager();
  ~TorrentManager();

  std::string add_torrent(const std::string& magnet_uri);
  void remove_torrent(const std::string& id);
  void set_pause_state(const std::string& id, bool pause);
  crow::json::wvalue get_status(const std::string& id) const;
  crow::json::wvalue get_all_statuses() const;

 private:
  static crow::json::wvalue create_status_json(const TorrentHandle& th);
  void handle_alerts();

  mutable std::mutex m_mutex;
  lt::session m_session;
  std::unordered_map<std::string, TorrentHandle> m_torrents;
  std::atomic<int> m_id_counter{0};
};