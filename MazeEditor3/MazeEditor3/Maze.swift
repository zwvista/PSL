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
    var height: Int {
        get {return size.row}
        set {size = Position(newValue, size.col)}
    }
    var width: Int {
        get {return size.col}
        set {size = Position(size.row, newValue)}
    }
    var curPos = Position()
    func setCurPos(p: Position) {
        let nArea = height * width
        let n = (p.row * height + p.col + nArea) % nArea
        curPos = Position(n / height, n % height)
    }
    private var pos2obj = [Position: Character]()
    var hasWall = false
    var horzWall = Set<Position>()
    var vertWall = Set<Position>()
    
    func getObject(p: Position) -> Character? {return pos2obj[p]}
    func setObject(p: Position, ch: Character) {pos2obj[p] = ch}
    func isHorzWall(p: Position) -> Bool {return horzWall.contains(p)}
    func isVertWall(p: Position) -> Bool {return vertWall.contains(p)}
    
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
                }
            } else {
                for r in 0..<height {
                    for c in 0..<width {
                        let p = Position(r, c)
                        let ch = getObject(p: p)
                        str += ch != nil ? String(ch!) : " "
                    }
                }
            }
            return str
        }
    }
}

let maze = Maze()

