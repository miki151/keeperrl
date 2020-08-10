

void serialize(PrettyInputArchive& ar1, VARIANT_NAME& v) {
  string name;
  auto bookmark = ar1.bookmark();
  ar1.readText(name);
#define X(Type, Index)\
  if (name == #Type || (name == "{" && !strcmp(#Type, "Chain"))) { \
    if (name == "{")\
      ar1.seek(bookmark);\
    v.index = Index; \
    new(&v.elem##Index) Type;\
    ar1(v.elem##Index); \
  } else
  VARIANT_TYPES_LIST
#undef X
  ar1.error(name + " is not part of variant");
}
