// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace arpg
{

/// Loads a thing once, hands it out by name, and lets go of it on command.
///
/// What is cached is deliberately not what loads it: the cache knows nothing of
/// files, of raylib or of textures, and is handed a function that does. That is
/// what makes it testable without any of them, and what lets the same cache
/// hold sheets, sounds and atlases.
///
/// A pointer handed out stays good for as long as the cache holds the thing,
/// which is why what is held is held behind a unique_ptr: a cache growing must
/// not move what somebody is already pointing at.
template <typename resource>
class resource_cache
{
public:
  /// Fetches @p name, or nothing when it cannot be had.
  using loader = std::function<std::optional<resource>(std::string_view name)>;

  /// Lets go of one, for a backend that has to be told.
  using unloader = std::function<void(resource&)>;

  resource_cache() = default;

  explicit resource_cache(loader load, unloader unload = {})
    : m_load(std::move(load))
    , m_unload(std::move(unload))
  {
  }

  resource_cache(const resource_cache&) = delete;
  resource_cache& operator=(const resource_cache&) = delete;

  resource_cache(resource_cache&&) noexcept = default;
  resource_cache& operator=(resource_cache&&) noexcept = default;

  ~resource_cache() { clear(); }

  /// The thing called @p name, loading it the first time and only the first
  /// time. Null when it cannot be had.
  ///
  /// A name that failed once is not tried again: a sheet that is not there
  /// would otherwise be looked for sixty times a second, and the log would say
  /// so sixty times a second.
  const resource* get(std::string_view name)
  {
    const auto held = m_held.find(std::string{name});

    if (held != m_held.end())
    {
      return held->second.get();
    }

    if (!m_load || m_refused.count(std::string{name}) != 0)
    {
      return nullptr;
    }

    std::optional<resource> fetched = m_load(name);

    if (!fetched)
    {
      m_refused.insert(std::string{name});
      return nullptr;
    }

    auto kept = std::make_unique<resource>(std::move(*fetched));
    const resource* answer = kept.get();
    m_held.emplace(std::string{name}, std::move(kept));

    return answer;
  }

  /// Whether @p name is already held, without going and getting it.
  bool holds(std::string_view name) const { return m_held.count(std::string{name}) != 0; }

  /// How many things are held.
  std::size_t size() const { return m_held.size(); }

  /// How many names were asked for and could not be had.
  std::size_t refused() const { return m_refused.size(); }

  /// Lets go of everything, and forgets what was refused so a file put back
  /// where it belongs is looked for again.
  void clear()
  {
    if (m_unload)
    {
      for (auto& entry : m_held)
      {
        m_unload(*entry.second);
      }
    }

    m_held.clear();
    m_refused.clear();
  }

private:
  loader m_load;
  unloader m_unload;

  // Ordered rather than hashed: what is in here is counted in dozens, and a
  // walk of it has to be the same on two machines for a log to be comparable.
  std::map<std::string, std::unique_ptr<resource>, std::less<>> m_held;
  std::set<std::string, std::less<>> m_refused;
};

} // namespace arpg
