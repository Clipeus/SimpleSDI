#include "pch.h"
#include "SimpleSDI.h"
#include "SimpleSDIApp.h"
#include "SimpleSDIWnd.h"
#include "AboutDlg.h"
#include "TextEncoding.h"

constexpr int IDC_MAIN_STATUSBAR = 1;
constexpr int IDC_EDIT_VIEW = IDC_MAIN_STATUSBAR + 1;
constexpr int IDC_MAIN_TOOLBAR = IDC_EDIT_VIEW + 1;

constexpr int STATUSBAR_PART1_WIDTH = 100;
constexpr int STATUSBAR_PART2_WIDTH = 300;

constexpr int WM_MONITORSTATE = WM_USER + 1;
constexpr int WM_MONITOREVENT = WM_USER + 2;
constexpr int WM_MONITORERROR = WM_USER + 3;

using namespace Yaswl;

SimpleSDIWnd::SimpleSDIWnd() : MainWindow(SimpleSDIWnd::GetClassName()),  m_wndEditView(this, &SimpleSDIWnd::SubWndProc)
{
}

SimpleSDIWnd::~SimpleSDIWnd()
{

}

void SimpleSDIWnd::OnRegisterClass(WNDCLASSEX& wc)
{
  wc.hIcon = (HICON)LoadImage(GetApp()->GetInstance(), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 32, 32, 0);
  wc.hIconSm = (HICON)LoadImage(GetApp()->GetInstance(), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 16, 16, 0);
  wc.lpszMenuName = MAKEINTRESOURCE(IDM_MAIN);
}

bool SimpleSDIWnd::Init()
{
  if (!MainWindow::Init())
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  return true;
}

bool SimpleSDIWnd::Create()
{
  if (!WindowEx::Create(WS_EX_ACCEPTFILES, GetClassName(), GetApp()->GetAppName()))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  _ASSERTE(m_hWnd);

  ::UpdateWindow(m_hWnd);

  return true;
}

const wchar_t* SimpleSDIWnd::GetClassName()
{
  return L"SimpleSDIApp_MainWindow_1.0";
}

std::wstring SimpleSDIWnd::SelectFile(bool bOpen, const std::wstring& strDefFileName, int* pCodePage)
{
  WinApp::WaitCursor wc;
  std::wstring path;

  if (!m_bComInited)
  {
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    _ASSERTE(SUCCEEDED(hr));
    if (!SUCCEEDED(hr))
      return path;

    m_bComInited = true;
  }

  _COM_SMARTPTR_TYPEDEF(IFileDialog, __uuidof(IFileDialog));
  _COM_SMARTPTR_TYPEDEF(IFileDialogCustomize, __uuidof(IFileDialogCustomize));
  _COM_SMARTPTR_TYPEDEF(IShellItem, __uuidof(IShellItem));

  _com_ptr_t<IFileDialogPtr> pfd;
  if (SUCCEEDED(pfd.CreateInstance(bOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER)))
  {
    constexpr int CTRL_ID = 1;
    _com_ptr_t<IFileDialogCustomizePtr> pfdc = pfd;

    if (!bOpen)
    {
      pfdc->AddComboBox(CTRL_ID);
      pfdc->AddControlItem(CTRL_ID, TextEncoding::GetOEMCodePage(), GetApp()->LoadString(IDS_OEM_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, TextEncoding::GetANSICodePage(), GetApp()->LoadString(IDS_ANSI_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, CP_UTF7, GetApp()->LoadString(IDS_UTF7_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, CP_UTF8, GetApp()->LoadString(IDS_UTF8_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, 1200, GetApp()->LoadString(IDS_UTF16_LE_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, 1201, GetApp()->LoadString(IDS_UTF16_BE_TXT).c_str());

      if (pCodePage)
      {
        if (S_OK != pfdc->SetSelectedControlItem(CTRL_ID, *pCodePage))
        {
          CPINFOEX cpInfo = { 0 };
          ::GetCPInfoEx(*pCodePage, CP_ACP, &cpInfo);
          pfdc->AddControlItem(CTRL_ID, *pCodePage, cpInfo.CodePageName);
          pfdc->SetSelectedControlItem(CTRL_ID, *pCodePage);
        }
      }
    }

    DWORD dwOptions;
    if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
    {
      if (bOpen)
        pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
      else
        pfd->SetOptions(dwOptions | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN);
    }

    COMDLG_FILTERSPEC filter[2];
    std::wstring str1 = GetApp()->LoadString(IDS_FILE_FILTER_TXT);
    filter[0].pszName = str1.c_str();
    std::wstring str2 = GetApp()->LoadString(IDS_FILE_FILTER_TXT_EXT);
    filter[0].pszSpec = str2.c_str();
    std::wstring str3 = GetApp()->LoadString(IDS_FILE_FILTER_ALL);
    filter[1].pszName = str3.c_str();
    std::wstring str4 = GetApp()->LoadString(IDS_FILE_FILTER_ALL_EXT);
    filter[1].pszSpec = str4.c_str();
    pfd->SetFileTypes(2, filter);

    pfd->SetDefaultExtension(str2.substr(2).c_str());
    if (strDefFileName.size())
    {
      std::filesystem::path path = strDefFileName;
      pfd->SetFileName(path.filename().wstring().c_str());
    }

    if (SUCCEEDED(pfd->Show(m_hWnd)))
    {
      _com_ptr_t <IShellItemPtr> psi;
      if (SUCCEEDED(pfd->GetResult(&psi)))
      {
        LPWSTR lpFolder;
        if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &lpFolder)))
        {
          path = lpFolder;
          ::CoTaskMemFree(lpFolder);

          if (!bOpen)
          {
            pfdc->GetSelectedControlItem(CTRL_ID, reinterpret_cast<DWORD*>(pCodePage));
          }
        }
      }
    }
  }

  return path;
}

bool SimpleSDIWnd::OpenFile(const std::wstring& strFileName)
{
  Yaswl::unique_handle m_hFile = Yaswl::make_unique_handle(::CreateFile(strFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (m_hFile.get() == INVALID_HANDLE_VALUE)
  {
    GetApp()->ReportSystemError(IDS_OPEN_FILE_FAILED);
    return false;
  }

  LARGE_INTEGER li;
  if (!::GetFileSizeEx(m_hFile.get(), &li))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  if (li.HighPart > 0)
  {
    GetApp()->ReportBox(IDS_FILE_IS_TO_LARGE, MB_OK | MB_ICONERROR);
    return false;
  }

  std::vector<uint8_t> vecBuffer;
  vecBuffer.resize(li.LowPart);
  DWORD dwRead = 0;
  if (!::ReadFile(m_hFile.get(), vecBuffer.data(), vecBuffer.size(), &dwRead, nullptr))
  {
    GetApp()->ReportSystemError(IDS_READ_FILE_FAILED);
    return false;
  }

  TextEncoding te(vecBuffer.data(), vecBuffer.size());
  m_nCodePage = te.DetectCodePage();
  size_t nDestSize = te.GetToUnicodeCharCount(m_nCodePage);
  if (nDestSize != -1)
  {
    HLOCAL hLocal = Edit_GetHandle(m_wndEditView);
    hLocal = ::LocalReAlloc(hLocal, (nDestSize + 1) * sizeof(wchar_t), LMEM_MOVEABLE);
    if (hLocal)
    {
      LPVOID pBuffer = ::LocalLock(hLocal);
      if (pBuffer)
      {
        te.ToUnicode(reinterpret_cast<wchar_t*>(pBuffer), nDestSize, m_nCodePage);
        reinterpret_cast<wchar_t*>(pBuffer)[nDestSize] = 0;
        ::LocalUnlock(hLocal);
        Edit_SetHandle(m_wndEditView, hLocal);
      }
      _ASSERTE(pBuffer);
    }
    _ASSERTE(hLocal);
  }
  _ASSERTE(nDestSize != -1);

  return true;
}

bool SimpleSDIWnd::SaveFile(const std::wstring& strFileName, int nCodePage)
{
  Yaswl::unique_handle m_hFile = Yaswl::make_unique_handle(::CreateFile(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (m_hFile.get() == INVALID_HANDLE_VALUE)
  {
    GetApp()->ReportSystemError(IDS_SAVE_FILE_FAILED);
    return false;
  }

  size_t nSrcSize = Edit_GetTextLength(m_wndEditView);
  HLOCAL hLocal = Edit_GetHandle(m_wndEditView);
  LPVOID pBuffer = ::LocalLock(hLocal);
  if (pBuffer)
  {
    TextEncoding te(pBuffer, nSrcSize);
    size_t nDestSize = te.GetFromUnicodeBufferSize(nCodePage);
    if (nDestSize != -1)
    {
      std::vector<char> vecBuffer;
      vecBuffer.resize(nDestSize);

      te.FromUnicode(nCodePage, vecBuffer.data(), vecBuffer.size());
      ::LocalUnlock(hLocal);

      DWORD dwWritten = 0;
      if (!::WriteFile(m_hFile.get(), vecBuffer.data(), vecBuffer.size(), &dwWritten, nullptr))
      {
        GetApp()->ReportSystemError(IDS_WRITE_FILE_FAILED);
        return false;
      }
    }
    _ASSERTE(nDestSize != -1);
  }
  _ASSERTE(pBuffer);

  return true;
}

void SimpleSDIWnd::ChangeUIState(HMENU hContextMenu)
{
  int nStartSel, nEndSel;
  ::SendMessage(m_wndEditView, EM_GETSEL, (WPARAM)&nStartSel, (LPARAM)&nEndSel);
  int nEnabled = MAKELONG(nEndSel > nStartSel, 0);
  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_CUT, nEnabled);
  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_COPY, nEnabled);
  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_DELETE, nEnabled);
  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_UNDO, MAKELONG(Edit_CanUndo(m_wndEditView), 0));
  
  std::filesystem::path path = m_strFileName;
  std::wstring fname = path.filename().wstring();

  std::wstring strText = Edit_GetModify(m_wndEditView) ? L"*" : L"";
  strText += (fname.size() ? fname : GetApp()->LoadString(IDS_UNTITLED_TXT)) + L" - " + GetApp()->LoadString(IDS_APP_TITLE);
  ::SetWindowText(m_hWnd, strText.c_str());

  UpdateStatusBarText();
}

void SimpleSDIWnd::UpdateStatusBarText()
{
  ::SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)GetApp()->LoadString(IDS_READY).c_str());
  
  std::wstring strCodePage = GetApp()->LoadString(ID_CODEPAGE_TXT);

  CPINFOEX cpInfo = { 0 };
  if (::GetCPInfoEx(m_nCodePage, CP_ACP, &cpInfo))
  {
    strCodePage += cpInfo.CodePageName;
  }
  else
  {
    if (m_nCodePage == 1200)
      strCodePage += GetApp()->LoadString(IDS_UTF16_LE_TXT);
    else if (m_nCodePage == 1201)
      strCodePage += GetApp()->LoadString(IDS_UTF16_BE_TXT);
    else
      strCodePage += GetApp()->LoadString(IDS_UNICODE_TXT);
  }

  int nLine = Edit_LineFromChar(m_wndEditView, -1) + 1;
  int nEndSel = 0;
  ::SendMessage(m_wndEditView, EM_GETSEL, 0L, (LPARAM)&nEndSel);
  int nCol = nEndSel - Edit_LineIndex(m_wndEditView, -1) + 1;

  ::SendMessage(m_hStatusBar, SB_SETTEXT, 1, (LPARAM)StrFormat(GetApp()->LoadString(IDS_LINE_COL_TXT).c_str(), nLine, nCol).c_str());
  ::SendMessage(m_hStatusBar, SB_SETTEXT, 2, (LPARAM)strCodePage.c_str());
}

int SimpleSDIWnd::ShowSaveConfirmDlg() const
{
  std::wstring strText = StrFormat(GetApp()->LoadString(IDS_SAVE_TO_FILE_TXT).c_str(), m_strFileName.size() ? m_strFileName.c_str() : GetApp()->LoadString(IDS_UNTITLED_TXT).c_str());

  TASKDIALOG_BUTTON tdb[2] = { 0 };
  tdb[0].nButtonID = IDYES;
  tdb[0].pszButtonText = MAKEINTRESOURCE(IDS_SAVE_BTN_TXT);
  tdb[1].nButtonID = IDNO;
  tdb[1].pszButtonText = MAKEINTRESOURCE(IDS_NO_SAVE_BTN_TXT);

  TASKDIALOGCONFIG tdc = { 0 };
  tdc.cbSize = sizeof(tdc);
  tdc.hwndParent = m_hWnd;
  tdc.hInstance = GetApp()->GetInstance();
  tdc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
  tdc.pszWindowTitle = MAKEINTRESOURCE(IDS_APP_TITLE);
  tdc.pszMainInstruction = strText.c_str();
  tdc.cButtons = 2;
  tdc.pButtons = tdb;

  int nButton = -1;
  if (S_OK != ::TaskDialogIndirect(&tdc, &nButton, nullptr, nullptr))
    return -1;

  return nButton;
}

LRESULT SimpleSDIWnd::SubWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
  if (hWnd == m_wndEditView)
  {
    switch (uMsg)
    {
      HANDLE_MSG(hWnd, WM_CONTEXTMENU, OnContextMenu), bHandled = true;
    }

    if (uMsg == WM_KEYUP || uMsg == WM_KEYDOWN || (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST))
      ChangeUIState();
  }
  return 0;
}

LRESULT SimpleSDIWnd::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
    HANDLE_MSG(hWnd, WM_SIZE, OnSize);
    HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
    HANDLE_MSG(hWnd, WM_MENUSELECT, OnMenuSelect);
    HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
    HANDLE_MSG(hWnd, WM_NOTIFY, OnNotify);
    HANDLE_MSG(hWnd, WM_SETCURSOR, OnSetCursor);
    HANDLE_MSG(hWnd, WM_ACTIVATE, OnActivate);
    HANDLE_MSG(hWnd, WM_INITMENUPOPUP, OnInitMenuPopup);
    HANDLE_MSG(hWnd, WM_MEASUREITEM, OnMeasureItem), true; // should return true
    HANDLE_MSG(hWnd, WM_DRAWITEM, OnDrawItem), true; // should return true
    default:
      return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}

void SimpleSDIWnd::OnDestroy(HWND hWnd)
{
  SaveWindowPlacement();

  if (m_bComInited)
    ::CoUninitialize();

  HFONT hFont = (HFONT)::SendMessage(m_wndEditView, WM_GETFONT, 0, 0);
  if (hFont)
    ::DeleteObject(hFont);

  ::PostQuitMessage(0);
}

bool SimpleSDIWnd::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
  m_hWnd = hWnd;

  TBBUTTON tbButtons[] =
  {
    {IDI_NEW, ID_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_OPEN, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_SAVE, ID_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_UNDO, ID_EDIT_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_CUT, ID_EDIT_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_COPY, ID_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_PASTE, ID_EDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_DELETE, ID_EDIT_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_HELP, IDM_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
  };

  if (!m_wndToolBar.Create(m_hWnd, IDC_MAIN_TOOLBAR))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  if (!m_wndToolBar.Init(tbButtons, sizeof(tbButtons) / sizeof(tbButtons[0])))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  m_hStatusBar = ::CreateWindowEx(0, STATUSCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(static_cast<long long>(IDC_MAIN_STATUSBAR)), GetApp()->GetInstance(), nullptr);
  if (!m_hStatusBar)
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  RECT rect;
  ::GetClientRect(m_hWnd, &rect);

  if (!m_wndEditView.Create(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
    0, 0, rect.right, rect.bottom, m_hWnd, reinterpret_cast<HMENU>(static_cast<long long>(IDC_EDIT_VIEW)), GetApp()->GetInstance()))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  Edit_LimitText(m_wndEditView, 1024 * 1024);

  HFONT hFont = GetStockFont(DEFAULT_GUI_FONT);
  LOGFONT lf;
  ::GetObject(hFont, sizeof(LOGFONT), &lf);

  HDC hDC = ::GetDC(m_wndEditView);
  lf.lfHeight = -::MulDiv(11, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
  ::ReleaseDC(m_wndEditView, hDC);

  hFont = ::CreateFontIndirect(&lf);
  ::SendMessage(m_wndEditView, WM_SETFONT, (WPARAM)hFont, 0);
  ::SendMessage(m_wndEditView, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 3);
  m_wndEditView.Subclass(true);

  ChangeUIState();

  return true;
}

void SimpleSDIWnd::OnSize(HWND hWnd, UINT state, int cx, int cy)
{
  ::SendMessage(m_wndToolBar, WM_SIZE, state, 0);
  ::SendMessage(m_hStatusBar, WM_SIZE, state, 0);

  int ptWidth[3];
  ptWidth[0] = cx - (STATUSBAR_PART1_WIDTH + STATUSBAR_PART2_WIDTH);
  ptWidth[1] = cx - STATUSBAR_PART2_WIDTH;
  ptWidth[2] = -1;
  ::SendMessage(m_hStatusBar, SB_SETPARTS, 3, (LPARAM)ptWidth);

  RECT Rect;
  int nTop, nBottom;

  ::GetWindowRect(m_wndToolBar, &Rect);
  nTop = Rect.bottom - Rect.top - 1;

  ::GetWindowRect(m_hStatusBar, &Rect);
  nBottom = Rect.bottom - Rect.top - 1;

  ::MoveWindow(m_wndEditView, 0, nTop, cx, cy - nBottom - nTop, true);
}

void SimpleSDIWnd::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
  switch (id)
  {
    case IDC_EDIT_VIEW:
    {
      if (codeNotify == EN_MAXTEXT)
        GetApp()->ReportBox(IDS_MAXTEXT_TXT, MB_OK| MB_ICONINFORMATION);

      break;
    }
    case IDM_EXIT:
    {
      ::PostMessage(hWnd, WM_CLOSE, 0, 0);
      break;
    }
    case ID_FILE_NEW:
    case ID_FILE_OPEN:
    {
      OnOpenFile(id == ID_FILE_NEW);
      ChangeUIState();
      break;
    }
    case ID_FILE_SAVE:
    case ID_FILE_SAVEAS:
    {
      OnSaveFile(id == ID_FILE_SAVEAS);
      ChangeUIState();
      break;
    }
    case ID_EDIT_UNDO:
    {
      Edit_Undo(m_wndEditView);
      ChangeUIState();
      break;
    }
    case ID_EDIT_CUT:
    {
      ::SendMessage(m_wndEditView, WM_CUT, 0, 0);
      ChangeUIState();
      break;
    }
    case ID_EDIT_COPY:
    {
      ::SendMessage(m_wndEditView, WM_COPY, 0, 0);
      ChangeUIState();
      break;
    }
    case ID_EDIT_PASTE:
    {
      ::SendMessage(m_wndEditView, WM_PASTE, 0, 0);
      ChangeUIState();
      break;
    }
    case ID_EDIT_DELETE:
    {
      ::SendMessage(m_wndEditView, WM_CLEAR, 0, 0);
      ChangeUIState();
      break;
    }
    case ID_EDIT_SELECTALL:
    {
      Edit_SetSel(m_wndEditView, 0, -1);
      ChangeUIState();
      break;
    }
    case IDM_ABOUT:
    {
      AboutDlg dlg;
      dlg.DoModal(m_hWnd);
      break;
    }
  }
}

bool SimpleSDIWnd::OnOpenFile(bool bNew)
{
  if (Edit_GetModify(m_wndEditView))
  {
    int nDlgResult = ShowSaveConfirmDlg();
    if (nDlgResult == IDYES)
    {
      if (!OnSaveFile())
        return false;
    }
    else if (nDlgResult == IDCANCEL)
    {
      return false;
    }
  }

  if (!bNew)
  {
    std::wstring path = SelectFile(true);
    if (!path.size())
      return false;

    if (OpenFile(path))
      m_strFileName = path;
  }
  else
  {
    Edit_SetText(m_wndEditView, L"");
    m_nCodePage = CP_UTF8;
  }

  Edit_SetModify(m_wndEditView, false);
  Edit_EmptyUndoBuffer(m_wndEditView);

  return true;
}

bool SimpleSDIWnd::OnSaveFile(bool bSaveAs)
{
  std::wstring path = (!bSaveAs ? m_strFileName : L"");
  int nCodePage = m_nCodePage;

  if (!path.size())
    path = SelectFile(false, m_strFileName, &nCodePage);

  if (path.size())
  {
    if (SaveFile(path, nCodePage))
    {
      m_strFileName = path;
      m_nCodePage = nCodePage;
      Edit_SetModify(m_wndEditView, false);
      return true;
    }
  }
  return false;
}

void SimpleSDIWnd::OnMenuSelect(HWND hWnd, HMENU hMenu, int item, HMENU hMenuPopup, UINT flags)
{
  std::wstring strText;

  if ((flags == 0xFFFFFFFF) && (hMenu == nullptr))
  {
    UpdateStatusBarText();
  }
  else
  {
    if ((flags & MF_POPUP) || (flags & MFT_SEPARATOR))
    {
      strText.clear();
    }
    else
    {
      strText = GetApp()->LoadString(item);
      size_t pos = strText.find(L'\n');
      if (std::wstring::npos != pos)
        strText = strText.substr(0, pos);
    }
    ::SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)strText.c_str());
  }
}

void SimpleSDIWnd::OnContextMenu(HWND hWnd, HWND hwndContext, UINT x, UINT y)
{
  POINT pt;
  RECT rect;

  if (x == -1 || y == -1)
  {
    Edit_GetRect(m_wndEditView, &rect);

    pt.x = rect.right / 2;
    pt.y = rect.bottom / 2;
    ::ClientToScreen(m_wndEditView, &pt);
  }
  else
  {
    pt.x = x;
    pt.y = y;

    if (m_wndEditView != ::WindowFromPoint(pt))
      return;
  }

  HMENU hMenuMain = ::LoadMenu(GetApp()->GetInstance(), MAKEINTRESOURCE(IDR_EDIT_CONTEXT_MENU));
  HMENU hMenuContext = ::GetSubMenu(hMenuMain, 0);

  ::TrackPopupMenu(hMenuContext, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hWnd, nullptr);
  ::DestroyMenu(hMenuContext);
  ::DestroyMenu(hMenuMain);
}

LRESULT SimpleSDIWnd::OnNotify(HWND hWnd, int idFrom, NMHDR* pNmndr)
{
  switch (pNmndr->code)
  {
    case TTN_GETDISPINFO:
    {
      LPTOOLTIPTEXT lpTTT = reinterpret_cast<LPTOOLTIPTEXT>(pNmndr);
      std::wstring strText = GetApp()->LoadString(idFrom);
      size_t pos = strText.find(L'\n');
      if (std::wstring::npos != pos)
        strText = strText.substr(pos + 1);
      wcsncpy_s(lpTTT->szText, strText.c_str(), sizeof(lpTTT->szText));
      return true;
    }
  }
  return false;
}

bool SimpleSDIWnd::OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
  if (GetApp()->IsWaitCursor())
  {
    ::SetCursor(GetApp()->GetWaitCursor());
    return true;
  }
  return FORWARD_WM_SETCURSOR(hWnd, hWndCursor, codeHitTest, msg, ::DefWindowProc);
}

void SimpleSDIWnd::OnActivate(HWND hWnd, UINT nState, HWND hWndActDeact, bool fMinimized)
{
  if (nState == WA_ACTIVE)
    ::SetFocus(m_wndEditView);
}

void SimpleSDIWnd::OnInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, bool fSystemMenu)
{
  if (!fSystemMenu)
  {
    int nCount = ::GetMenuItemCount(hMenu);
    for (int i = 0; i < nCount; i++)
    {
      MENUITEMINFO mii = { 0 };
      mii.cbSize = sizeof(mii);
      mii.fMask = MIIM_ID;
      ::GetMenuItemInfo(hMenu, i, true, &mii);
      if (mii.wID == ID_FILE_SAVEAS)
      {
        ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND | (m_strFileName.size() ? MFS_ENABLED : MFS_DISABLED));
      }
      else if (mii.wID == ID_EDIT_UNDO)
      {
        ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND | (Edit_CanUndo(m_wndEditView) ? MFS_ENABLED : MFS_DISABLED));
      }
      else if (mii.wID == ID_EDIT_CUT || mii.wID == ID_EDIT_COPY || mii.wID == ID_EDIT_DELETE)
      {
        int nStartSel, nEndSel;
        ::SendMessage(m_wndEditView, EM_GETSEL, (WPARAM)&nStartSel, (LPARAM)&nEndSel);
        ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND | (nEndSel > nStartSel ? MFS_ENABLED : MFS_DISABLED));
      }
    }

    Yaswl::OwnerDrawMenuImpl<SimpleSDIWnd>::OnInitMenuPopup(hWnd, hMenu, item, fSystemMenu);
  }
}
