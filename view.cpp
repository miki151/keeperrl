#include "stdafx.h"

#include "view.h"


View::ListElem::ListElem(const string& t, ElemMod m, Optional<ActionId> a) : text(t), mod(m), action(a) {
}

View::ListElem::ListElem(const char* s, ElemMod m, Optional<ActionId> a) : text(s), mod(m), action(a) {
}

const string& View::ListElem::getText() const {
  return text;
}

View::ElemMod View::ListElem::getMod() const {
  return mod;
}

Optional<ActionId> View::ListElem::getActionId() const {
  return action;
}

vector<View::ListElem> View::getListElem(const vector<string>& v) {
  function<ListElem(const string&)> fun = [](const string& s) -> ListElem { return ListElem(s); };
  return transform2<ListElem>(v, fun);
}
