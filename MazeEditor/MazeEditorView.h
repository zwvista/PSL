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

// MazeEditorView.h : interface of the CMazeEditorView class
//

#pragma once


class CMazeEditorView : public CView
{
protected: // create from serialization only
	CMazeEditorView();
	DECLARE_DYNCREATE(CMazeEditorView)

// Attributes
public:
	CMazeEditorDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();

protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CMazeEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CMazeEditorDoc* m_pDoc;
	CMFCRibbonBar* m_pBar;
	int m_nSideLen;
	CMFCRibbonEdit* m_pEditRows;
	CMFCRibbonEdit* m_pEditCols;
	char m_chLast;
	CMFCRibbonEdit* m_pEditChar;
	CMFCRibbonEdit* m_pEditCurPos;
	CMFCRibbonComboBox* m_pComboMovement;

	CRect GetPosRect(int r, int c) {
		return CRect(c * m_nSideLen, r * m_nSideLen, c * m_nSideLen + m_nSideLen, r * m_nSideLen + m_nSideLen);
	}
	CRect GetPosRect(const Position& p) {return GetPosRect(p.first, p.second);}
	void SetCurPos(Position p);
	void OnMazeChanged() {Invalidate();}
	void OnMazeCleared();
	void OnMazeResized();

	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnResizeMaze();
	afx_msg void OnClearMaze();
	afx_msg void OnUpdateMazeHasWall(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_pDoc->HasWall()); }
	afx_msg void OnMazeHasWallChanged();
	afx_msg void OnMazeChar();
	afx_msg void OnMazeFillAll();
	afx_msg void OnMazeFillBorderCells();
	afx_msg void OnMazeFillBorderLines();
	afx_msg void OnUpdateMazeFillBorderLines(CCmdUI* pCmdUI) { pCmdUI->Enable(m_pDoc->HasWall()); }
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateMovement(CCmdUI* pCmdUI) {pCmdUI->Enable();}
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in MazeEditorView.cpp
inline CMazeEditorDoc* CMazeEditorView::GetDocument() const
   { return reinterpret_cast<CMazeEditorDoc*>(m_pDocument); }
#endif

