#pragma once

class SimpleSDIWnd;

class SimpleSDIApp : public Yaswl::WinApp
{
public:
  SimpleSDIApp();
  ~SimpleSDIApp();

public:
  bool Init(HINSTANCE hInstance, const std::wstring& strCmdLine) override;
  int Run(Yaswl::Window* pMainWnd) override;
};
