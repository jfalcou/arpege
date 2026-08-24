// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <filesystem>
#include <functional>
#include <optional>

namespace arpg
{

/// What tells one version of a file from another.
///
/// The size goes with the time because a filesystem stores that time coarsely:
/// two saves a few milliseconds apart can carry the same one, and an edit that
/// changes the length is then still seen.
struct file_stamp
{
  std::filesystem::file_time_type written{};
  std::uintmax_t size = 0;

  friend bool operator==(const file_stamp&, const file_stamp&) = default;
};

/// Reads what identifies a file, or nothing when it cannot be reached.
using stamp_reader = std::function<std::optional<file_stamp>(const std::filesystem::path&)>;

/// Reads the stamp from the filesystem. Errors are reported as absence rather
/// than as exceptions, since a file being written to is briefly unreachable.
std::optional<file_stamp> disk_stamp(const std::filesystem::path& path);

/// Watches one file and reports when it has been rewritten.
///
/// The filesystem is asked on an interval rather than every frame: a file
/// changes when a human saves it, and sixty questions a second about that is
/// work for nothing.
class file_watch
{
public:
  file_watch() = default;

  /// Starts from the current state of @p path, so a watch never reports the
  /// file it was handed as already changed.
  explicit file_watch(std::filesystem::path path, float interval = 1.0f, stamp_reader read = disk_stamp);

  /// True on the poll where the file turns out to differ from the last one
  /// seen. Several edits inside one interval read as a single change.
  bool poll(float dt);

  const std::filesystem::path& path() const { return m_path; }

private:
  std::filesystem::path m_path;
  stamp_reader m_read;
  float m_interval = 1.0f;
  float m_elapsed = 0.0f;
  std::optional<file_stamp> m_seen;
};

} // namespace arpg
