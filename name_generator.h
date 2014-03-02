#ifndef _NAME_GENERATOR
#define _NAME_GENERATOR

class NameGenerator {
  public:
  NameGenerator() = default;
  string getNext();

  static NameGenerator firstNames;
  static NameGenerator scrolls;
  static NameGenerator aztecNames;
  static NameGenerator creatureNames;
  static NameGenerator weaponNames;
  static NameGenerator worldNames;
  static NameGenerator townNames;
  static NameGenerator dwarfNames;
  static NameGenerator deityNames;
  static NameGenerator demonNames;
  static NameGenerator dogNames;

  static void init(
      const string& firstNamesPath,
      const string& aztecNamesPath,
      const string& specialCreaturesPath,
      const string& specialWeaponsPath,
      const string& worldsPath,
      const string& townsPath,
      const string& dwarfPath,
      const string& deitiesPath,
      const string& demonsPath,
      const string& dogsPath);

  private:
  NameGenerator(vector<string> names, bool oneName = false);
  queue<string> names;
  bool oneName;
};

#endif
