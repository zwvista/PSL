//
//  MazeViewController.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/01.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

class MazeViewController: NSViewController, MazeDelegate {
    
    var sizes = [Int](1...20)
    let movements = ["None", "Move Up", "Move Down", "Move Left", "Move Right"]
    var curMovement: MazeMovement {
        return MazeMovement(rawValue: movementPopup.indexOfSelectedItem)!
    }
    
    @IBOutlet var arrayController: NSArrayController!
    @IBOutlet weak var heightPopup: NSPopUpButton!
    @IBOutlet weak var widthPopup: NSPopUpButton!
    @IBOutlet weak var mazeView: MazeView!
    @IBOutlet weak var positionTextField: NSTextField!
    @IBOutlet weak var mouseTextField: NSTextField!
    @IBOutlet weak var isSquareCheckbox: NSButton!
    @IBOutlet weak var movementPopup: NSPopUpButton!
    @IBOutlet weak var charTextField: NSTextField!
    @IBOutlet weak var hasWallCheckbox: NSButton!
    @IBOutlet weak var fillBorderLinesButton: NSButton!
    
    override func viewDidLoad() {
        arrayController.content = sizes
        super.viewDidLoad()
        maze.delegate = self
        mazeView.delegate = self
        maze.updateMaze()
       
        movementPopup.removeAllItems()
        movementPopup.addItems(withTitles: movements)
        movementPopup.selectItem(at: MazeMovement.moveRight.rawValue)
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

    @IBAction func isSquareChanged(_ sender: NSButton) {
        maze.isSquare = sender.state == .on
    }
    
    @IBAction func hasWallChanged(_ sender: NSButton) {
        maze.hasWall = sender.state == .on
    }
    
    @IBAction func fillBorderLines(_ sender: NSButton) {
        maze.fillBorderLines()
    }
    
    @IBAction func copy(_ sender: NSButton) {
        let pb = NSPasteboard.general
        pb.clearContents()
        pb.setString(maze.data, forType: NSPasteboard.PasteboardType.string)
    }
    
    @IBAction func paste(_ sender: NSButton) {
        maze.data = NSPasteboard.general.string(forType: NSPasteboard.PasteboardType.string)!
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
    
    private func desc(p: Position) -> String {
        return "\(p.row),\(p.col)"
    }
    
    func updateSelectedPosition() {
        positionTextField.stringValue = desc(p: maze.selectedPosition)
    }
    
    func updateMousePosition(p: Position) {
        mouseTextField.stringValue = desc(p: p)
    }
    
    func updateIsSquare() {
        isSquareCheckbox.state = maze.isSquare ? .on : .off
        widthPopup.isEnabled = !maze.isSquare
    }
    
    func updateCurObject() {
        charTextField.stringValue = String(maze.curObj)
    }
    
    func updateHasWall() {
        hasWallCheckbox.state = maze.hasWall ? .on : .off
        fillBorderLinesButton.isEnabled = maze.hasWall
    }
    
    deinit {
        Swift.print("deinit called: \(NSStringFromClass(type(of: self)))")
    }
}

