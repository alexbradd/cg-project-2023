#pragma once

#include <seng/log.hpp>

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace seng {

// Forward declares to avoid conflicts
template <typename... CallbackArgs>
class Hook;
template <typename... CallbackArgs>
class HookRegistrar;
template <typename... CallbackArgs>
class HookToken;

/**
 * A generic hook.
 *
 * A hook stores a list of callbacks in a registrar (see `HookRegistrar`) and
 * can be called using the function-call operator, causing the dispatch of all
 * registered callbacks.
 *
 * Hook callbacks can be any callable that takes as input `CallbackArgs` and
 * returns void.
 *
 * A hook is not copyable nor movable.
 */
template <typename... CallbackArgs>
class Hook {
 public:
  /// Typedef for the function type of the hook
  using HookFunc = std::function<void(CallbackArgs...)>;

  Hook() = default;
  Hook(const Hook&) = delete;
  Hook(Hook&&) = delete;

  Hook& operator=(const Hook&) = delete;
  Hook& operator=(Hook&&) = delete;

  /// Access the registrar
  HookRegistrar<CallbackArgs...>& registrar() { return m_registrar; }

  /// Invoke all callbacks associated to this hook
  void operator()(CallbackArgs... args) const
  {
    for (const auto& cb : m_registrar.callbacks()) std::get<1>(cb)(args...);
  }

 private:
  HookRegistrar<CallbackArgs...> m_registrar;
};

/**
 * Allows registration to a Hook of the same type parameters.
 *
 * It not copyable nor movable;
 */
template <typename... CallbackArgs>
class HookRegistrar {
 public:
  /// Covenience typedef
  using CallbackRegisterType =
      std::unordered_map<uint64_t, typename Hook<CallbackArgs...>::HookFunc>;

  /// Covenience typedef
  using CallbackType = typename CallbackRegisterType::value_type;

  HookRegistrar() = default;
  HookRegistrar(const HookRegistrar&) = delete;
  HookRegistrar(HookRegistrar&&) = delete;

  HookRegistrar& operator=(const HookRegistrar&) = delete;
  HookRegistrar& operator=(HookRegistrar&&) = delete;

  /// All callbacks registered with this registrar
  const CallbackRegisterType callbacks() const { return m_callbacks; }

  /**
   * Register a new callback. The returned token can be used to access this
   * specific callback.
   */
  HookToken<CallbackArgs...> insert(typename Hook<CallbackArgs...>::HookFunc callback)
  {
    m_callbacks.emplace(m_index, callback);
    HookToken<CallbackArgs...> tok{this, m_index};
    m_index++;
    return tok;
  }

  /**
   * Replace the callback identified by the given token with a new callback
   */
  void replace(const HookToken<CallbackArgs...>& token,
               typename Hook<CallbackArgs...>::HookFunc callback)
  {
    if (!token.from(*this)) {
      seng::log::error("Token from another registrar, ignoring... Something is wrong");
      return;
    }
    auto it = m_callbacks.find(token.m_id);
    if (it != m_callbacks.end()) {
      it->second = callback;
    }
  }

  /**
   * Delete the callback identified by the given content. After removal the
   * token is cleared.
   */
  void remove(HookToken<CallbackArgs...>& token)
  {
    if (!token.from(*this)) {
      seng::log::error("Token from another registrar, ignoring... Something is wrong");
      return;
    }
    m_callbacks.erase(token.m_id);
    token.clear();
  }

 private:
  std::uint64_t m_index;
  CallbackRegisterType m_callbacks;
};

/**
 * Token returned by a `HookRegistrar`. Identifies uniquely a callback registered
 * in that registrar.
 */
template <typename... CallbackArgs>
class HookToken {
 public:
  HookToken() = default;
  HookToken(const HookToken&) = default;
  HookToken(HookToken&&) = default;

  HookToken& operator=(const HookToken&) = default;
  HookToken& operator=(HookToken&&) = default;

 private:
  HookToken(const HookRegistrar<CallbackArgs...>* registrar, uint64_t id) :
      m_registrar(registrar), m_id(id)
  {
  }

  bool from(const HookRegistrar<CallbackArgs...>& other) const
  {
    return m_registrar == std::addressof(other);
  }

  void clear() { m_registrar = nullptr; }

  const HookRegistrar<CallbackArgs...>* m_registrar;
  uint64_t m_id;

  friend class HookRegistrar<CallbackArgs...>;
};

}  // namespace seng
