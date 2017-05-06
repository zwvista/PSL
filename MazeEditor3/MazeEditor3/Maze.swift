//
//  Maze.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/02.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

protocol MazeDelegate: class {
    func updateMazeSize()
    func updateMazeView()
    func updateCurPosition()
}

class Maze: NSObject {
    private var size = Position(8, 8)
    weak var delegate: MazeDelegate?
    var height: Int {
        get {return size.row}
        set {
            size = Position(newValue, size.col)
            delegate?.updateMazeSize()
            delegate?.updateMazeView()
        }
    }
    var width: Int {
        get {return size.col}
        set {size = Position(size.row, newValue)}
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
    var hasWall = false
    var horzWall = Set<Position>()
    var vertWall = Set<Position>()
    
    func getObject(p: Position) -> Character? {return pos2obj[p]}
    func setObject(p: Position, ch: Character) {pos2obj[p] = ch}
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
        delegate?.updateCurPosition()
        delegate?.updateMazeView()
    }
    
    func updateMaze() {
        delegate?.updateMazeSize()
        delegate?.updateCurPosition()
        delegate?.updateMazeView()
    }
}

let maze = Maze()

