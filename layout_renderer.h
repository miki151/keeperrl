#pragma once

#include "stdafx.h"
#include "file_path.h"
#include "layout_canvas.h"

class MainLoop;

void renderAscii(const LayoutCanvas::Map&, istream& file);
void generateMapLayout(const MainLoop&, const string& layoutName, FilePath glyphPath, const string& layoutSize);
