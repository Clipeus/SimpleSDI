#include "pch.h"
#include "SimpleSDI.h"
#include "SimpleSDIApp.h"
#include "SimpleSDIWnd.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
  SimpleSDIApp app;
  if (!app.Init(hInstance, lpCmdLine))
    return 1;

  SimpleSDIWnd wnd;
  if (!wnd.Init() || !wnd.Create())
    return 1;

  wnd.RestoreWindowPlacement(nCmdShow);
  return app.Run(&wnd);
}
