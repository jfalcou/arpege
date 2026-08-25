// SPDX-License-Identifier: BSL-1.0

#include <data/hot_reload.hpp>

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

} // namespace arpg
