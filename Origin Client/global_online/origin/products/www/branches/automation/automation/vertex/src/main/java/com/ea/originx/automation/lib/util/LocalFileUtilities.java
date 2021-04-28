package com.ea.originx.automation.lib.util;

import java.io.File;
import java.util.Arrays;

/**
 * Utilities for editing files on the local machine.
 * 
 * @author glivingstone
 */
public class LocalFileUtilities {
    /**
     * Deletes a folder and all subfolders from the hard drive. If a file is
     * passed in (instead of a folder), we try to delete it anyway.
     *
     * @param folderLoc - The location of the folder we wish to delete
     * @return true if the folder is successfully deleted or does not exist.
     * true if it is a file that is successfully deleted false otherwise
     */
    public static boolean deleteFolder(File folderLoc) {
        if (folderLoc.isDirectory()) {
            // If the directory is not empty, delete all subfiles and folders first.
            File[] fileList = folderLoc.listFiles();
            Arrays.stream(fileList).forEach(f -> deleteFolder(f));
        }
        return deleteFile(folderLoc);
    }
    
    /**
     * Deletes a file from the hard drive if it exists.
     *
     * @param fileLoc The location of the given file on the hard drive.
     * @return true if the file does not exist, or is deleted successfully.
     */
    public static boolean deleteFile(File fileLoc) {
        if (!fileLoc.exists()) {
            // delete() returns false if the file doesn't exist, but not
            // existing is fine, so wrap it to return true
            return true;
        }
        return fileLoc.delete();
    }
}
