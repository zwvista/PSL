//
//  MazeView.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/02.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

class MazeView: NSView {

    override var isFlipped: Bool { return true }
    
    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        NSColor.white.setFill()
        NSRectFill(dirtyRect)

        let blackColor = NSColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
        blackColor.set()
        let noHLines = maze.size.row;
        let noVLines = maze.size.col;
        let vSpacing = dirtyRect.size.height / CGFloat(noHLines)
        let hSpacing = dirtyRect.size.width / CGFloat(noVLines)
        let spacing = min(vSpacing, hSpacing)
        let bPath:NSBezierPath = NSBezierPath()
        bPath.lineWidth = 1.0
        for i in 0...noHLines {
            let yVal = CGFloat(i) * spacing
            bPath.move(to: NSMakePoint(0, yVal))
            bPath.line(to: NSMakePoint(CGFloat(noVLines) * spacing , yVal))
        }
        bPath.stroke()
        for i in 0...noVLines {
            let xVal = CGFloat(i) * spacing
            bPath.move(to: NSMakePoint(xVal, 0))
            bPath.line(to: NSMakePoint(xVal, CGFloat(noHLines) * spacing))
        }
        bPath.stroke()
        
        NSColor.green.setFill()
        let margin:CGFloat = 2
        NSRectFill(NSRect(x: CGFloat(maze.currPos.col) * spacing + margin,
                          y: CGFloat(maze.currPos.row) * spacing + margin,
                          width: spacing - margin * 2, height: spacing - margin * 2))

    }
    
}
