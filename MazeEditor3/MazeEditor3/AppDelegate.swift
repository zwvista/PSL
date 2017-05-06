//
//  AppDelegate.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2017/05/01.
//  Copyright © 2017年 趙偉. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {



    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Insert code here to initialize your application
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }

    // http://stackoverflow.com/questions/5268757/how-to-quit-cocoa-app-when-windows-close
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
}

