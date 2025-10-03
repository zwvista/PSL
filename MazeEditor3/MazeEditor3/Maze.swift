//
//  Maze.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/02.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

class Maze: NSObject {
    private var size = Position(8, 8)
    weak var delegate: MazeDelegate?
    var height: Int {
        get { size.row }
        set {
            size = Position(newValue, isSquare_ ? newValue : size.col)
            delegate?.updateMazeSize()
            delegate?.updateMazeView()
        }
    }
    var width: Int {
        get { size.col }
        set {
            size = Position(size.row, newValue)
            delegate?.updateMazeSize()
            delegate?.updateMazeView()
        }
    }
    private var isSquare_ = true
    var isSquare: Bool {
        get { isSquare_ }
        set {
            isSquare_ = newValue
            delegate?.updateIsSquare()
            if newValue {width = height}
        }
    }
    var selectedPositions = [Position()]
    var selectedPosition: Position {
        return selectedPositions.last!
    }
    private func adjustedPosition(p: Position) -> Position {
        let nArea = height * width
        let n = (p.row * width + p.col + nArea) % nArea
        return Position(n / width, n % width)
    }
    func setSelectedPosition(p: Position) {
        selectedPositions = [adjustedPosition(p: p)]
        delegate?.updateSelectedPosition()
        delegate?.updateMazeView()
    }
    func toggleSelectedPosition(p: Position) {
        let p2 = adjustedPosition(p: p)
        if let i = selectedPositions.index(of: p2) {
            if selectedPositions.count > 1 {
                selectedPositions.remove(at: i)
            }
        } else {
            selectedPositions.append(p2)
        }
        delegate?.updateSelectedPosition()
        delegate?.updateMazeView()
    }
    private var pos2obj = [Position: Character]()
    private var hasWall_ = false
    var hasWall: Bool {
        get { hasWall_ }
        set {
            hasWall_ = newValue
            delegate?.updateHasWall()
            delegate?.updateMazeView()
        }
    }
    var horzWall = Set<Position>()
    var vertWall = Set<Position>()
    var dots = Set<Position>()
    var curObj: Character = " "
    
    func getObject(p: Position) -> Character? { pos2obj[p] }
    func setObject(p: Position, ch: Character) {
        pos2obj[p] = ch
        curObj = ch
        delegate?.updateCurObject()
        delegate?.updateMazeView()
    }
    func isHorzWall(p: Position) -> Bool { horzWall.contains(p) }
    func isVertWall(p: Position) -> Bool { vertWall.contains(p) }
    func isDot(p: Position) -> Bool { dots.contains(p) }
    func toggleHorzWall(p: Position) {
        if horzWall.contains(p) {
            horzWall.remove(p)
        } else {
            horzWall.insert(p)
        }
        delegate?.updateMazeView()
    }
    func toggleVertWall(p: Position) {
        if vertWall.contains(p) {
            vertWall.remove(p)
        } else {
            vertWall.insert(p)
        }
        delegate?.updateMazeView()
    }
    func toggleDot(p: Position) {
        if dots.contains(p) {
            dots.remove(p)
        } else {
            dots.insert(p)
        }
        delegate?.updateMazeView()
    }
    
    func fillBorderLines() {
        guard hasWall else {return}
        for r in 0..<height {
            vertWall.insert(Position(r, 0))
            vertWall.insert(Position(r, width))
        }
        for c in 0..<width {
            horzWall.insert(Position(0, c))
            horzWall.insert(Position(height, c))
        }
        delegate?.updateMazeView()
    }
    
    func encloseSelectedCells() {
        guard hasWall else {return}
        for p in selectedPositions {
            if !selectedPositions.contains(Position(p.row - 1, p.col)) { horzWall.insert(p) }
            if !selectedPositions.contains(Position(p.row + 1, p.col)) { horzWall.insert(Position(p.row + 1, p.col)) }
            if !selectedPositions.contains(Position(p.row, p.col - 1)) { vertWall.insert(p) }
            if !selectedPositions.contains(Position(p.row, p.col + 1)) { vertWall.insert(Position(p.row, p.col + 1)) }
        }
        delegate?.updateMazeView()
    }
    
    var data: String {
        get {
            var str = ""
            if hasWall {
                for r in 0...height {
                    for c in 0..<width {
                        let p = Position(r, c)
                        str += isDot(p: p) ? "O" : " "
                        str += isHorzWall(p: p) ? "-" : " "
                    }
                    str += isDot(p: Position(r, width)) ? "O" : " "
                    str += "`\n"
                    if r == height {break}
                    for c in 0...width {
                        let p = Position(r, c)
                        str += isVertWall(p: p) ? "|" : " "
                        if c == width {break}
                        let ch = getObject(p: p)
                        str += ch != nil ? String(ch!) : " "
                    }
                    str += "`\n"
                }
            } else {
                for r in 0..<height {
                    for c in 0..<width {
                        let p = Position(r, c)
                        let ch = getObject(p: p)
                        str += ch != nil ? String(ch!) : " "
                    }
                    str += "`\n"
                }
            }
            return str
        }
        set {
            clearAll()
            let strs = newValue.split { $0.isNewline || $0 == "`" }.map(String.init)
            if hasWall {
                size = Position(strs.count / 2, strs[0].count / 2)
                for r in 0...height {
                    let str1 = strs[2 * r]
                    for c in 0..<width {
                        if str1[2 * c] == "O" {
                            dots.insert(Position(r, c))
                        }
                        if str1[2 * c + 1] == "-" {
                            horzWall.insert(Position(r, c))
                        }
                    }
                    if str1[2 * width] == "O" {
                        dots.insert(Position(r, width))
                    }
                    if r == height {break}
                    let str2 = strs[2 * r + 1]
                    for c in 0...width {
                        if str2[2 * c] == "|" {
                            vertWall.insert(Position(r, c))
                        }
                        if c == width {break}
                        let ch = str2[2 * c + 1]
                        if ch != " " {
                            pos2obj[Position(r, c)] = ch
                        }
                    }
                }
            } else {
                size = Position(strs.count, strs[0].count)
                for r in 0..<height {
                    let str = strs[r]
                    for c in 0..<width {
                        let ch = str[c]
                        if ch != " " {
                            pos2obj[Position(r, c)] = ch
                        }
                    }
                }
            }
            updateMaze()
        }
    }
    
    func clearAll() {
        clearWalls()
        clearChars()
    }
    
    func clearWalls() {
        horzWall.removeAll()
        vertWall.removeAll()
        delegate?.updateMazeView()
    }
    
    func clearChars() {
        selectedPositions = [Position()]
        pos2obj.removeAll()
        curObj = " "
        delegate?.updateSelectedPosition()
        delegate?.updateCurObject()
        delegate?.updateMazeView()
    }
    
    func updateMaze() {
        delegate?.updateMazeSize()
        delegate?.updateIsSquare()
        delegate?.updateSelectedPosition()
        delegate?.updateMazeView()
        delegate?.updateHasWall()
    }
    
    deinit {
        Swift.print("deinit called: \(NSStringFromClass(type(of: self)))")
    }
}

let maze = Maze()

