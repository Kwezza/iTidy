# What's New in iTidy v2.0

iTidy v2.0 is a major update with a refreshed look and a number of useful new features. Here's a quick overview of what's new.

---

## Fresh New Look

The entire interface has been redesigned using the native Workbench 3.2 style, so iTidy fits right in with your desktop. This version requires **Workbench 3.2 or later**.

---

## Automatic Icon Creation

iTidy v2.0 can create icons automatically for files and folders that don't already have them. To do this it uses **DefIcons**, the file type detection system built into Workbench 3.2. DefIcons examines each file, identifies its type using its built-in file type database, and supplies the appropriate default icon. iTidy does not maintain its own type database or icon set -- it relies entirely on what DefIcons already knows about.

On top of the standard DefIcons icons, iTidy can optionally generate a **thumbnail preview** directly into the icon image for pictures, or a miniature rendered preview of the contents for text files. These are not separate files -- the preview is embedded in the .info icon itself. Whether to do this is configurable per file type.

For image files, ILBM/IFF is handled natively without needing a datatype. All other formats go through the Amiga datatype system -- if DefIcons identifies the file as an image type and a matching datatype is installed, iTidy can decode it and produce a thumbnail. A standard Workbench 3.2.3 installation includes datatypes for JPEG, BMP, GIF, and PNG, so those formats work out of the box. Any additional third-party datatypes you install are picked up automatically -- no changes to iTidy are needed.

**Note:** If DefIcons does not recognise a file type, iTidy will not create an icon for it. What gets recognised depends on your Workbench 3.2 installation and any additional DefIcons type entries you have added.

---

## Choose Which File Types Get Icons

A new settings window lists all the file types that DefIcons reports on your system. You can enable or disable icon creation for each type individually, and view or change the default tool assigned to open files of that type. Only image types that DefIcons recognises will appear here.

---

## Exclude Folders From Icon Creation

You can now tell iTidy to skip specific folders when creating icons. System directories like C:, Libs:, and Devs: are already excluded by default, and you can add your own.


