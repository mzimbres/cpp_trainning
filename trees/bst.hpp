#include <vector>
#include <utility>
#include <iterator>
#include <functional>
#include <algorithm>

#include "bst_iterator.hpp"
#include "node_pool.hpp"

namespace rtcpp {

template < typename T
         , typename Compare = std::less<T>
         , typename Allocator = node_pool<T>>
class bst { // Unbalanced binary search tree
  public:
  typedef T key_type;
  typedef bst_iterator<T> const_iterator;
  typedef const_iterator iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  private:
  typedef node<T> node_type;
  typedef node_type* node_pointer;
  typedef const node_type* const_node_pointer;
  typedef std::vector<node_type> pool_type;
  Allocator* pool;
  node_type head;
  Compare comp;
  public:
  typedef typename pool_type::size_type size_type;
  bst(Allocator& alloc);
  bst(const bst& rhs);
  bst& operator=(const bst& rhs);
  template <typename InputIt>
  bst(InputIt begin, InputIt end, Allocator& alloc);
  ~bst();
  void copy(bst& rhs) const;
  void clear();
  std::pair<iterator, bool> insert(T key);
  const_iterator begin() const {return const_iterator(inorder_successor(&head));}
  const_iterator end() const {return const_iterator(&head);}
  const_reverse_iterator rbegin() const {return const_reverse_iterator(end());}
  const_reverse_iterator rend() const {return const_reverse_iterator(begin());}
};

template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>& bst<T, Compare, Allocator>::operator=(const bst<T, Compare, Allocator>& rhs)
{
  // This ctor can fail if the allocator runs out of memory.
  rhs.copy(*this);
  return *this;
}

template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>::bst(const bst<T, Compare, Allocator>& rhs)
{
  // This ctor can fail if the allocator runs out of memory.
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
  rhs.copy(*this);
}


template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>::bst(Allocator& alloc)
: pool(&alloc)
{
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
}

template <typename T, typename Compare, typename Allocator>
template <typename InputIt>
bst<T, Compare, Allocator>::bst(InputIt begin, InputIt end, Allocator& alloc)
: pool(&alloc)
{
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
  auto func = [this](const T& tmp) -> void {
    auto pair = this->insert(tmp);
    if (pair.second)
      return;
  };
  std::for_each(begin, end, func);
}

template <typename T, typename Compare, typename Allocator>
void bst<T, Compare, Allocator>::clear()
{
  node_pointer p = &head;
  for (;;) {
    node_pointer q = inorder_successor(p);
    if (p != &head)
      pool->deallocate(p);
    if (q == &head)
      break;
    p = q;
  }
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
}

template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>::~bst()
{
  clear();
}

template <typename T, typename Compare, typename Allocator>
void bst<T, Compare, Allocator>::copy(bst<T, Compare, Allocator>& rhs) const
{
  if (this == &rhs)
    return;

  rhs.clear();
  rhs.pool = pool;

  const_node_pointer p = &head;
  node_pointer q = &rhs.head;

  for (;;) {
    if (!has_null_llink(p->tag)) {
      node_pointer tmp = rhs.pool->allocate();
      if (!tmp)
        break; // The tree has exhausted its capacity.

      attach_node_left(q, tmp);
    }

    p = preorder_successor(p);
    q = preorder_successor(q);

    if (p == &head)
      break;

    if (!has_null_rlink(p->tag)) {
      node_pointer tmp = rhs.pool->allocate();
      if (!tmp)
        break; // The tree has exhausted its capacity.

      attach_node_right(q, tmp);
    }

    q->key = p->key;
  }
}

template <typename T, typename Compare, typename Allocator>
std::pair<typename bst<T, Compare, Allocator>::iterator, bool>
bst<T, Compare, Allocator>::insert(T key)
{
  typedef typename bst<T>::const_iterator const_iterator;
  if (has_null_llink(head.tag)) { // The tree is empty
    node_pointer q = pool->allocate();
    if (!q)
      return std::make_pair(const_iterator(), false); // The tree has exhausted its capacity.

    attach_node_left(&head, q);
    q->key = key;
    return std::make_pair(const_iterator(q), true);
  }

  node_pointer p = head.llink;
  for (;;) {
    if (comp(key, p->key)) {
      if (!has_null_llink(p->tag)) {
        p = p->llink;
        continue;
      }
      node_pointer q = pool->allocate();
      if (!q)
        return std::make_pair(const_iterator(), false); // The tree has exhausted its capacity.

      attach_node_left(p, q);
      q->key = key;
      return std::make_pair(q, true);
    } else if (comp(p->key, key)) {
      if (!has_null_rlink(p->tag)) {
        p = p->rlink;
        continue;
      }
      node_pointer q = pool->allocate();
      if (!q)
        return std::make_pair(const_iterator(), false); // The tree has exhausted its capacity.

      attach_node_right(p, q);
      q->key = key;
      return std::make_pair(q, true);
    } else {
      return std::make_pair(p, false);
    }
  }
}

}
