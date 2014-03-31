#ifndef _ITEM_H
#define _ITEM_H

#include "util.h"
#include "view_object.h"
#include "item_attributes.h"
#include "effect.h"
#include "enums.h"
#include "attack.h"
#include "fire.h"
#include "unique_entity.h"

class Level;

class Item;
typedef function<bool(const Item*)> ItemPredicate;

class Item : private ItemAttributes, public UniqueEntity {
  public:
  typedef ItemAttributes ItemAttributes;
  Item(ViewObject o, const ItemAttributes&);
  virtual ~Item();

  static void identifyEverything();
  static bool isEverythingIdentified();

  virtual void apply(Creature*, Level*);

  bool isDiscarded();

  const ViewObject& getViewObject() const;

  string getName(bool plural = false, bool blind = false) const;
  string getTheName(bool plural = false, bool blind = false) const;
  string getAName(bool plural = false, bool blind = false) const;
  string getNameAndModifiers(bool plural = false, bool blind = false) const;
  string getArtifactName() const;

  virtual Optional<EffectType> getEffectType() const;
  ItemType getType() const;
  
  int getPrice() const;
  void setShopkeeper(const Creature* shopkeeper);
  const Creature* getShopkeeper() const;

  Optional<TrapType> getTrapType() const;

  bool canEquip() const;
  EquipmentSlot getEquipmentSlot() const;
  void addModifier(AttrType attributeType, int value);
  int getModifier(AttrType attributeType) const;

  void tick(double time, Level*, Vec2 position);
  
  string getApplyMsgThirdPerson() const;
  string getApplyMsgFirstPerson() const;
  string getNoSeeApplyMsg() const;

  static void identify(const string& name);
  void identify();
  bool isIdentified() const;
  bool canIdentify() const;
  void onEquip(Creature*);
  void onUnequip(Creature*);
  virtual void onEquipSpecial(Creature*) {}
  virtual void onUnequipSpecial(Creature*) {}
  virtual void setOnFire(double amount, const Level* level, Vec2 position);
  double getFireSize() const;

  void onHitSquareMessage(Vec2 position, Square*, bool plural);
  void onHitCreature(Creature* c, const Attack& attack, bool plural);

  int getAccuracy() const;
  double getApplyTime() const;
  double getWeight() const;
  string getDescription() const;

  AttackType getAttackType() const;
  bool isWieldedTwoHanded() const;

  static ItemPredicate effectPredicate(EffectType);
  static ItemPredicate typePredicate(ItemType);
  static ItemPredicate typePredicate(vector<ItemType>);
  static ItemPredicate namePredicate(const string& name);

  static vector<pair<string, vector<Item*>>> stackItems(vector<Item*>,
      function<string(const Item*)> addSuffix = [](const Item*) { return ""; });

  struct CorpseInfo {
    bool canBeRevived;
    bool hasHead;
    bool isSkeleton;

    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };

  virtual Optional<CorpseInfo> getCorpseInfo() const { return Nothing(); }

  SERIALIZATION_DECL(Item);

  protected:
  virtual void specialTick(double time, Level*, Vec2 position) {}
  void setName(const string& name);
  ViewObject SERIAL(viewObject);
  bool SERIAL2(discarded, false);
  bool SERIAL(inspected);
  static bool everythingIdentified;

  private:
  static bool isIdentified(const string& name);
  string getVisibleName(bool plural) const;
  string getRealName(bool plural) const;
  string getBlindName(bool plural) const;
  const Creature* SERIAL2(shopkeeper, nullptr);
  Fire SERIAL(fire);
};

#endif
