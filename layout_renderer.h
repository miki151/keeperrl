#pragma once

#include "stdafx.h"
#include "layout_canvas.h"

class MainLoop;

void renderAscii(const LayoutCanvas::Map&, istream& file);
void generateMapLayout(const MainLoop&, const string& layoutName, const string& glyphPath, const string& layoutSize);
