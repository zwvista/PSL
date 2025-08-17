// This MFC Samples source code demonstrates using MFC Microsoft Office Fluent User Interface 
// (the "Fluent UI") and is provided only as referential material to supplement the 
// Microsoft Foundation Classes Reference and related electronic documentation 
// included with the MFC C++ library software.  
// License terms to copy, use or distribute the Fluent UI are available separately.  
// To learn more about our Fluent UI licensing program, please visit 
// http://msdn.microsoft.com/officeui.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// MazeEditorView.cpp : implementation of the CMazeEditorView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "MazeEditor.h"
#endif

#include "MazeEditorDoc.h"
#include "MazeEditorView.h"
#include "MemDC.h"
#undef min
#undef max
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMazeEditorView

IMPLEMENT_DYNCREATE(CMazeEditorView, CView)

BEGIN_MESSAGE_MAP(CMazeEditorView, CView)
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CMazeEditorView::OnFilePrintPreview)
    ON_WM_CONTEXTMENU()
    ON_WM_RBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_COMMAND(ID_MAZE_HEIGHT, &CMazeEditorView::OnResizeMaze)
    ON_COMMAND(ID_MAZE_WIDTH, &CMazeEditorView::OnResizeMaze)
    ON_COMMAND(ID_MAZE_CHAR, &CMazeEditorView::OnMazeChar)
    ON_COMMAND(ID_MAZE_SIDELEN, &CMazeEditorView::OnMazeSideLen)
    ON_COMMAND(ID_EDIT_COPY, &CMazeEditorView::OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, &CMazeEditorView::OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_MAZE_MOVEMENT, &CMazeEditorView::OnUpdateMovement)
    ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

// CMazeEditorView construction/destruction

CMazeEditorView::CMazeEditorView()
    : m_pDoc(NULL)
    , m_pBar(NULL)
    , m_pEditHeight(NULL)
    , m_pEditWidth(NULL)
{
}

CMazeEditorView::~CMazeEditorView()
{
}

BOOL CMazeEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CView::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
        ::LoadCursor(NULL, IDC_ARROW), NULL, NULL);

    return TRUE;
}

// CMazeEditorView drawing

void CMazeEditorView::OnDraw(CDC* pDC)
{
    if (!m_pDoc) return;

    CRect clientRect;
    GetClientRect(&clientRect); // Get the client area dimensions

    int canvasWidth = clientRect.Width();
    int canvasHeight = clientRect.Height();
    cellSize = max(min(canvasWidth, canvasHeight) * 0.8 / max(rows(), cols()), 10.0);
    boardWidth = cols() * cellSize;
    boardHeight = rows() * cellSize;
    startX = (canvasWidth - boardWidth) / 2;
    startY = (canvasHeight - boardHeight) / 2;

    CMemDC2 memdc(pDC);

    COLORREF clrFill = RGB(0, 200, 0);
    COLORREF clrFill2 = RGB(200, 0, 0);
    COLORREF clrWall = RGB(128, 0, 0);
    CPen penWall(PS_SOLID, 5, clrWall);
    COLORREF clrNone = RGB(0, 0, 0);
    CPen penNone(PS_SOLID, 1, clrNone);

    int save = memdc.SaveDC();
    memdc.SetBkMode(TRANSPARENT);
    for (int r = 0;; r++) {
        for (int c = 0;; c++) {
            Position p(r, c);
            CRect posRect(startX + c * cellSize, startY + r * cellSize,
                startX + (c + 1) * cellSize, startY + (r + 1) * cellSize);
            if (m_pDoc->IsSelectedPosition(p))
                memdc.FillSolidRect(posRect, clrFill);

            if (m_pDoc->IsObject(p)) {
                char ch = m_pDoc->GetObject(p);
                if (ch == '#' && !m_pDoc->IsSelectedPosition(p))
                    memdc.FillSolidRect(posRect, clrFill2);
                CString str(ch, 1);
                memdc.DrawText(str, posRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            if (m_pDoc->IsDot(p)) {
                int up = startY + p.first * cellSize - tolerance;
                int left = startX + p.second * cellSize - tolerance;
                int down = startY + p.first * cellSize + tolerance;
                int right = startX + p.second * cellSize + tolerance;
                memdc.SelectObject(penWall);
                memdc.Ellipse(left, up, right, down);
            }

            if (c == cols()) break;
            memdc.SelectObject(&(m_pDoc->IsHorzWall(p) ? penWall : penNone));
            memdc.MoveTo(startX + c * cellSize, startY + r * cellSize);
            memdc.LineTo(startX + (c + 1) * cellSize, startY + r * cellSize);
        }
        if (r == rows()) break;
        for (int c = 0; c < cols() + 1; c++) {
            memdc.SelectObject(&(m_pDoc->IsVertWall(Position(r, c)) ? penWall : penNone));
            memdc.MoveTo(startX + c * cellSize, startY + r * cellSize);
            memdc.LineTo(startX + c * cellSize, startY + (r + 1) * cellSize);
        }
    }
    memdc.RestoreDC(save);
}


// CMazeEditorView printing


void CMazeEditorView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
    AFXPrintPreview(this);
#endif
}

BOOL CMazeEditorView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}

void CMazeEditorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}

void CMazeEditorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}

void CMazeEditorView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
    ClientToScreen(&point);
    OnContextMenu(this, point);
}

void CMazeEditorView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
    theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CMazeEditorView diagnostics

#ifdef _DEBUG
void CMazeEditorView::AssertValid() const
{
    CView::AssertValid();
}

void CMazeEditorView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CMazeEditorDoc* CMazeEditorView::GetDocument() const // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMazeEditorDoc)));
    return (CMazeEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CMazeEditorView message handlers

void CMazeEditorView::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    m_pDoc = GetDocument();
    m_pDoc->m_sigMazeResized.connect(bind(&CMazeEditorView::OnMazeResized, this));

    m_pBar = ((CFrameWndEx*)GetParent())->GetRibbonBar();

    m_pEditHeight = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_HEIGHT);
    m_pEditWidth = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_WIDTH);
    m_pEditChar = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_CHAR);
    m_pEditSelectedPosition = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_CURPOS);
    m_pEditMousePosition = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_MOUSEPOS);
    OnMazeResized();
    m_pComboMovement = (CMFCRibbonComboBox*)m_pBar->FindByID(ID_MAZE_MOVEMENT);
    m_pEditSideLen = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_SIDELEN);

    for (auto str : {_T("None"), _T("Up"), _T("Down"), _T("Left"), _T("Right")})
        m_pComboMovement->AddItem(str);
    m_pComboMovement->SelectItem(4);

    CString str;
    str.Format(_T("%d"), cellSize);
    m_pEditSideLen->SetEditText(str);
}

void CMazeEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
    int pointX = point.x - startX;
    int pointY = point.y - startY;
    Position p(min((pointY + tolerance) / cellSize, cols()),
        min((pointX + tolerance) / cellSize, rows()));
    bool bAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    bool bCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    if (bAlt)
        ToggleSelectedPosition(p);
    else
        SetSelectedPosition(p);
    if (bCtrl)
        if (abs(pointY - p.first * cellSize) < tolerance &&
            abs(pointX - p.second * cellSize) < tolerance)
            m_pDoc->ToggleDot(p);
        else if (abs(pointY - p.first * cellSize) < tolerance)
            m_pDoc->SetHorzWall(false, bShift);
        else if (abs(pointX - p.second * cellSize) < tolerance)
            m_pDoc->SetVertWall(false, bShift);
        else if (abs(pointY - (p.first + 1) * cellSize) < tolerance)
            m_pDoc->SetHorzWall(true, bShift);
        else if (abs(pointX - (p.second + 1) * cellSize) < tolerance)
            m_pDoc->SetVertWall(true, bShift);
}

void CMazeEditorView::MoveUp()
{
    SetSelectedPosition(m_pDoc->GetSelectedPosition() + Position(-1, 0));
}

void CMazeEditorView::MoveDown()
{
    SetSelectedPosition(m_pDoc->GetSelectedPosition() + Position(1, 0));
}

void CMazeEditorView::MoveLeft()
{
    SetSelectedPosition(m_pDoc->GetSelectedPosition() + Position(0, -1));
}

void CMazeEditorView::MoveRight()
{
    SetSelectedPosition(m_pDoc->GetSelectedPosition() + Position(0, 1));
}

void CMazeEditorView::DoSelectedPosition(Position p, function<void(const Position&)> f)
{
    int nArea = cols() * rows();
    int n = (p.first * rows() + p.second + nArea) % nArea;
    p = {n / rows(), n % rows()};
    f(p);
    CString strSelectedPosition;
    strSelectedPosition.Format(_T("%d,%d"), p.first, p.second);
    m_pEditSelectedPosition->SetEditText(strSelectedPosition);
    Invalidate();
}

void CMazeEditorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{                      
    bool bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool bCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    switch (nChar) {
    case VK_UP:
        if (bCtrl)
            m_pDoc->SetHorzWall(false, bShift);
        else
            MoveUp();
        break;
    case VK_DOWN:
        if (bCtrl)
            m_pDoc->SetHorzWall(true, bShift);
        else
            MoveDown();
        break;
    case VK_LEFT:
        if (bCtrl)
            m_pDoc->SetVertWall(false, bShift);
        else
            MoveLeft();
        break;
    case VK_RIGHT:
        if (bCtrl)
            m_pDoc->SetVertWall(true, bShift);
        else
            MoveRight();
        break;
    case VK_RETURN:
        m_pDoc->SetObject(m_pDoc->m_chLast);
        break;
    }
}

void CMazeEditorView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_BACK) {
        m_pDoc->SetObject(_T(' '));
        MoveLeft();
        return;
    }
    if (isprint(nChar))
        m_pDoc->SetObject(nChar);
    if (nChar != ' ' && nChar != VK_RETURN)
        m_pDoc->m_chLast = nChar;

    switch (m_pComboMovement->GetCurSel()) {
    case 1:
        MoveUp();
        break;
    case 2:
        MoveDown();
        break;
    case 3:
        MoveLeft();
        break;
    case 4:
        MoveRight();
        break;
    }
}

void CMazeEditorView::OnResizeMaze()
{
    static bool b = false;
    if (!b) {
        b = true;
        int h = _ttoi(m_pEditHeight->GetEditText());
        int w = m_pDoc->IsSquare() ? h : _ttoi(m_pEditWidth->GetEditText());
        m_pDoc->ResizeMaze(h, w);
        b = false;
    }
}

void CMazeEditorView::OnMazeChar()
{
    m_pDoc->m_chLast = m_pEditChar->GetEditText()[0];
}

void CMazeEditorView::OnMazeSideLen()
{
    cellSize = _ttoi(m_pEditSideLen->GetEditText());
    Invalidate();
}

void CMazeEditorView::OnEditCopy()
{
    HGLOBAL h;
    LPTSTR arr;
    CString strText = m_pDoc->GetData();
    TRACE(strText);
    TRACE("\n");

    size_t bytes = (strText.GetLength()+1)*sizeof(TCHAR);
    h=GlobalAlloc(GMEM_MOVEABLE, bytes);
    arr=(LPTSTR)GlobalLock(h);
    ZeroMemory(arr,bytes);
    _tcscpy_s(arr, strText.GetLength()+1, strText);
    GlobalUnlock(h); 

    ::OpenClipboard (NULL);
    EmptyClipboard();
#if _UNICODE
    SetClipboardData(CF_UNICODETEXT, h);
#else
    SetClipboardData(CF_TEXT, h);
#endif
    CloseClipboard();
}

void CMazeEditorView::OnEditPaste()
{
    CString str;
    if (::OpenClipboard(NULL)) {
#if _UNICODE
        HANDLE hData = ::GetClipboardData( CF_UNICODETEXT );
#else
        HANDLE hData = ::GetClipboardData( CF_TEXT );
#endif
        str = (LPTSTR)::GlobalLock( hData );
        m_pDoc->SetData(str);
        ::GlobalUnlock( hData );
        ::CloseClipboard();
    }
}

void CMazeEditorView::OnMazeResized()
{
    CString str;
    str.Format(_T("%d"), cols());
    m_pEditHeight->SetEditText(str);
    str.Format(_T("%d"), rows());
    m_pEditWidth->SetEditText(str);
}

void CMazeEditorView::OnMouseMove(UINT nFlags, CPoint point)
{

}
