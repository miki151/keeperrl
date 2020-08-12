#pragma once

#include <vector>
#include <set>
#include <type_traits>
#include "extern/optional.h"
#include "debug.h"

using std::experimental::optional;
using std::experimental::none;

template <typename Iter>
using RequireInputIter = typename
  std::enable_if<std::is_convertible<typename std::iterator_traits<Iter>::iterator_category,
      std::input_iterator_tag>::value>::type;

template <typename T>
class vector {
  public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using size_type = int;

  vector() {}
  vector(initializer_list<value_type> i) : impl(i) {}

  template <typename InputIter, typename = RequireInputIter<InputIter>>
  vector(InputIter begin, InputIter end) : impl(begin, end) {}

  vector(const std::vector<T>& v) noexcept : impl(v) {}
  vector(std::vector<T>&& v) noexcept : impl(std::move(v)) {}

  vector(int size, const T& elem) : impl(size, elem) {}
  vector(int size) : impl(size) {}

  int size() const {
    return int(impl.size());
  }

  int capacity() const {
    return int(impl.capacity());
  }

  void shrink_to_fit() {
    impl.shrink_to_fit();
  }

  bool empty() const {
    return impl.empty();
  }

  void push_back(T t) {
#ifndef RELEASE // due to compile errors on older clang
    static_assert(std::is_nothrow_move_constructible<T>::value, "T should be noexcept MoveConstructible");
#endif
    impl.push_back(std::move(t));
    ++modCounter;
  }

  void push_front(T t) {
    impl.insert(impl.begin(), std::move(t));
    ++modCounter;
  }

  void insert(int index, T t) {
    impl.insert(impl.begin() + index, std::move(t));
    ++modCounter;
  }

  template <typename... Args>
  void emplace_back(Args&&... a) {
    impl.emplace_back(std::forward<Args>(a)...);
    ++modCounter;
  }

  void pop_back() {
    impl.pop_back();
    ++modCounter;
  }

  T& operator[] (int i) {
    CHECK(i >= 0 && i < size()) << "Bad index: " << i << " (vector size " << size() << ")";
    return impl[i];
  }

  const T& operator[] (int i) const {
    CHECK(i >= 0 && i < size()) << "Bad index: " << i << " (vector size " << size() << ")";
    return impl[i];
  }

  T& back() {
    CHECK(!empty());
    return impl.back();
  }

  const T& back() const {
    CHECK(!empty());
    return impl.back();
  }

  T& front() {
    CHECK(!empty());
    return impl.front();
  }

  const T& front() const {
    CHECK(!empty());
    return impl.front();
  }

  void clear() {
    impl.clear();
    ++modCounter;
  }

  void reserve(int s) {
    impl.reserve(s);
    ++modCounter;
  }

  auto data() {
    return impl.data();
  }

  auto data() const {
    return impl.data();
  }

  bool operator == (const vector<T>& o) const {
    return impl == o.impl;
  }

  bool operator != (const vector<T>& o) const {
    return impl != o.impl;
  }

  template <typename V>
  bool contains(const V& elem) const {
    return std::find(impl.begin(), impl.end(), elem) != impl.end();
  }

  template <typename V>
  bool contains(const optional<V>& elem) const {
    return elem && contains(*elem);
  }

  void removeIndex(int index) {
    impl.at(index) = std::move(impl.back());
    impl.pop_back();
    ++modCounter;
  }

  T removeIndexPreserveOrder(int index) {
    auto ret = std::move(impl.at(index));
    impl.erase(impl.begin() + index);
    ++modCounter;
    return ret;
  }

  template <typename Iter>
  void append(Iter begin, const Iter& end) {
    impl.reserve(size() + (end - begin));
    while (begin != end) {
      impl.push_back(*begin);
      ++begin;
    }
    ++modCounter;
  }

  void append(vector<T>&& elems) {
    impl.reserve(size() + elems.size());
    for (auto&& elem : elems)
      impl.push_back(std::move(elem));
    ++modCounter;
  }

  void append(const vector<T>& elems) {
    impl.reserve(size() + elems.size());
    for (auto& elem : elems)
      impl.push_back(elem);
    ++modCounter;
  }

  void erase(int index, int count) {
    impl.erase(impl.begin() + index, impl.begin() + index + count);
  }

  void insert(int index, const vector<T>& v) {
    impl.insert(impl.begin() + index, v.begin(), v.end());
  }

  optional<int> findAddress(const T* ptr) const {
    for (int i = 0; i < size(); ++i)
      if (&impl.at(i) == ptr)
        return i;
    return none;
  }

  optional<int> findElement(const T& element) const {
    for (int i = 0; i < size(); ++i)
      if (impl[i] == element)
        return i;
    return none;
  }

  bool removeElementMaybe(const T& element) {
    if (auto ind = findElement(element)) {
      removeIndex(*ind);
      return true;
    }
    return false;
  }

  bool removeElementMaybePreserveOrder(const T& element) {
    if (auto ind = findElement(element)) {
      removeIndexPreserveOrder(*ind);
      return true;
    }
    return false;
  }

  void removeElement(const T& element) {
    removeIndex(*findElement(element));
  }

  const T& getOnlyElement() const& {
    CHECK(size() == 1) << "Size " << size();
    return impl[0];
  }

  T& getOnlyElement() & {
    CHECK(size() == 1) << "Size " << size();
    return impl[0];
  }

  T getOnlyElement() && {
    CHECK(size() == 1) << "Size " << size();
    return std::move(impl[0]);
  }

  const T* getFirstElement() const& {
    if (!empty())
      return &impl[0];
    else
      return nullptr;
  }

  T* getFirstElement() & {
    if (!empty())
      return &impl[0];
    else
      return nullptr;
  }

  optional<T> getFirstElement() && {
    if (!empty())
      return std::move(impl[0]);
    else
      return none;
  }

  vector<T> reverse() const {
    return vector<T>(impl.rbegin(), impl.rend());
  }

  void resize(int sz) {
    impl.resize(sz);
    ++modCounter;
  }

  template <typename Predicate>
  vector<T> filter(Predicate predicate) const& {
    vector<T> ret;
    for (const T& t : *this)
      if (predicate(t))
        ret.push_back(t);
    return ret;
  }

  template <typename Predicate>
  vector<T> filter(Predicate predicate) && {
    vector<T> ret;
    for (T& t : *this)
      if (predicate(t))
        ret.push_back(std::move(t));
    return ret;
  }

  template <typename Fun>
  auto transform(Fun fun) const {
    vector<decltype(fun(*impl.begin()))> ret;
    ret.reserve(size());
    for (const auto& elem : *this)
      ret.push_back(fun(elem));
    return ret;
  }

  vector<T> getSubsequence(int start, optional<int> lengthOption = none) const {
    auto length = min(size() - start, lengthOption.value_or(size() - start));
    CHECK(start >= 0 && length >= 0 && start + length <= size());
    vector<T> ret;
    ret.reserve(length);
    for (int i = start; i < start + length; ++i)
      ret.push_back(this->operator[](i));
    return ret;
  }

  vector<T> getPrefix(int num) const {
    return getSubsequence(0, num);
  }

  vector<T> getSuffix(int num) const {
    return getSubsequence(size() - num, num);
  }

  template <typename BaseIterator>
  class Iterator : public std::iterator<std::random_access_iterator_tag, T> {
    public:
    Iterator(const BaseIterator& i, const vector<T>* p) : it(i), parent(p), currentMod(p->modCounter) {}
    Iterator() {}

    using value_type = T;
    using difference_type = long;

    void checkParent() const {
      CHECK(parent->modCounter == currentMod) << "Container was modified during iteration.";
    }

    decltype(auto) operator* () const {
      checkParent();
      return *it;
    }

    decltype(auto) operator-> () const {
      checkParent();
      return *it;
    }

    bool operator != (const Iterator& other) const {
      return !(*this == other);
    }

    bool operator == (const Iterator& other) const {
      return it == other.it;
    }

    Iterator operator + (difference_type n) const {
      checkParent();
      return Iterator(it + n, parent);
    }

    Iterator operator - (difference_type n) const {
      checkParent();
      return Iterator(it - n, parent);
    }

    template <typename BaseIterator2>
    auto operator - (const Iterator<BaseIterator2>& o) const {
      return it - o.it;
    }

    Iterator& operator++ () {
      ++it;
      return *this;
    }

    Iterator& operator-- () {
      --it;
      return *this;
    }

    Iterator& operator+= (difference_type n) {
      it += n;
      return *this;
    }

    Iterator& operator-= (difference_type n) {
      it -= n;
      return *this;
    }

    Iterator operator++(int) {
      checkParent();
      return Iterator(it++, parent);
    }

    Iterator operator--(int) {
      checkParent();
      return Iterator(it--, parent);
    }

    bool operator > (const Iterator& o) const {
      return it > o.it;
    }

    bool operator < (const Iterator& o) const {
      return it < o.it;
    }

    bool operator >= (const Iterator& o) const {
      return it >= o.it;
    }

    bool operator <= (const Iterator& o) const {
      return it <= o.it;
    }

    BaseIterator it;
    const vector<T>* parent;
    int currentMod;
  };

  using iterator = Iterator<T*>;
  using const_iterator = Iterator<const T*>;

  template <typename>
  friend class Iterator;

  auto begin() const {
    return const_iterator(&*impl.begin(), this);
  }

  auto end() const {
    return const_iterator(&*impl.end(), this);
  }

  auto begin() {
    return iterator(&*impl.begin(), this);
  }

  auto end() {
    return iterator(&*impl.end(), this);
  }

  std::vector<T> impl;
  private:
  int modCounter = 0;
};

template <class Archive, typename T>
inline void serialize(Archive& ar1, vector<T>& v) {
  ar1(v.impl);
}


template <typename T, typename Compare = std::less<T>>
class set : public std::set<T, Compare> {
  public:
  using std::set<T, Compare>::set;

  using base = std::set<T, Compare>;

  int size() const {
    return (int) base::size();
  }

  template <typename Fun>
  auto transform(Fun fun) const {
    vector<decltype(fun(std::declval<T>()))> ret;
    ret.reserve(size());
    for (const auto& elem : *this)
      ret.push_back(fun(elem));
    return ret;
  }
};

template <typename T, typename Hash = std::hash<T>>
class unordered_set : public std::unordered_set<T, Hash> {
  public:
  using std::unordered_set<T, Hash>::unordered_set;

  using base = std::unordered_set<T, Hash>;

  int size() const {
    return (int) base::size();
  }

  template <typename Fun>
  auto transform(Fun fun) const {
    vector<decltype(fun(std::declval<T>()))> ret;
    ret.reserve(size());
    for (const auto& elem : *this)
      ret.push_back(fun(elem));
    return ret;
  }

  template <typename Fun>
  auto filter(Fun fun) const {
    unordered_set ret;
    for (const auto& elem : *this)
      if (fun(elem))
        ret.insert(elem);
    return ret;
  }

  vector<T> asVector() const {
    vector<T> ret;
    for (auto&& elem : *this)
      ret.push_back(std::move(elem));
    return ret;
  }
};

template<class T>
std::ostream& operator<<(std::ostream& d, const vector<T>& container){
  d << "{";
  for (const T& elem : container) {
    d << elem << ",";
  }
  d << "}";
  return d;
}
