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
    weak var delegate: MazeDelegate?
    
    override func awakeFromNib() {
        super.awakeFromNib()
        let options:NSTrackingArea.Options = [
            NSTrackingArea.Options.mouseEnteredAndExited,
            NSTrackingArea.Options.mouseMoved,
            NSTrackingArea.Options.cursorUpdate,
            NSTrackingArea.Options.activeAlways
        ]
        let trackingArea = NSTrackingArea(rect: bounds, options: options, owner: self, userInfo: nil)
        addTrackingArea(trackingArea)

    }
    
    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        NSColor.white.setFill()
        dirtyRect.fill()

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
            let color2 = NSColor(calibratedRed: 128, green: 0, blue: 0, alpha: 1)
            color2.set()
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
        
        let color1 = NSColor(calibratedRed: 0, green: 200, blue: 0, alpha: 1)
        color1.setFill()
        let margin:CGFloat = 3
        for p in maze.selectedPositions {
            NSRect(x: CGFloat(p.col) * spacing + margin,
                              y: CGFloat(p.row) * spacing + margin,
                              width: spacing - margin * 2, height: spacing - margin * 2).fill()
        }
        
        let font = NSFont(name: "Helvetica Bold", size: 20.0)
        let textStyle = NSMutableParagraphStyle.default.mutableCopy() as! NSMutableParagraphStyle
        textStyle.alignment = NSTextAlignment.center
        let textColor = NSColor.brown
        let textFontAttributes: [NSAttributedStringKey: Any] = [
            NSAttributedStringKey.font: font as Any,
            NSAttributedStringKey.foregroundColor: textColor,
            NSAttributedStringKey.paragraphStyle: textStyle
        ]
        for r in 0..<rows {
            for c in 0..<cols {
                let p = Position(r, c)
                if let ch = maze.getObject(p: p) {
                    let str = "\(ch)" as NSString
                    str.draw(
                        in: NSRect(x: CGFloat(c) * spacing,
                                   y: CGFloat(r) * spacing,
                                   width: spacing, height: spacing),
                        withAttributes: textFontAttributes)
                }
            }
        }
    }
    
    override func keyDown(with event: NSEvent) {
        func moveLeft() {
            maze.setSelectedPosition(p: Position(maze.selectedPosition.row, maze.selectedPosition.col - 1))
        }
        func moveRight() {
            maze.setSelectedPosition(p: Position(maze.selectedPosition.row, maze.selectedPosition.col + 1))
        }
        func moveUp() {
            maze.setSelectedPosition(p: Position(maze.selectedPosition.row - 1, maze.selectedPosition.col))
        }
        func moveDown() {
            maze.setSelectedPosition(p: Position(maze.selectedPosition.row + 1, maze.selectedPosition.col))
        }
        func moveNext() {
            switch delegate!.curMovement {
            case .moveUp:
                moveUp()
            case .moveDown:
                moveDown()
            case .moveLeft:
                moveLeft()
            case .moveRight:
                moveRight()
            default:
                break
            }
        }
        // http://stackoverflow.com/questions/9268045/how-can-i-detect-that-the-shift-key-has-been-pressed
        var ch = Int(event.charactersIgnoringModifiers!.utf16[String.UTF16View.Index(encodedOffset: 0)])
        let hasCommand = event.modifierFlags.contains(.command)
        switch ch {
        case NSLeftArrowFunctionKey:
            if !hasCommand {
                moveLeft()
            } else {
                for p in maze.selectedPositions {
                    maze.toggleVertWall(p: p)
                }
            }
        case NSRightArrowFunctionKey:
            if !hasCommand {
                moveRight()
            } else {
                for p in maze.selectedPositions {
                    maze.toggleVertWall(p: Position(p.row, p.col + 1))
                }
            }
        case NSUpArrowFunctionKey:
            if !hasCommand {
                moveUp()
            } else {
                for p in maze.selectedPositions {
                    maze.toggleHorzWall(p: p)
                }
            }
        case NSDownArrowFunctionKey:
            if !hasCommand {
                moveDown()
            } else {
                for p in maze.selectedPositions {
                    maze.toggleHorzWall(p: Position(p.row + 1, p.col))
                }
            }
        case NSDeleteCharacter:
            maze.setObject(p: maze.selectedPosition, ch: Character(UnicodeScalar(" ")!))
            moveLeft()
        case NSCarriageReturnCharacter:
            maze.setObject(p: maze.selectedPosition, ch: maze.curObj)
            moveNext()
        default:
//            if isprint(Int32(ch)) != 0 {
                ch = Int(event.characters!.utf16[String.UTF16View.Index(encodedOffset: 0)])
                for p in maze.selectedPositions {
                    maze.setObject(p: p, ch: Character(UnicodeScalar(ch)!))
                }
                moveNext()
//            }
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
            maze.toggleVertWall(p: p2)
        } else if maze.hasWall && abs(y - CGFloat(p2.row) * spacing) < offset {
            maze.toggleHorzWall(p: p2)
        } else if event.modifierFlags.contains(.command) {
            maze.toggleSelectedPosition(p: p)
        } else {
            maze.setSelectedPosition(p: p)
        }
    }
    
    override func mouseMoved(with event: NSEvent) {
        let pt = event.locationInWindow
        let (x, y) = (pt.x, frame.size.height - pt.y)
        let p = Position(min(maze.height - 1, Int(y / spacing)), min(maze.width - 1, Int(x / spacing)))
        delegate?.updateMousePosition(p: p)
    }
    
    deinit {
        Swift.print("deinit called: \(NSStringFromClass(type(of: self)))")
    }
}
