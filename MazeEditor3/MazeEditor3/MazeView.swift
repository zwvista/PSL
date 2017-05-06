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
    override var acceptsFirstResponder: Bool {return true}

    var spacing:CGFloat = 0
    weak var mazeVC: MazeViewController?
    
    override func awakeFromNib() {
        super.awakeFromNib()
        let options:NSTrackingAreaOptions = [
            .mouseEnteredAndExited,
            .mouseMoved,
            .cursorUpdate,
            .activeAlways
        ]
        let trackingArea = NSTrackingArea(rect: bounds, options: options, owner: self, userInfo: nil)
        addTrackingArea(trackingArea)

    }
    
    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        NSColor.white.setFill()
        NSRectFill(dirtyRect)

        NSColor.black.set()
        let rows = maze.height;
        let cols = maze.width;
        let vSpacing = dirtyRect.size.height / CGFloat(rows)
        let hSpacing = dirtyRect.size.width / CGFloat(cols)
        spacing = min(vSpacing, hSpacing)
        let bPath:NSBezierPath = NSBezierPath()
        bPath.lineWidth = 2
        for i in 0...rows {
            bPath.move(to: NSMakePoint(0, CGFloat(i) * spacing))
            bPath.line(to: NSMakePoint(CGFloat(cols) * spacing , CGFloat(i) * spacing))
        }
        bPath.stroke()
        for i in 0...cols {
            bPath.move(to: NSMakePoint(CGFloat(i) * spacing, 0))
            bPath.line(to: NSMakePoint(CGFloat(i) * spacing, CGFloat(rows) * spacing))
        }
        bPath.stroke()
        if maze.hasWall {
            NSColor.red.set()
            let bPath2:NSBezierPath = NSBezierPath()
            bPath2.lineWidth = 5
            for p in maze.horzWall {
                bPath2.move(to: NSMakePoint(CGFloat(p.col) * spacing, CGFloat(p.row) * spacing))
                bPath2.line(to: NSMakePoint(CGFloat(p.col + 1) * spacing, CGFloat(p.row) * spacing))
            }
            for p in maze.vertWall {
                bPath2.move(to: NSMakePoint(CGFloat(p.col) * spacing, CGFloat(p.row) * spacing))
                bPath2.line(to: NSMakePoint(CGFloat(p.col) * spacing, CGFloat(p.row + 1) * spacing))
            }
            bPath2.stroke()
        }
        
        NSColor.green.setFill()
        let margin:CGFloat = 2
        NSRectFill(NSRect(x: CGFloat(maze.curPos.col) * spacing + margin,
                          y: CGFloat(maze.curPos.row) * spacing + margin,
                          width: spacing - margin * 2, height: spacing - margin * 2))
        
        let font = NSFont(name: "Helvetica Bold", size: 20.0)
        let textStyle = NSMutableParagraphStyle.default().mutableCopy() as! NSMutableParagraphStyle
        textStyle.alignment = NSTextAlignment.center
        let textColor = NSColor.brown
        let textFontAttributes = [
            NSFontAttributeName: font,
            NSForegroundColorAttributeName: textColor,
            NSParagraphStyleAttributeName: textStyle
        ]
        for r in 0..<rows {
            for c in 0..<cols {
                let p = Position(r, c)
                if let ch = maze.getObject(p: p) {
                    ("\(ch)" as NSString).draw(
                        in: NSRect(x: CGFloat(c) * spacing + margin,
                                   y: CGFloat(r) * spacing + margin,
                                   width: spacing - margin * 2, height: spacing - margin * 2),
                        withAttributes: textFontAttributes)
                }
            }
        }

    }
    
    func moveLeft() {
        maze.setCurPos(p: Position(maze.curPos.row, maze.curPos.col - 1))
        needsDisplay = true
    }
    
    func moveRight() {
        maze.setCurPos(p: Position(maze.curPos.row, maze.curPos.col + 1))
        needsDisplay = true
    }
    
    func moveUp() {
        maze.setCurPos(p: Position(maze.curPos.row - 1, maze.curPos.col))
        needsDisplay = true
    }
    
    func moveDown() {
        maze.setCurPos(p: Position(maze.curPos.row + 1, maze.curPos.col))
        needsDisplay = true
    }
    
    override func keyDown(with event: NSEvent) {
        // http://stackoverflow.com/questions/9268045/how-can-i-detect-that-the-shift-key-has-been-pressed
        let char = Int(event.charactersIgnoringModifiers!.utf16[String.UTF16View.Index(0)])
        let hasCommand = event.modifierFlags.contains(.command)
        switch char {
        case NSLeftArrowFunctionKey:
            moveLeft()
        case NSRightArrowFunctionKey:
            moveRight()
        case NSUpArrowFunctionKey:
            moveUp()
        case NSDownArrowFunctionKey:
            moveDown()
        default:
            if isprint(Int32(char)) != 0 {
                maze.setObject(p: maze.curPos, char: Character(UnicodeScalar(char)!))
                moveRight()
            }
            super.keyDown(with: event)
        }
    }
    
    override func mouseDown(with event: NSEvent) {
        let offset:CGFloat = 10
        let pt = event.locationInWindow
        let (x, y) = (pt.x, frame.size.height - pt.y)
        let p = Position(min(maze.height - 1, Int(y / spacing)), min(maze.width - 1, Int(x / spacing)))
        let p2 = Position(min(maze.height - 1, Int((y + offset) / spacing)), min(maze.width - 1, Int((x + offset) / spacing)))
        if maze.hasWall && abs(x - CGFloat(p2.col) * spacing) < offset {
            maze.vertWall.insert(p2)
        } else if maze.hasWall && abs(y - CGFloat(p2.row) * spacing) < offset {
            maze.horzWall.insert(p2)
        } else {
            maze.setCurPos(p: p)
        }
        needsDisplay = true
    }
    
    override func mouseMoved(with event: NSEvent) {
        let pt = event.locationInWindow
        let (x, y) = (pt.x, frame.size.height - pt.y)
        let p = Position(min(maze.height - 1, Int(y / spacing)), min(maze.width - 1, Int(x / spacing)))
        mazeVC?.updatePosition(p: p)
    }
    
}
