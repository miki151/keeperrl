#include "stdafx.h"

#include "view.h"

using namespace std;


const static string prefix[] {"[title]", "[inactive]"};

View::ListElem::ListElem(const string& t, Optional<ElemMod> m, Optional<ActionId> a) : text(t), mod(m), action(a) {
}

View::ListElem::ListElem(const char* s) : ListElem(s, Nothing(), Nothing()) {
}

const string& View::ListElem::getText() const {
  return text;
}

Optional<View::ElemMod> View::ListElem::getMod() const {
  return mod;
}

Optional<ActionId> View::ListElem::getActionId() const {
  return action;
}

vector<View::ListElem> View::getListElem(const vector<string>& v) {
  function<ListElem(const string&)> fun = [](const string& s) -> ListElem { return ListElem(s); };
  return transform2(v, fun);
}
