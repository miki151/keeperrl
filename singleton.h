#ifndef _SINGLETON_H
#define _SINGLETON_H

template<class T, class E>
class Singleton {
  public:
  static T* get(E);
  static void set(E, T*);
  static bool exists(E);
  static void initialize();

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar & elems;
  }

  private:
  static unordered_map<E, unique_ptr<T>> elems;

};

#endif
