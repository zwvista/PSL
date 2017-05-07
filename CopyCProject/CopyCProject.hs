module Main where

import System.IO
import Data.String.Utils
import Control.Monad

main = do
    let prj9 = "Puzzles9"
        cprojectPath prj = "../" ++ prj ++ "/.cproject"
    inputHandle <- openFile (cprojectPath prj9) ReadMode 
    hSetEncoding inputHandle utf8
    theInput <- hGetContents inputHandle
    forM_ ["A", "B", "C", "D", "E", "F", "G", "H", "I", "K", "L", "M", "N", "O", "P", "R", "S", "T", "W", "Y", "Z"] $ \a -> do
        let prj = "Puzzles" ++ a 
        outputHandle <- openFile (cprojectPath prj) WriteMode
        hSetEncoding outputHandle utf8
        hPutStr outputHandle $ replace prj9 prj theInput
        hClose outputHandle
