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
	boost::signals2::signal<void()> m_sigMazeCleared;
	boost::signals2::signal<void()> m_sigMazeChanged;
	boost::signals2::signal<void()> m_sigMazeResized;

	int MazeWidth() {return m_szMaze.second;}
	int MazeHeight() {return m_szMaze.first;}
	bool HasWall() {return m_bHasWall;}
	bool IsHorzWall(const Position& p) {return IsWall(p, false);}
	bool IsVertWall(const Position& p) {return IsWall(p, true);}
	bool IsObject(const Position& p) {return m_mapObjects.count(p) != 0;}

// Operations
public:
	void SetHasWall(bool bHasWall);
	void SetHorzWall(const Position& p, bool bReset) {SetWall(p, false, bReset);}
	void SetVertWall(const Position& p, bool bReset) {SetWall(p, true, bReset);}
	char GetObject(const Position& p) {return IsObject(p) ? m_mapObjects[p] : ' ';}
	void SetObject(const Position& p, char ch);
	void ClearMaze();
	void ResizeMaze(int w, int h);
	CString GetData();
	void SetData(const CString& strData);
	void FillAll(char ch);
	void FillBorderCells(char ch);
	void FillBorderLines();

	void SelectPos(const Position& p);
	void SelectSinglePos(const Position& p);

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
	set<Position> m_setSelected;
	map<Position, char> m_mapObjects;

	set<Position>& GetWallSet(bool bVert) {return bVert ? m_setVertWall : m_setHorzWall;}
	bool IsWall(const Position& p, bool bVert);
	void SetWall(const Position& p, bool bVert, bool bReset);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
