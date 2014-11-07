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
	bool IsHorzWall(const Position& p) const {return IsWall(p, false);}
	bool IsVertWall(const Position& p) const { return IsWall(p, true); }
	bool IsObject(const Position& p) {return m_mapObjects.count(p) != 0;}

// Operations
public:
	void SetHasWall(bool bHasWall);
	void SetHorzWall(bool isDown, bool bReset);
	void SetVertWall(bool isRight, bool bReset);
	char GetObject(const Position& p) {return IsObject(p) ? m_mapObjects[p] : ' ';}
	void SetObject(char ch);
	void ResizeMaze(int w, int h);
	CString GetData();
	void SetData(const CString& strData);

	void AddCurPos(const Position& p);
	void SetCurPos(const Position& p);
	const Position& GetCurPos() { return *m_setCurPoss.begin(); }
	bool isCurPos(const Position& p) {
		return m_setCurPoss.count(p) != 0;
	}

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
	set<Position> m_setHorzWall, m_setVertWall;
	set<Position> m_setCurPoss;
	map<Position, char> m_mapObjects;

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
	afx_msg void OnClearMaze();
	afx_msg void OnUpdateMazeHasWall(CCmdUI* pCmdUI) { pCmdUI->SetCheck(HasWall()); }
	afx_msg void OnMazeHasWallChanged();
	afx_msg void OnMazeFillAll();
	afx_msg void OnMazeFillBorderCells();
	afx_msg void OnMazeFillBorderLines();
	afx_msg void OnEnclosedSelected();
	afx_msg void OnUpdateMazeHasWall2(CCmdUI* pCmdUI) { pCmdUI->Enable(HasWall()); }
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
