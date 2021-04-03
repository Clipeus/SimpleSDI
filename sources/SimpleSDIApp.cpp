#include "pch.h"
#include "SimpleSDI.h"
#include "SimpleSDIApp.h"
#include "SimpleSDIWnd.h"

using namespace Yaswl;

SimpleSDIApp::SimpleSDIApp()
{
  m_strAppName = LoadString(IDS_APP_TITLE);
  m_strRegistryRoot = L"SOFTWARE\\LMSoft\\SimpleSDI";
}

SimpleSDIApp::~SimpleSDIApp()
{
}

bool SimpleSDIApp::Init(HINSTANCE hInstance, const std::wstring& strCmdLine)
{
  HWND hWnd = ::FindWindow(SimpleSDIWnd::GetClassName(), nullptr);
  if (hWnd)
  {
    if (::IsIconic(hWnd))
      ::ShowWindow(hWnd, SW_RESTORE);
    ::SetForegroundWindow(hWnd);
    return false;
  }

  if (!WinApp::Init(hInstance, strCmdLine))
  {
    ReportSystemError(IDS_SYSTEM_ERROR);
    return 1;
  }

  return true;
}

int SimpleSDIApp::Run(Yaswl::Window* pMainWnd)
{
  m_hAccel = ::LoadAccelerators(GetInstance(), MAKEINTRESOURCE(IDR_MAIN_ACCEL));
  if (!m_hAccel)
  {
    ReportSystemError(IDS_SYSTEM_ERROR);
    return 1;
  }

  return WinApp::Run(pMainWnd);
}
