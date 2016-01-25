#ifndef _STAIR_KEY_H
#define _STAIR_KEY_H


class StairKey {
  public:
  static StairKey getNew();
  static StairKey heroSpawn();
  static StairKey keeperSpawn();

  bool operator == (const StairKey&) const;

  SERIALIZATION_DECL(StairKey);

  int getInternalKey() const;

  private:
  StairKey(int key);
  int SERIAL(key);
  static int numKeys;
};

namespace std {

template <> struct hash<StairKey> {
  size_t operator()(const StairKey& obj) const {
    return hash<int>()(obj.getInternalKey());
  }
};
}


#endif
