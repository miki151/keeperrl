#ifndef _UNIQUE_ENTITY_H
#define _UNIQUE_ENTITY_H

typedef int UniqueId;

class UniqueEntity {
  public:
  UniqueEntity();
  UniqueId getUniqueId() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  UniqueId id;
};

#endif
