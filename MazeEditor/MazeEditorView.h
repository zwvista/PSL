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
    CMFCRibbonEdit* m_pEditHeight;
    CMFCRibbonEdit* m_pEditWidth;
    CMFCRibbonEdit* m_pEditChar;
    CMFCRibbonEdit* m_pEditSelectedPosition;
    CMFCRibbonEdit* m_pEditMousePosition;
    CMFCRibbonComboBox* m_pComboMovement;
    CMFCRibbonEdit* m_pEditSideLen;
    int cellSize, boardWidth, boardHeight, startX, startY;
    const int tolerance = 8;

    int rows() const { return m_pDoc->MazeHeight(); }
    int cols() const { return m_pDoc->MazeWidth(); }
    void OnMazeChanged() {Invalidate();}
    void OnMazeResized();

    void MoveUp();
    void MoveDown();
    void MoveLeft();
    void MoveRight();
    void DoSelectedPosition(Position p, function<void(const Position&)> f);
    void SetSelectedPosition(Position p) {
        DoSelectedPosition(p, [&](const Position& p2) { m_pDoc->SetSelectedPosition(p2); });
    }
    void ToggleSelectedPosition(Position p) {
        DoSelectedPosition(p, [&](const Position& p2) { m_pDoc->ToggleSelectedPosition(p2); });
    }

// Generated message map functions
protected:
    afx_msg void OnFilePrintPreview();
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnResizeMaze();
    afx_msg void OnMazeChar();
    afx_msg void OnMazeSideLen();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateMovement(CCmdUI* pCmdUI) {pCmdUI->Enable();}
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in MazeEditorView.cpp
inline CMazeEditorDoc* CMazeEditorView::GetDocument() const
   { return reinterpret_cast<CMazeEditorDoc*>(m_pDocument); }
#endif

