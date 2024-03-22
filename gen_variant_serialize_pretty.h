

void serialize(PrettyInputArchive& ar1, VARIANT_NAME& v) {
  string name;
  auto bookmark = ar1.bookmark();
  ar1.readText(name);
#define X(Type, Index)\
  if (name == #Type) { \
    v.index = Index; \
    new(&v.elem##Index) Type;\
    ar1(v.elem##Index); \
    return; \
  }
  VARIANT_TYPES_LIST
#undef X
#ifdef DEFAULT_ELEM
#define X(Type, Index)\
  if (!strcmp(DEFAULT_ELEM, #Type)) { \
    v.index = Index; \
    new(&v.elem##Index) Type;\
    ar1.seek(bookmark);\
    ar1(v.elem##Index); \
    return; \
  }
  VARIANT_TYPES_LIST
  #undef X
  ar1.error("Bad default elem");
#else
  ar1.error(name + " is not part of variant");
#endif
}
