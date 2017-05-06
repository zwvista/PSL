//
//  MazeViewController.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/01.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

class MazeViewController: NSViewController, MazeDelegate {
    
    let sizes = [Int](1...20)
    
    @IBOutlet weak var heightPopup: NSPopUpButton!
    @IBOutlet weak var widthPopup: NSPopUpButton!
    @IBOutlet weak var mazeView: MazeView!
    @IBOutlet weak var positionTextField: NSTextField!
    @IBOutlet weak var mouseTextField: NSTextField!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        maze.delegate = self
        mazeView.mazeVC = self
        maze.updateMaze()
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    @IBAction func heightPopupChanged(_ sender: NSPopUpButton) {
        maze.height = sender.selectedItem!.representedObject as! Int
    }
    
    @IBAction func widthPopupChanged(_ sender: NSPopUpButton) {
        maze.width = sender.selectedItem!.representedObject as! Int
    }

    @IBAction func hasWallChanged(_ sender: NSButton) {
        maze.hasWall = sender.state == NSOnState
    }
    
    @IBAction func fillBorderLines(_ sender: NSButton) {
        maze.fillBorderLines()
    }
    
    func desc(p: Position) -> String {
        return "\(p.row),\(p.col)"
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
        maze.data = NSPasteboard.general().string(forType: NSPasteboardTypeString)!
    }
    
    @IBAction func clearAll(_ sender: NSButton) {
        maze.clearAll()
    }
    
    @IBAction func clearWalls(_ sender: NSButton) {
        maze.clearWalls()
    }
    
    @IBAction func clearChars(_ sender: NSButton) {
        maze.clearChars()
    }
    
    func updateMazeSize() {
        heightPopup.selectItem(withTitle: String(maze.height))
        widthPopup.selectItem(withTitle: String(maze.width))
    }
    
    func updateMazeView() {
        mazeView.needsDisplay = true
    }
    
    func updateCurPosition() {
        positionTextField.stringValue = desc(p: maze.curPos)
    }
}

