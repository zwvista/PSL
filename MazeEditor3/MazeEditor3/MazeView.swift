//
//  MazeView.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/02.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

class MazeView: NSView {

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        let blackColor = NSColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
        blackColor.set()
        var width = dirtyRect.size.width
        var height = dirtyRect.size.height
        let noHLines = 5;
        let noVLines = 10;
        let vSpacing = dirtyRect.size.height / CGFloat(noHLines)
        let hSpacing = dirtyRect.size.width / CGFloat(noVLines)
        var i:Int = 0;
        var bPath:NSBezierPath = NSBezierPath()
        bPath.lineWidth = 1.0
        for i in 0...noHLines {
            var yVal = CGFloat(i) * vSpacing
            bPath.move(to: NSMakePoint(0, yVal))
            bPath.line(to: NSMakePoint(dirtyRect.size.width , yVal))
        }
        bPath.stroke()
        for i in 0...noVLines {
            var xVal = CGFloat(i) * hSpacing
            bPath.move(to: NSMakePoint(xVal, 0))
            bPath.line(to: NSMakePoint(xVal, dirtyRect.size.height))
        }
        bPath.stroke()

    }
    
}
