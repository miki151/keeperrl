#ifndef _FIELD_OF_VIEW_H
#define _FIELD_OF_VIEW_H

#include "util.h"
#include "square.h"

class FieldOfView {
  public:
  FieldOfView(const Table<PSquare>& squares);
  bool canSee(Vec2 from, Vec2 to);
  const vector<Vec2>& getVisibleTiles(Vec2 from);
  void squareChanged(Vec2 pos);

  SERIALIZATION_DECL(FieldOfView);

  private:

  const static int sightRange = 30;

  class Visibility {
    public:

    bool checkVisible(int x,int y) const;
    const vector<Vec2>& getVisibleTiles() const;

    Visibility(const Table<PSquare>& squares, int x, int y);
    Visibility(Visibility&&) = default;
    Visibility& operator = (Visibility&&) = default;

    SERIALIZATION_DECL(Visibility);

    private:
    char visible[sightRange * 2 + 1][sightRange * 2 + 1];
    SERIAL3(visible);
    vector<Vec2> SERIAL(visibleTiles);
    void calculate(int,int,int,int, int, int, int, int,
        function<bool (int, int)> isBlocking,
        function<void (int, int)> setVisible);
    void setVisible(int, int);

    int SERIAL(px);
    int SERIAL(py);
  };
  
  const Table<PSquare>* SERIAL(squares);
  Table<Optional<Visibility>> SERIAL(visibility);
};

#endif
