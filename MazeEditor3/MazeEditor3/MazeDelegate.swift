//
//  MazeDelegate.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/06.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Foundation

enum MazeMovement: Int {
    case none, moveUp, moveDown, moveLeft, moveRight
}

protocol MazeDelegate: class {
    func updateMazeSize()
    func updateMazeView()
    func updateCurPosition()
    func updateMousePosition(p: Position)
    func updateIsSquare()
    func updateCurObject()
    func updateHasWall()
    var curMovement: MazeMovement {get}
}
