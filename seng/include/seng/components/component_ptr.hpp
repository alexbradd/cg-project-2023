#pragma once

#include <seng/components/base_component.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace seng {

/**
 * Checks if the given type is a subclass of BaseComponent
 */
template <typename T>
using IfComponent = std::enable_if<
    std::is_base_of<BaseComponent, typename std::remove_reference<T>::type>::value>;

/**
 * Thin wrapper around std::unique_ptr<BaseComponent> that provides handy
 * casting methods.
 *
 * For convenience, unique_ptr<BaseCompoenent> is trivially convertible to it.
 */
class ComponentPtr {
 public:
  ComponentPtr() : m_ptr(nullptr) {}
  ComponentPtr(std::unique_ptr<BaseComponent> &&ptr) : m_ptr(std::move(ptr)) {}
  ComponentPtr(std::nullptr_t) : ComponentPtr() {}

  ComponentPtr(const ComponentPtr &) = delete;
  ComponentPtr(ComponentPtr &&other) noexcept = default;

  ComponentPtr &operator=(const ComponentPtr &) = delete;
  ComponentPtr &operator=(ComponentPtr &&) noexcept = default;
  ComponentPtr &operator=(std::nullptr_t) noexcept
  {
    m_ptr = nullptr;
    return *this;
  }

  friend bool operator==(const ComponentPtr &lhs, const ComponentPtr &rhs)
  {
    return lhs.m_ptr == rhs.m_ptr;
  }
  friend bool operator==(const ComponentPtr &lhs, std::nullptr_t)
  {
    return lhs.m_ptr == nullptr;
  }
  friend bool operator==(std::nullptr_t, const ComponentPtr &rhs)
  {
    return nullptr == rhs.m_ptr;
  }

  friend bool operator!=(const ComponentPtr &lhs, const ComponentPtr &rhs)
  {
    return !(lhs == rhs);
  }
  friend bool operator!=(const ComponentPtr &lhs, std::nullptr_t)
  {
    return !(lhs == nullptr);
  }
  friend bool operator!=(std::nullptr_t, const ComponentPtr &rhs)
  {
    return !(nullptr == rhs);
  }

  explicit operator bool() const noexcept { return m_ptr.get() != nullptr; }

  BaseComponent &operator*() const noexcept { return *m_ptr; }
  BaseComponent *operator->() const noexcept { return m_ptr.get(); }

  std::unique_ptr<BaseComponent> &&release() noexcept { return std::move(m_ptr); }
  void rebind(std::unique_ptr<BaseComponent> &&p) noexcept { m_ptr = std::move(p); }
  void swap(ComponentPtr &other) noexcept { m_ptr.swap(other.m_ptr); }

  /**
   * Return the naked pointer stored.
   */
  BaseComponent *get() const noexcept { return m_ptr.get(); }

  /**
   * Return a casted pointer to the concrete type you are **sure** that the pointer
   * has.
   *
   * Under the hood this is basically a static_cast, so you NEED to be sure, otherwise
   * it borks.
   */
  template <typename Concrete, typename = IfComponent<Concrete>>
  Concrete *sureGet() const noexcept
  {
    return static_cast<Concrete *>(m_ptr.get());
  }

  /**
   * Return a caster pointer to the concrete type. If casting fails, return
   * nullptr.
   *
   * Under the hood it does a dynamic_cast, so it is slower than `sureGet()`,
   * however it is safer
   */
  template <typename Concrete, typename = IfComponent<Concrete>>
  Concrete *maybeGet() const
  {
    return dynamic_cast<Concrete>(m_ptr.get());
  }

 private:
  std::unique_ptr<BaseComponent> m_ptr;
};

};  // namespace seng
