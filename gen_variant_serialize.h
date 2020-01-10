


template <class Archive>
void serialize(Archive& ar1, VARIANT_NAME& v) {
  ar1(v.index);
  switch (v.index) {
#define X(Type, Index)\
    case Index: \
      if (Archive::is_loading::value) \
        new(&v.elem##Index) Type; \
      ar1(v.elem##Index); \
      break;
    VARIANT_TYPES_LIST
#undef X
    default: FATAL << "Error saving variant";
  }
}
