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
        get {return size.row}
        set {
            size = Position(newValue, isSquare_ ? newValue : size.col)
            delegate?.updateMazeSize()
            delegate?.updateMazeView()
        }
    }
    var width: Int {
        get {return size.col}
        set {
            size = Position(size.row, newValue)
            delegate?.updateMazeSize()
            delegate?.updateMazeView()
        }
    }
    private var isSquare_ = true
    var isSquare: Bool {
        get {return isSquare_}
        set {
            isSquare_ = newValue
            delegate?.updateIsSquare()
            if newValue {width = height}
        }
    }
    var curPos = Position()
    func setCurPos(p: Position) {
        let nArea = height * width
        let n = (p.row * width + p.col + nArea) % nArea
        curPos = Position(n / width, n % width)
        delegate?.updateCurPosition()
        delegate?.updateMazeView()
    }
    private var pos2obj = [Position: Character]()
    private var hasWall_ = false
    var hasWall: Bool {
        get {return hasWall_}
        set {
            hasWall_ = newValue
            delegate?.updateHasWall()
            delegate?.updateMazeView()
        }
    }
    var horzWall = Set<Position>()
    var vertWall = Set<Position>()
    var curObj: Character = " "
    
    func getObject(p: Position) -> Character? {return pos2obj[p]}
    func setObject(p: Position, ch: Character) {
        pos2obj[p] = ch
        curObj = ch
        delegate?.updateCurObject()
        delegate?.updateMazeView()
    }
    func isHorzWall(p: Position) -> Bool {return horzWall.contains(p)}
    func isVertWall(p: Position) -> Bool {return vertWall.contains(p)}
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
    
    var data: String {
        get {
            var str = ""
            if hasWall {
                for r in 0...height {
                    for c in 0..<width {
                        let p = Position(r, c)
                        str += isHorzWall(p: p) ? " -" : "  "
                    }
                    str += " `\n"
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
                    str += " `\n"
                }
            }
            return str
        }
        set {
            clearAll()
            let strs = newValue.components(separatedBy: "`\n").filter{$0 != ""}
            if hasWall {
                size = Position(strs.count / 2, strs[0].characters.count / 2)
                for r in 0...height {
                    let str1 = strs[2 * r]
                    for c in 0..<width {
                        if str1.characters[str1.index(str1.startIndex, offsetBy: 2 * c + 1)] == "-" {
                            horzWall.insert(Position(r, c))
                        }
                    }
                    if r == height {break}
                    let str2 = strs[2 * r + 1]
                    for c in 0...width {
                        if str2.characters[str1.index(str1.startIndex, offsetBy: 2 * c)] == "|" {
                            vertWall.insert(Position(r, c))
                        }
                        if c == width {break}
                        let ch = str2.characters[str1.index(str1.startIndex, offsetBy: 2 * c + 1)]
                        if ch != " " {
                            pos2obj[Position(r, c)] = ch
                        }
                    }
                }
            } else {
                size = Position(strs.count, strs[0].characters.count)
                for r in 0..<height {
                    let str = strs[r]
                    for c in 0..<width {
                        let ch = str.characters[str.index(str.startIndex, offsetBy: c)]
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
        curPos = Position()
        pos2obj.removeAll()
        curObj = " "
        delegate?.updateCurPosition()
        delegate?.updateCurObject()
        delegate?.updateMazeView()
    }
    
    func updateMaze() {
        delegate?.updateMazeSize()
        delegate?.updateIsSquare()
        delegate?.updateCurPosition()
        delegate?.updateMazeView()
    }
    
    deinit {
        Swift.print("deinit called: \(NSStringFromClass(type(of: self)))")
    }
}

let maze = Maze()

