//
//  MazeViewController.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/01.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

class MazeViewController: NSViewController {
    
    let sizes = [Int](1...20)
    
    @IBOutlet weak var heightPopup: NSPopUpButton!
    @IBOutlet weak var widthPopup: NSPopUpButton!
    @IBOutlet weak var mazeView: MazeView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        NSEvent.addLocalMonitorForEvents(matching: .keyDown) {
            self.keyDown(with: $0)
            return $0
        }
        heightPopup.selectItem(withTitle: String(maze.height))
        widthPopup.selectItem(withTitle: String(maze.width))
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    override var acceptsFirstResponder: Bool {return true}
    
    @IBAction func heightPopupChanged(_ sender: NSPopUpButton) {
        maze.height = sender.selectedItem!.representedObject as! Int
        mazeView.needsDisplay = true
    }
    
    @IBAction func widthPopupChanged(_ sender: NSPopUpButton) {
        maze.width = sender.selectedItem!.representedObject as! Int
        mazeView.needsDisplay = true
    }
    
    func moveLeft() {
        maze.setCurrPos(p: Position(maze.currPos.row, maze.currPos.col - 1))
        mazeView.needsDisplay = true
    }
    
    func moveRight() {
        maze.setCurrPos(p: Position(maze.currPos.row, maze.currPos.col + 1))
        mazeView.needsDisplay = true
    }
    
    func moveUp() {
        maze.setCurrPos(p: Position(maze.currPos.row - 1, maze.currPos.col))
        mazeView.needsDisplay = true
    }
    
    func moveDown() {
        maze.setCurrPos(p: Position(maze.currPos.row + 1, maze.currPos.col))
        mazeView.needsDisplay = true
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
                maze.pos2obj[maze.currPos] = Character(UnicodeScalar(char)!)
                moveRight()
            }
            super.keyDown(with: event)
        }
    }

}

