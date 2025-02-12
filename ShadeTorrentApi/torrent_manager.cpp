#include "torrent_manager.hpp"

#include <filesystem>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>

namespace fs = std::filesystem;

const char* state_to_string(const lt::torrent_status::state_t s) {
  switch (s) {
    case lt::torrent_status::checking_files:
      return "checking";
    case lt::torrent_status::downloading_metadata:
      return "downloading_metadata";
    case lt::torrent_status::downloading:
      return "downloading";
    case lt::torrent_status::finished:
      return "finished";
    case lt::torrent_status::seeding:
      return "seeding";
    case lt::torrent_status::checking_resume_data:
      return "checking_resume_data";
    default:
      return "unknown";
  }
}

TorrentManager::TorrentManager() {
  m_session.set_alert_notify([this]() { this->handle_alerts(); });
}

TorrentManager::~TorrentManager() { m_session.pause(); }

fs::path get_desktop_path() {
#ifdef _WIN32
  const char* user_profile = std::getenv("USERPROFILE");
  return fs::path(user_profile) / "Desktop";
#elif __APPLE__
  const char* home = std::getenv("HOME");
  return fs::path(home) / "Desktop";
#else  // Linux
  const char* home = std::getenv("HOME");
  return fs::path(home) / "Desktop";
#endif
}

std::string TorrentManager::add_torrent(const std::string& magnet_uri) {
  std::lock_guard lock(m_mutex);
  lt::add_torrent_params params;

  lt::error_code ec;
  parse_magnet_uri(magnet_uri, params, ec);
  if (ec) throw std::runtime_error("Invalid magnet URI: " + ec.message());

  params.save_path = get_desktop_path().string();
  const lt::torrent_handle handle = m_session.add_torrent(params);

  std::string id = "t" + std::to_string(++m_id_counter);
  m_torrents.emplace(
      id, TorrentHandle{handle, params.name.empty() ? "Unknown" : params.name,
                        false});
  return id;
}

void TorrentManager::remove_torrent(const std::string& id) {
  std::lock_guard lock(m_mutex);
  if (const auto it = m_torrents.find(id); it != m_torrents.end()) {
    m_session.remove_torrent(it->second.handle, lt::session::delete_files);
    m_torrents.erase(it);
  }
}

void TorrentManager::set_pause_state(const std::string& id, const bool pause) {
  std::lock_guard lock(m_mutex);
  if (const auto it = m_torrents.find(id); it != m_torrents.end()) {
    if (pause) {
      it->second.handle.pause();
      it->second.paused = true;
    } else {
      it->second.handle.resume();
      it->second.paused = false;
    }
  }
}

crow::json::wvalue TorrentManager::get_status(const std::string& id) const {
  std::lock_guard lock(m_mutex);
  const auto it = m_torrents.find(id);
  if (it == m_torrents.end()) return {};
  return create_status_json(it->second);
}

crow::json::wvalue TorrentManager::get_all_statuses() const {
  std::lock_guard lock(m_mutex);
  crow::json::wvalue result;
  int index = 0;

  for (const auto& [id, th] : m_torrents) {
    auto status = create_status_json(th);
    result[index]["id"] = id;
    result[index]["name"] = std::move(status["name"]);
    result[index]["paused"] = std::move(status["paused"]);
    result[index]["progress"] = std::move(status["progress"]);
    result[index]["download_rate"] = std::move(status["download_rate"]);
    result[index]["upload_rate"] = std::move(status["upload_rate"]);
    result[index]["total_bytes"] = std::move(status["total_bytes"]);
    result[index]["bytes_downloaded"] = std::move(status["bytes_downloaded"]);
    result[index]["state"] = std::move(status["state"]);
    result[index]["absolute_path"] = std::move(status["absolute_path"]);
    index++;
  }
  return result;
}

crow::json::wvalue TorrentManager::create_status_json(const TorrentHandle& th) {
  const lt::torrent_status status = th.handle.status();
  crow::json::wvalue json;
  json["name"] = th.name;
  json["paused"] = th.paused;
  json["progress"] = status.progress;
  json["download_rate"] = status.download_rate;
  json["upload_rate"] = status.upload_rate;
  json["total_bytes"] = status.total_download;
  json["bytes_downloaded"] = status.total_done;
  json["state"] = state_to_string(status.state);
  std::filesystem::path torrent_path = get_desktop_path() / status.name;
  json["absolute_path"] = torrent_path.string();
  return json;
}

void TorrentManager::handle_alerts() {
  std::vector<lt::alert*> alerts;
  m_session.pop_alerts(&alerts);
  // Handle important alerts here if needed
}