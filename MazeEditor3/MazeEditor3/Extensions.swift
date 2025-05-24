//
//  Extensions.swift
//  MazeEditor3
//
//  Created by 趙偉 on 2025/05/25.
//  Copyright © 2025 趙偉. All rights reserved.
//

extension String {
    public subscript(integerIndex: Int) -> Character {
        let index = self.index(startIndex, offsetBy: integerIndex)
        return self[index]
    }
}
