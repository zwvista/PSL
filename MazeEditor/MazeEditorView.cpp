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
	ON_COMMAND(ID_MAZE_ROWS, &CMazeEditorView::OnResizeMaze)
	ON_COMMAND(ID_MAZE_COLS, &CMazeEditorView::OnResizeMaze)
	ON_COMMAND(ID_MAZE_CLEAR, &CMazeEditorView::OnClearMaze)
	ON_COMMAND(ID_MAZE_HAS_WALL, &CMazeEditorView::OnMazeHasWallChanged)
	ON_UPDATE_COMMAND_UI(ID_MAZE_HAS_WALL, &CMazeEditorView::OnUpdateMazeHasWall)
	ON_COMMAND(ID_MAZE_CHAR, &CMazeEditorView::OnMazeChar)
	ON_COMMAND(ID_MAZE_FILL_ALL, &CMazeEditorView::OnMazeFillAll)
	ON_COMMAND(ID_MAZE_FILL_BORDER_CELLS, &CMazeEditorView::OnMazeFillBorderCells)
	ON_COMMAND(ID_MAZE_FILL_BORDER_LINES, &CMazeEditorView::OnMazeFillBorderLines)
	ON_UPDATE_COMMAND_UI(ID_MAZE_FILL_BORDER_LINES, &CMazeEditorView::OnUpdateMazeFillBorderLines)
	ON_COMMAND(ID_EDIT_COPY, &CMazeEditorView::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &CMazeEditorView::OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_MAZE_MOVEMENT, &CMazeEditorView::OnUpdateMovement)
END_MESSAGE_MAP()

// CMazeEditorView construction/destruction

CMazeEditorView::CMazeEditorView()
	: m_posCur(0, 0)
	, m_nSideLen(40)
	, m_pDoc(NULL)
	, m_pBar(NULL)
	, m_pEditRows(NULL)
	, m_pEditCols(NULL)
	, m_chLast(' ')
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
	CMemDC2 memdc(pDC);

	COLORREF clrFill = RGB(0, 200, 0);
	COLORREF clrFill2 = RGB(200, 0, 0);
	COLORREF clrWall = RGB(128, 0, 0);
	CPen penWall(PS_SOLID, 5, clrWall);
	COLORREF clrNone = RGB(0, 0, 0);
	CPen penNone(PS_SOLID, 1, clrNone);

	int save = memdc.SaveDC();
	memdc.SetBkMode(TRANSPARENT);
	memdc.FillSolidRect(GetPosRect(m_posCur), clrFill);
	for(int r = 0;; r++){
		for(int c = 0; c < m_pDoc->MazeWidth(); c++){
			Position p(r, c);
			memdc.SelectObject(&(m_pDoc->IsHorzWall(p) ? penWall : penNone));
			memdc.MoveTo(c * m_nSideLen, r * m_nSideLen);
			memdc.LineTo((c + 1) * m_nSideLen, r * m_nSideLen);

			if(m_pDoc->IsObject(p)){
				char ch = m_pDoc->GetObject(p);
				if(ch == '#' && p != m_posCur)
					memdc.FillSolidRect(GetPosRect(p), clrFill2);
				CString str(ch, 1);
				memdc.DrawText(str, GetPosRect(p), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
		}
		if(r == m_pDoc->MazeHeight()) break;
		for(int c = 0; c < m_pDoc->MazeWidth() + 1; c++){
			memdc.SelectObject(&(m_pDoc->IsVertWall(Position(r, c)) ? penWall : penNone));
			memdc.MoveTo(c * m_nSideLen, r * m_nSideLen);
			memdc.LineTo(c * m_nSideLen, (r + 1) * m_nSideLen);
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
	m_pDoc->m_sigMazeCleared.connect(bind(&CMazeEditorView::OnMazeCleared, this));
	m_pDoc->m_sigMazeChanged.connect(bind(&CMazeEditorView::OnMazeChanged, this));
	m_pDoc->m_sigMazeResized.connect(bind(&CMazeEditorView::OnMazeResized, this));

	m_pBar = ((CFrameWndEx*)GetParent())->GetRibbonBar();

	m_pEditRows = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_ROWS);
	m_pEditCols = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_COLS);
	m_pEditChar = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_CHAR);
	m_pEditCurPos = (CMFCRibbonEdit*)m_pBar->FindByID(ID_MAZE_CURPOS);
	OnMazeResized();
	SetCurPos(Position(0, 0));
	m_pComboMovement = (CMFCRibbonComboBox*)m_pBar->FindByID(ID_MAZE_MOVEMENT);

	for(auto str : {_T("None"), _T("Up"), _T("Down"), _T("Left"), _T("Right")})
		m_pComboMovement->AddItem(str);
	m_pComboMovement->SelectItem(4);
}

void CMazeEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCurPos(Position(point.y / m_nSideLen, point.x / m_nSideLen));
}

void CMazeEditorView::MoveUp()
{
	SetCurPos(m_posCur + Position(-1, 0)); 
}

void CMazeEditorView::MoveDown()
{
	SetCurPos(m_posCur + Position(1, 0));
}

void CMazeEditorView::MoveLeft()
{
	SetCurPos(m_posCur + Position(0, -1));
}

void CMazeEditorView::MoveRight()
{
	SetCurPos(m_posCur + Position(0, 1));
}

void CMazeEditorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool bShift = GetKeyState(VK_SHIFT) & 0x8000 ? true : false;
	bool bCtrl = GetKeyState(VK_CONTROL) & 0x8000 ? true : false;
	switch(nChar){
	case VK_UP:
		if(bCtrl)
			m_pDoc->SetHorzWall(m_posCur, bShift);
		else
			MoveUp();
		break;
	case VK_DOWN:
		if(bCtrl)
			m_pDoc->SetHorzWall(m_posCur + Position(1, 0), bShift);
		else
			MoveDown();
		break;
	case VK_LEFT:
		if(bCtrl)
			m_pDoc->SetVertWall(m_posCur, bShift);
		else
			MoveLeft();
		break;
	case VK_RIGHT:
		if(bCtrl)
			m_pDoc->SetVertWall(m_posCur + Position(0, 1), bShift);
		else
			MoveRight();
		break;
	case VK_RETURN:
		m_pDoc->SetObject(m_posCur, m_chLast);
		break;
	}
}

void CMazeEditorView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(isprint(nChar))
		m_pDoc->SetObject(m_posCur, nChar);
	if(nChar != ' ' && nChar != VK_RETURN)
		m_chLast = nChar;

	switch(m_pComboMovement->GetCurSel()){
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
	if(!b){
		b = true;
		int rows = _ttoi(m_pEditRows->GetEditText());
		int cols = _ttoi(m_pEditCols->GetEditText());
		m_pDoc->ResizeMaze(cols, rows);
		b = false;
	}
}

void CMazeEditorView::OnClearMaze()
{
	m_pDoc->ClearMaze();
}

void CMazeEditorView::OnMazeHasWallChanged()
{
	m_pDoc->SetHasWall(!m_pDoc->HasWall());
}

void CMazeEditorView::OnMazeCleared()
{
	m_chLast = ' ';
	SetCurPos(Position(0, 0));
}

void CMazeEditorView::OnMazeFillAll()
{
	m_pDoc->FillAll(m_chLast);
}

void CMazeEditorView::OnMazeFillBorderCells()
{
	m_pDoc->FillBorderCells(m_chLast);
}

void CMazeEditorView::OnMazeFillBorderLines()
{
	m_pDoc->FillBorderLines();
}

void CMazeEditorView::OnMazeChar()
{
	m_chLast = m_pEditChar->GetEditText()[0];
}

void CMazeEditorView::SetCurPos( Position p )
{
	//if(p.first < 0 || p.first >= m_pDoc->MazeHeight() ||
	//	p.second < 0 || p.second >= m_pDoc->MazeWidth()) return;

	int nArea = m_pDoc->MazeHeight() * m_pDoc->MazeWidth();
	int n = (p.first * m_pDoc->MazeWidth() + p.second + nArea) % nArea;
	p = {n / m_pDoc->MazeWidth(), n % m_pDoc->MazeWidth()};

	m_posCur = p;
	CString strCurPos;
	strCurPos.Format(_T("%d,%d"), p.first, p.second);
	m_pEditCurPos->SetEditText(strCurPos);
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
	if(::OpenClipboard(NULL)){
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
	str.Format(_T("%d"), m_pDoc->MazeHeight());
	m_pEditRows->SetEditText(str);
	str.Format(_T("%d"), m_pDoc->MazeWidth());
	m_pEditCols->SetEditText(str);
}