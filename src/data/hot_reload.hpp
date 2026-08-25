// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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

/// What a directory holds, by name and by stamp.
using directory_listing = std::vector<std::pair<std::string, file_stamp>>;

/// Reads what a directory holds, or nothing when it cannot be reached.
using directory_reader = std::function<directory_listing(const std::filesystem::path&)>;

/// Lists the `.lua` files of a directory, sorted by name.
///
/// Sorted rather than left to the filesystem: the order decides nothing today,
/// and the day it decides something, an unordered walk would make two machines
/// disagree about a world drawn from the same seed.
directory_listing disk_listing(const std::filesystem::path& directory);

/// Watches a directory of data files rather than one file.
///
/// A file appearing or disappearing is a change like a file being rewritten:
/// dropping a biome in beside the others has to take effect, which is the
/// whole point of one file per biome.
class directory_watch
{
public:
  directory_watch() = default;

  explicit directory_watch(std::filesystem::path directory, float interval = 1.0f,
                           directory_reader read = disk_listing);

  bool poll(float dt);

  const std::filesystem::path& path() const { return m_path; }

private:
  std::filesystem::path m_path;
  directory_reader m_read;
  float m_interval = 1.0f;
  float m_elapsed = 0.0f;
  directory_listing m_seen;
};

} // namespace arpg
