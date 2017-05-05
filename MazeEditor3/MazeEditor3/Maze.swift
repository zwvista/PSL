//
//  Maze.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/02.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

class Maze: NSObject {
    var size = Position(8, 8)
    var height: Int {
        get {return size.row}
        set {size = Position(newValue, size.col)}
    }
    var width: Int {
        get {return size.col}
        set {size = Position(size.row, newValue)}
    }
    var currPos = Position()
    func setCurrPos(p: Position) {
        let nArea = height * width
        let n = (p.row * height + p.col + nArea) % nArea
        currPos = Position(n / height, n % height)
    }
    var pos2obj = [Position: Character]()
    
}

let maze = Maze()
