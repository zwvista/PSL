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
    
    override func viewDidLoad() {
        super.viewDidLoad()
        heightPopup.selectItem(withTitle: String(maze.height))
        widthPopup.selectItem(withTitle: String(maze.width))
        mazeView.mazeVC = self
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
    
    func updatePosition(p: Position) {
        positionTextField.stringValue = "\(p.row),\(p.col)"
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

