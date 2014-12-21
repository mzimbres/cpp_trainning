#pragma once

#include <array>
#include <utility>
#include <exception>

#include <memory/node_stack.hpp>

#include "align.hpp"

namespace rt {

template < typename T
         , std::size_t S = size_of<T>::size
         , bool B = !(S < size_of<char*>::size)
         >
class allocator {
  public:
  std::size_t m_size;
  char* m_data;
  public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  template<typename U>
  struct rebind {
    typedef allocator< U
                     , size_of<U>::size
                     , !(size_of<U>::size < size_of<char*>::size)> other;
  };
  bool operator==(const allocator& alloc) const {return m_data == alloc.m_data;}
  bool operator!=(const allocator& alloc) const {return !(*this == alloc);}
  void swap(allocator& other) noexcept
  {
    std::swap(m_size, other.m_size);
    std::swap(m_data, other.m_data);
  }
  void init(char* data, std::size_t size)
  {
    m_size = size;
    m_data = data;
    // aligns the pointer on a word boundary.
    const std::size_t a = reinterpret_cast<std::size_t>(m_data);
    const std::size_t b = sizeof (char*); // word size.
    const std::size_t c = is_aligned(a, b) ? a : next_aligned(a, b);
    m_data = reinterpret_cast<char*>(c);
    size = c - a;
  }
  allocator(char* data, std::size_t size) { init(data, size); }
  template <std::size_t N>
  explicit allocator(std::array<char, N>& arr)
  {
    init(&arr.front(), arr.size());
  }
  explicit allocator(std::vector<char>& arr)
  {
    init(&arr.front(), arr.size());
  }
};

template <typename T, std::size_t N>
class allocator<T, N, true> {
  public:
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  template<class U>
  struct rebind {
    typedef allocator< U
                     , size_of<U>::size
                     , !(size_of<U>::size < size_of<char*>::size)> other;
  };
  private:
  node_stack<size_of<value_type>::size> m_stack;
  public:
  template<typename U>
  allocator(const allocator<U, size_of<U>::size, false>& alloc)
  : m_stack(alloc.m_data, alloc.m_size)
  {}
  template<typename U>
  allocator(const allocator<U, size_of<U>::size, true>& alloc)
  : m_stack(alloc.m_stack)
  {}
  pointer allocate(size_type)
  {
    char* p = m_stack.pop(); 
    if (!p)
      throw std::bad_alloc();
    return reinterpret_cast<pointer>(p); 
  }
  void deallocate(pointer p, size_type) { m_stack.push(reinterpret_cast<char*>(p)); }
  template<typename U>
  void destroy(U* p) {p->~U();}
  template< typename U, typename... Args>
  void construct(U* p, Args&&... args) {::new((void *)p) U(std::forward<Args>(args)...);}
  bool operator==(const allocator& alloc) const {return m_stack == alloc.m_stack;}
  bool operator!=(const allocator& alloc) const {return !(*this == alloc);}
  void swap(allocator& other) noexcept
  {
    m_stack.swap(other.m_stack);
  }
};

}

namespace std {

template <typename T>
void swap(rt::allocator<T>& s1, rt::allocator<T>& s2)
{
  s1.swap(s2);
}

}

