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
    @IBOutlet weak var positionTextField: NSTextField!
    @IBOutlet weak var mouseTextField: NSTextField!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        heightPopup.selectItem(withTitle: String(maze.height))
        widthPopup.selectItem(withTitle: String(maze.width))
        mazeView.mazeVC = self
        updateCurPosition()
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    @IBAction func heightPopupChanged(_ sender: NSPopUpButton) {
        maze.height = sender.selectedItem!.representedObject as! Int
        mazeView.needsDisplay = true
    }
    
    @IBAction func widthPopupChanged(_ sender: NSPopUpButton) {
        maze.width = sender.selectedItem!.representedObject as! Int
        mazeView.needsDisplay = true
    }

    @IBAction func hasWallChanged(_ sender: NSButton) {
        maze.hasWall = sender.state == NSOnState
        mazeView.needsDisplay = true
    }
    
    @IBAction func fillBorderLines(_ sender: NSButton) {
        maze.fillBorderLines()
        mazeView.needsDisplay = true
    }
    
    func desc(p: Position) -> String {
        return "\(p.row),\(p.col)"
    }
    
    func updateCurPosition() {
        positionTextField.stringValue = desc(p: maze.curPos)
    }
    
    func updateMousePosition(p: Position) {
        mouseTextField.stringValue = desc(p: p)
    }
    
    @IBAction func copy(_ sender: NSButton) {
        let pb = NSPasteboard.general()
        pb.clearContents()
        pb.setString(maze.data, forType: NSPasteboardTypeString)
    }
    
    @IBAction func paste(_ sender: NSButton) {
    }
    
    @IBAction func clear(_ sender: NSButton) {
    }
}

