

void serialize(PrettyInputArchive& ar1, VARIANT_NAME& v) {
  string name;
  ar1.readText(name);
#define X(Type, Index)\
  if (name == #Type) { \
    v.index = Index; \
    new(&v.elem##Index) Type;\
    ar1(v.elem##Index); \
  } else
  VARIANT_TYPES_LIST
#undef X
  ar1.error(name + " is not part of variant");
}
