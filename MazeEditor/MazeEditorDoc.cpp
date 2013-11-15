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

// MazeEditorDoc.cpp : implementation of the CMazeEditorDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "MazeEditor.h"
#endif

#include "MazeEditorDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMazeEditorDoc

IMPLEMENT_DYNCREATE(CMazeEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CMazeEditorDoc, CDocument)
END_MESSAGE_MAP()


// CMazeEditorDoc construction/destruction

CMazeEditorDoc::CMazeEditorDoc()
	: m_szMaze(8, 8)
	, m_bHasWall(false)
{
}

CMazeEditorDoc::~CMazeEditorDoc()
{
}

BOOL CMazeEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CMazeEditorDoc serialization

void CMazeEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CMazeEditorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CMazeEditorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CMazeEditorDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CMazeEditorDoc diagnostics

#ifdef _DEBUG
void CMazeEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMazeEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMazeEditorDoc commands

CString CMazeEditorDoc::GetData()
{
	CString str;
	if(m_bHasWall)
		for(int r = 0;; r++){
			for(int c = 0; c < MazeWidth(); c++){
				Position p(r, c);
				str += IsHorzWall(p) ? _T(" -") : _T("  ");
			}
			str += " \\\r\n";
			if(r == MazeHeight()) break;
			for(int c = 0; ; c++){
				Position p(r, c);
				str += IsVertWall(p) ? _T("|") : _T(" ");
				if(c == MazeWidth()) break;
				str += IsObject(p) ? CString(GetObject(p), 1) : _T(" ");
			}
			str += "\\\r\n";
		}
	else
		for(int r = 0; r < MazeHeight(); ++r){
			for(int c = 0; c < MazeWidth(); ++c){
				Position p(r, c);
				str += IsObject(p) ? CString(GetObject(p), 1) : _T(" ");
			}
			str += "\\\r\n";
		}
		return str;
}

bool CMazeEditorDoc::IsWall( const Position& p, bool bVert )
{
	 return m_bHasWall && GetWallSet(bVert).count(p) != 0;
}

void CMazeEditorDoc::SetWall( const Position& p, bool bVert, bool bReset )
{
	if(!m_bHasWall) return;
	set<Position>& s = GetWallSet(bVert);
	bReset ? s.erase(p) : (void)s.insert(p);
	m_sigMazeChanged();
}

void CMazeEditorDoc::SetHasWall( bool bHasWall )
{
	m_bHasWall = bHasWall;
	ClearMaze();
}

void CMazeEditorDoc::SetObject( const Position& p, char ch )
{
	ch == ' ' ? m_mapObjects.erase(p) : (m_mapObjects[p] = ch);
	m_sigMazeChanged();
}

void CMazeEditorDoc::ClearMaze()
{
	m_setHorzWall.clear();
	m_setVertWall.clear();
	m_mapObjects.clear();
	m_sigMazeCleared();
	m_sigMazeChanged();
}

void CMazeEditorDoc::ResizeMaze( int w, int h )
{
	m_szMaze = Position(h, w);
	m_sigMazeResized();
	ClearMaze();
}

void CMazeEditorDoc::FillAll( char ch )
{
	for(int r = 0; r < MazeHeight(); ++r)
		for(int c = 0; c < MazeWidth(); ++c)
			m_mapObjects[Position(r, c)] = ch;
	m_sigMazeChanged();
}

void CMazeEditorDoc::FillBorder( char ch )
{
	for(int r = 0; r < MazeHeight(); ++r)
		m_mapObjects[Position(r, 0)] =
		m_mapObjects[Position(r, MazeWidth() - 1)] = ch;
	for(int c = 0; c < MazeWidth(); ++c)
		m_mapObjects[Position(0, c)] =
		m_mapObjects[Position(MazeHeight() - 1, c)] = ch;
	m_sigMazeChanged();
}

void SplitString( const CString& strText, LPCTSTR pszDelim, vector<CString>& vstrs )
{
	CString str;
	vstrs.clear();
	for(int p1 = 0, p2 = 0;;){
		p2 = strText.Find(pszDelim, p1);
		if(p2 == -1){
			str = strText.Mid(p1);
			if(!str.IsEmpty())
				vstrs.push_back(str);
			break;
		}
		str = strText.Mid(p1, p2 - p1);
		if(!str.IsEmpty())
			vstrs.push_back(str);
		p1 = p2 + _tcslen(pszDelim);
	}
}

void CMazeEditorDoc::SetData( const CString& strData )
{
	vector<CString> vstrs;
	SplitString(strData, _T("\\\r\n"), vstrs);
	if(m_bHasWall){
		ResizeMaze(vstrs[0].GetLength() / 2, vstrs.size() / 2);
		for(int r = 0;; r++){
			const CString& str1 = vstrs[2 * r];
			for(int c = 0; c < MazeWidth(); c++)
				if(str1[2 * c + 1] == '-')
					SetHorzWall(Position(r, c), false);
			if(r == MazeHeight()) break;
			const CString& str2 = vstrs[2 * r + 1];
			for(int c = 0; ; c++){
				Position p(r, c);
				if(str2[2 * c] == '|')
					SetVertWall(Position(r, c), false);
				if(c == MazeWidth()) break;
				char ch = str2[2 * c + 1];
				if(ch != ' ')
					SetObject(Position(r, c), ch);
			}
		}
	}
	else{
		ResizeMaze(vstrs[0].GetLength(), vstrs.size());
		for(int r = 0; r < MazeHeight(); ++r){
			const CString& str = vstrs[r];
			for(int c = 0; c < MazeWidth(); ++c){
				char ch = str[c];
				if(ch != ' ')
					SetObject(Position(r, c), ch);
			}
		}
	}
	m_sigMazeChanged();
}