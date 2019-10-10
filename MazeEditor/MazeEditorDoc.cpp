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

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

// CMazeEditorDoc

IMPLEMENT_DYNCREATE(CMazeEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CMazeEditorDoc, CDocument)
    ON_COMMAND(ID_MAZE_CLEAR_ALL, &CMazeEditorDoc::OnMazeClearAll)
    ON_COMMAND(ID_MAZE_CLEAR_WALLS, &CMazeEditorDoc::OnMazeClearWalls)
    ON_COMMAND(ID_MAZE_CLEAR_CHARS, &CMazeEditorDoc::OnMazeClearChars)
    ON_COMMAND(ID_MAZE_HAS_WALL, &CMazeEditorDoc::OnMazeHasWallChanged)
    ON_UPDATE_COMMAND_UI(ID_MAZE_HAS_WALL, &CMazeEditorDoc::OnUpdateMazeHasWall)
    ON_COMMAND(ID_MAZE_FILL_ALL, &CMazeEditorDoc::OnMazeFillAll)
    ON_COMMAND(ID_MAZE_FILL_BORDER_CELLS, &CMazeEditorDoc::OnMazeFillBorderCells)
    ON_COMMAND(ID_MAZE_FILL_BORDER_LINES, &CMazeEditorDoc::OnMazeFillBorderLines)
    ON_UPDATE_COMMAND_UI(ID_MAZE_FILL_BORDER_LINES, &CMazeEditorDoc::OnUpdateMazeHasWall2)
    ON_COMMAND(ID_MAZE_ENCLOSE_SELECTED, &CMazeEditorDoc::OnEnclosedSelected)
    ON_UPDATE_COMMAND_UI(ID_MAZE_ENCLOSE_SELECTED, &CMazeEditorDoc::OnUpdateMazeHasWall2)
    ON_COMMAND(ID_MAZE_SQUARE, &CMazeEditorDoc::OnMazeSquare)
    ON_UPDATE_COMMAND_UI(ID_MAZE_WIDTH, &CMazeEditorDoc::OnUpdateMazeWidth)
    ON_UPDATE_COMMAND_UI(ID_MAZE_SQUARE, &CMazeEditorDoc::OnUpdateMazeSquare)
END_MESSAGE_MAP()


// CMazeEditorDoc construction/destruction

CMazeEditorDoc::CMazeEditorDoc()
    : m_szMaze(8, 8)
    , m_bHasWall(false)
    , m_nSideLen(40)
    , m_chLast(' ')
    , m_bIsSquare(true)
{
    m_vecSelectedPositions.emplace_back(0, 0);
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
    if (m_bHasWall)
        for (int r = 0;; r++) {
            for (int c = 0; c < MazeWidth(); c++) {
                Position p(r, c);
                str += IsHorzWall(p) ? _T(" -") : _T("  ");
            }
            str += " `\r\n";
            if (r == MazeHeight()) break;
            for (int c = 0; ; c++) {
                Position p(r, c);
                str += IsVertWall(p) ? _T("|") : _T(" ");
                if (c == MazeWidth()) break;
                str += IsObject(p) ? CString(GetObject(p), 1) : _T(" ");
            }
            str += "`\r\n";
        }
    else
        for (int r = 0; r < MazeHeight(); ++r) {
            for (int c = 0; c < MazeWidth(); ++c) {
                Position p(r, c);
                str += IsObject(p) ? CString(GetObject(p), 1) : _T(" ");
            }
            str += "`\r\n";
        }
    return str;
}

bool CMazeEditorDoc::IsWall(const Position& p, bool bVert) const
{
    return m_bHasWall && GetWallSet(bVert).count(p) != 0;
}

void CMazeEditorDoc::SetWall(bool isDownOrRight, bool bVert, bool bReset)
{
    if (!m_bHasWall) return;
    for (auto& p : m_vecSelectedPositions)
        SetWall(p, isDownOrRight, bVert, bReset);
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::SetHorzWall(bool isDown, bool bReset)
{
    if (!m_bHasWall) return;
    SetWall(isDown, false, bReset);
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::SetVertWall(bool isRight, bool bReset)
{
    if (!m_bHasWall) return;
    SetWall(isRight, true, bReset);
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::SetWall(Position p, bool isDownOrRight, bool bVert, bool bReset)
{
    auto& rng = GetWallSet(bVert);
    if (isDownOrRight)
        p += bVert ? Position(0, 1) : Position(1, 0);
    bReset ? rng.erase(p) : (void)rng.insert(p);
}

void CMazeEditorDoc::SetHasWall( bool bHasWall )
{
    m_bHasWall = bHasWall;
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::ToggleDot(const Position& p)
{
    if (m_setDots.count(p) == 0)
        m_setDots.insert(p);
    else
        m_setDots.erase(p);
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::SetObject( char ch )
{
    for (const auto& p : m_vecSelectedPositions)
        SetObject(p, ch);
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::SetObject(const Position& p, char ch)
{
    ch == ' ' ? m_mapObjects.erase(p) : (m_mapObjects[p] = ch);
}

void CMazeEditorDoc::OnMazeClearAll()
{
    OnMazeClearWalls();
    OnMazeClearChars();
}

void CMazeEditorDoc::OnMazeClearWalls()
{
    m_setHorzWall.clear();
    m_setVertWall.clear();
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::OnMazeClearChars()
{
    m_mapObjects.clear();
    m_vecSelectedPositions.clear();
    m_vecSelectedPositions.emplace_back(0, 0);
    m_chLast = ' ';
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::ResizeMaze( int h, int w )
{
    m_szMaze = Position(h, w);
    m_sigMazeResized();
    OnMazeClearAll();
}

void CMazeEditorDoc::OnMazeHasWallChanged()
{
    SetHasWall(!HasWall());
}

void CMazeEditorDoc::OnMazeFillAll()
{
    for (int r = 0; r < MazeHeight(); ++r)
        for (int c = 0; c < MazeWidth(); ++c)
            m_mapObjects[{r, c}] = m_chLast;
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::OnMazeFillBorderCells()
{
    for (int r = 0; r < MazeHeight(); ++r)
        m_mapObjects[{r, 0}] =
        m_mapObjects[{r, MazeWidth() - 1}] = m_chLast;
    for (int c = 0; c < MazeWidth(); ++c)
        m_mapObjects[{0, c}] =
        m_mapObjects[{MazeHeight() - 1, c}] = m_chLast;
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::OnMazeFillBorderLines()
{
    for (int r = 0; r < MazeHeight(); ++r)
        m_setVertWall.insert({r, 0}),
        m_setVertWall.insert({r, MazeWidth()});
    for (int c = 0; c < MazeWidth(); ++c)
        m_setHorzWall.insert({0, c}),
        m_setHorzWall.insert({MazeHeight(), c});
    UpdateAllViews(NULL);
}

void SplitString( const CString& strText, LPCTSTR pszDelim, vector<CString>& vstrs )
{
    CString str;
    vstrs.clear();
    for (int p1 = 0, p2 = 0;;) {
        p2 = strText.Find(pszDelim, p1);
        if (p2 == -1) {
            str = strText.Mid(p1);
            if (!str.IsEmpty())
                vstrs.push_back(str);
            break;
        }
        str = strText.Mid(p1, p2 - p1);
        if (!str.IsEmpty())
            vstrs.push_back(str);
        p1 = p2 + _tcslen(pszDelim);
    }
}

void CMazeEditorDoc::SetData( const CString& strData )
{
    vector<CString> vstrs;
    SplitString(strData, _T("`\r\n"), vstrs);
    if (m_bHasWall) {
        ResizeMaze(vstrs.size() / 2, vstrs[0].GetLength() / 2);
        for (int r = 0;; r++) {
            const CString& str1 = vstrs[2 * r];
            for (int c = 0; c < MazeWidth(); c++)
                if (str1[2 * c + 1] == '-')
                    SetHorzWall({r, c}, false);
            if (r == MazeHeight()) break;
            const CString& str2 = vstrs[2 * r + 1];
            for (int c = 0; ; c++) {
                Position p(r, c);
                if (str2[2 * c] == '|')
                    SetVertWall({r, c}, false);
                if (c == MazeWidth()) break;
                char ch = str2[2 * c + 1];
                if (ch != ' ')
                    SetObject({r, c}, ch);
            }
        }
    }
    else{
        ResizeMaze(vstrs.size(), vstrs[0].GetLength());
        for (int r = 0; r < MazeHeight(); ++r) {
            const CString& str = vstrs[r];
            for (int c = 0; c < MazeWidth(); ++c) {
                char ch = str[c];
                if (ch != ' ')
                    SetObject({r, c}, ch);
            }
        }
    }
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::ToggleSelectedPosition(const Position& p)
{
    if (!IsSelectedPosition(p))
        m_vecSelectedPositions.push_back(p);
    else if (m_vecSelectedPositions.size() > 1)
        boost::remove_erase(m_vecSelectedPositions, p);
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::SetSelectedPosition(const Position& p)
{
    m_vecSelectedPositions.clear();
    ToggleSelectedPosition(p);
}

void CMazeEditorDoc::OnEnclosedSelected()
{
    for (const auto& p : m_vecSelectedPositions) {
        auto p2 = p + offset[0];
        if (!IsSelectedPosition(p2))
            SetHorzWall(p, false);
        p2 = p + offset[2];
        if (!IsSelectedPosition(p2))
            SetHorzWall(p2, false);
        p2 = p + offset[3];
        if (!IsSelectedPosition(p2))
            SetVertWall(p, false);
        p2 = p + offset[1];
        if (!IsSelectedPosition(p2))
            SetVertWall(p2, false);
    }
    UpdateAllViews(NULL);
}

void CMazeEditorDoc::OnMazeSquare()
{
    if ((m_bIsSquare = !m_bIsSquare) && MazeHeight() != MazeWidth())
        ResizeMaze(MazeHeight(), MazeHeight());
}
