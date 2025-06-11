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

// MazeEditorDoc.h : interface of the CMazeEditorDoc class
//


#pragma once


class CMazeEditorDoc : public CDocument
{
protected: // create from serialization only
    CMazeEditorDoc();
    DECLARE_DYNCREATE(CMazeEditorDoc)

// Attributes
public:
    boost::signals2::signal<void()> m_sigMazeResized;
    char m_chLast;
    int m_nSideLen;

    int MazeWidth() const {return m_szMaze.second;}
    int MazeHeight() const { return m_szMaze.first; }
    bool HasWall() const { return m_bHasWall; }
    bool IsHorzWall(const Position& p) const { return IsWall(p, false); }
    bool IsVertWall(const Position& p) const { return IsWall(p, true); }
    bool IsObject(const Position& p) const { return m_mapObjects.contains(p); }
    bool IsDot(const Position& p) const { return m_bHasWall && m_setDots.contains(p); }

// Operations
public:
    void SetHasWall(bool bHasWall);
    void SetHorzWall(bool isDown, bool bReset);
    void SetVertWall(bool isRight, bool bReset);
    void ToggleDot(const Position& p);
    char GetObject(const Position& p) {return IsObject(p) ? m_mapObjects[p] : ' ';}
    void SetObject(char ch);
    void ResizeMaze(int h, int w);
    CString GetData();
    void SetData(const CString& strData);

    void ToggleSelectedPosition(const Position& p);
    void SetSelectedPosition(const Position& p);
    const Position& GetSelectedPosition() { return *m_vecSelectedPositions.begin(); }
    bool IsSelectedPosition(const Position& p) {
        return boost::algorithm::any_of_equal(m_vecSelectedPositions, p);
    }
    bool IsSquare() { return m_bIsSquare; }

// Overrides
public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
    virtual void InitializeSearchContent();
    virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
    virtual ~CMazeEditorDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    Position m_szMaze;
    bool m_bHasWall;
    set<Position> m_setHorzWall, m_setVertWall, m_setDots;
    vector<Position> m_vecSelectedPositions;
    map<Position, char> m_mapObjects;
    bool m_bIsSquare;

    const set<Position>& GetWallSet(bool bVert) const { return bVert ? m_setVertWall : m_setHorzWall; }
    set<Position>& GetWallSet(bool bVert) { return bVert ? m_setVertWall : m_setHorzWall; }
    bool IsWall(const Position& p, bool bVert) const;
    void SetHorzWall(const Position& p, bool bReset) { SetWall(p, false, false, bReset); }
    void SetVertWall(const Position& p, bool bReset) { SetWall(p, false, true, bReset); }
    void SetWall(bool isDownOrRight, bool bVert, bool bReset);
    void SetWall(Position p, bool isDownOrRight, bool bVert, bool bReset);
    void SetObject(const Position& p, char ch);
    bool IsValid(const Position& p) const {
        return p.first >= 0 && p.first < MazeHeight() && p.second >= 0 && p.second < MazeWidth();
    }

// Generated message map functions
protected:
    afx_msg void OnMazeClearAll();
    afx_msg void OnMazeClearWalls();
    afx_msg void OnMazeClearChars();
    afx_msg void OnUpdateMazeHasWall(CCmdUI* pCmdUI) { pCmdUI->SetCheck(HasWall()); }
    afx_msg void OnMazeHasWallChanged();
    afx_msg void OnMazeFillAll();
    afx_msg void OnMazeFillBorderCells();
    afx_msg void OnMazeFillBorderLines();
    afx_msg void OnEnclosedSelected();
    afx_msg void OnUpdateMazeHasWall2(CCmdUI* pCmdUI) { pCmdUI->Enable(HasWall()); }
    afx_msg void OnMazeSquare();
    afx_msg void OnUpdateMazeWidth(CCmdUI* pCmdUI) { pCmdUI->Enable(!IsSquare()); }
    afx_msg void OnUpdateMazeSquare(CCmdUI* pCmdUI) { pCmdUI->SetCheck(IsSquare()); }
    DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
    // Helper function that sets search content for a Search Handler
    void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
