// SPDX-License-Identifier: BSL-1.0

#include <data/hot_reload.hpp>

#include <algorithm>

namespace arpg
{

std::optional<file_stamp> disk_stamp(const std::filesystem::path& path)
{
  std::error_code failure;

  const std::filesystem::file_time_type written = std::filesystem::last_write_time(path, failure);

  if (failure)
  {
    return std::nullopt;
  }

  const std::uintmax_t size = std::filesystem::file_size(path, failure);

  if (failure)
  {
    return std::nullopt;
  }

  return file_stamp{written, size};
}

file_watch::file_watch(std::filesystem::path path, float interval, stamp_reader read)
  : m_path(std::move(path))
  , m_read(std::move(read))
  , m_interval(interval)
  , m_seen(m_read(m_path))
{
}

bool file_watch::poll(float dt)
{
  if (!m_read)
  {
    return false;
  }

  m_elapsed += dt;

  if (m_elapsed < m_interval)
  {
    return false;
  }

  // The interval is taken out rather than the clock zeroed: zeroing throws
  // away whatever overshot it, and a period that is always a fraction of a
  // frame too long drifts further from the asked one the longer it runs.
  m_elapsed -= m_interval;

  const std::optional<file_stamp> now = m_read(m_path);

  // An editor writing the file makes it briefly unreachable. Reporting that
  // as a change would reload a file in the middle of being written, so a
  // missing file is no news and the last version seen is kept.
  if (!now)
  {
    return false;
  }

  if (now == m_seen)
  {
    return false;
  }

  m_seen = now;
  return true;
}

directory_listing disk_listing(const std::filesystem::path& directory)
{
  directory_listing found;
  std::error_code failure;

  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory, failure))
  {
    if (!entry.is_regular_file() || entry.path().extension() != ".lua")
    {
      continue;
    }

    const std::optional<file_stamp> stamp = disk_stamp(entry.path());

    if (stamp)
    {
      found.emplace_back(entry.path().filename().string(), *stamp);
    }
  }

  if (failure)
  {
    return {};
  }

  std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

  return found;
}

directory_watch::directory_watch(std::filesystem::path directory, float interval, directory_reader read)
  : m_path(std::move(directory))
  , m_read(std::move(read))
  , m_interval(interval)
  , m_seen(m_read(m_path))
{
}

bool directory_watch::poll(float dt)
{
  if (!m_read)
  {
    return false;
  }

  m_elapsed += dt;

  if (m_elapsed < m_interval)
  {
    return false;
  }

  m_elapsed -= m_interval;

  const directory_listing now = m_read(m_path);

  // A directory that reads as empty is taken for one that could not be read:
  // reporting it would reload a world with no biome in it, and a directory
  // being written to is briefly unreadable.
  if (now.empty() || now == m_seen)
  {
    return false;
  }

  m_seen = now;
  return true;
}

} // namespace arpg
