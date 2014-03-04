#ifndef _GENDER_H
#define _GENDER_H

class Gender {
  public:
  const static Gender male;
  const static Gender female;
  Gender(bool female);
  static void initialize();

  string his() const;
  string he() const;
  string god() const;

  SERIALIZATION_DECL(Gender);

  private:
  bool fem;
};

#endif
