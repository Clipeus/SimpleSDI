#pragma once

class SimpleSDIWnd : public Yaswl::MainWindow, public Yaswl::OwnerDrawMenuImpl<SimpleSDIWnd>
{
  IMPLEMENT_ODW(SimpleSDIWnd, m_wndToolBar);

public:
  SimpleSDIWnd();
  ~SimpleSDIWnd();

public:
  bool Init();
  bool Create();

public:
  static const wchar_t* GetClassName();
  LRESULT SubWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);

protected:
  void OnRegisterClass(WNDCLASSEX& wc) override;

private:
  std::wstring SelectFile(bool bOpen, const std::wstring& strDefFileName = L"", int* pCodePage = nullptr);
  bool OpenFile(const std::wstring& strFileName);
  bool SaveFile(const std::wstring& strFileName, int nCodePage);
  void ChangeUIState(HMENU hContextMenu = nullptr);
  void UpdateStatusBarText();
  int ShowSaveConfirmDlg() const;

private:
  LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
  void OnDestroy(HWND hWnd);
  bool OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
  void OnSize(HWND hWnd, UINT state, int cx, int cy);
  void OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
  void OnMenuSelect(HWND hWnd, HMENU hMenu, int item, HMENU hMenuPopup, UINT flags);
  void OnContextMenu(HWND hWnd, HWND hwndContext, UINT x, UINT y);
  LRESULT OnNotify(HWND hWnd, int idFrom, NMHDR* pNmndr);
  bool OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg);
  void OnActivate(HWND hWnd, UINT nState, HWND hWndActDeact, bool fMinimized);
  void OnInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, bool fSystemMenu);

  bool OnOpenFile(bool bNew = false);
  bool OnSaveFile(bool bSaveAs = false);

private:
  bool m_bComInited = false;
  bool m_bDirty = false;
  Yaswl::WindowSubclassEx<SimpleSDIWnd> m_wndEditView;
  Yaswl::ToolBar m_wndToolBar;
  HWND m_hStatusBar = nullptr;
  std::wstring m_strFileName;
  int m_nCodePage = CP_UTF8;
};
