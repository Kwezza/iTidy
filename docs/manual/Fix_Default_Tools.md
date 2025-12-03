# iTidy Default Tool Analysis Window

This document explains the Default Tool Analysis window, accessible by clicking "Fix Default Tools..." in the main iTidy window.

---

## Purpose

This feature scans icons in the selected folder to find and optionally repair missing or invalid default tools.

---

## Window Layout

### Folder Selection

At the top of the window, you can see and change the folder being scanned:
- **Folder:** Shows the current folder path
- **Browse...** Opens a file requester to select a different folder

### Filter Cycle Gadget

A cycle gadget allows you to filter the tool list:
- **Show All** - Display all tools found
- **Show Existing** - Only show tools that exist on your system
- **Show Missing** - Only show tools that are missing

### Tool List

The main list displays all default tools found in the scanned icons:

| Column | Description |
|--------|-------------|
| Tool | The default tool path (e.g., `SYS:Tools/TextEdit`) |
| Files | Number of icons using this tool |
| Status | MISSING or EXISTS |

### Details Panel

Below the tool list, a details panel shows information about the selected tool:
- **Tool:** Full path to the default tool
- **Status:** EXISTS or MISSING, plus version information if available
- **File list:** Shows which icon files use this tool

---

## What are Default Tools?

Every Amiga icon can specify a "Default Tool" - the program that opens when you double-click the icon. For example, a `.txt` file icon might have `C:Ed` or `SYS:Utilities/MultiView` as its default tool.

---

## The Problem

When copying icons between systems or extracting from archives, the default tool paths may point to programs that don't exist on your system. Double-clicking these icons produces an error like "Unable to open your tool".

---

## How Fix Default Tools Works

### 1. Scan Phase

- Examines all icons in the target folder and all subfolders (always recursive)
- Checks if each icon's default tool exists on your system
- Uses the Amiga PATH to search for tools
- Builds a cache of found/missing tools

### 2. Review Phase

- Displays a list of all default tools with their status
- Shows status: EXISTS or MISSING
- Filter by status to see only problems
- Export reports to formatted text files

### 3. Repair Phase (optional)

- Select icons with missing tools
- Choose a replacement tool from suggestions
- Apply fixes to update the icons
- Creates backup before modifying (if enabled)

---

## Menu Bar

### Project Menu

| Menu Item | Shortcut | Description |
|-----------|----------|-------------|
| New | Amiga+N | Clear the current scan and start fresh |
| Open... | Amiga+O | Load a previously saved scan from file |
| Save | Amiga+S | Save the current scan to the last used file |
| Save as... | Amiga+A | Save the current scan to a new file |
| Close | Amiga+C | Close the Default Tool Analysis window |

**Saving and Loading Scans:**
Scanning complete drives (e.g., `SYS:` or `Work:`) can take considerable time. The Project menu allows you to save scan results to a file and reload them later, avoiding the need to rescan.

### File Menu

| Menu Item | Shortcut | Description |
|-----------|----------|-------------|
| Export list of tools | Amiga+T | Export the tool list to a formatted text file |
| Export list of files and tools | Amiga+F | Export a detailed list of all files with their default tools |

**Export Features:**
These options allow you to save reports to a location of your choosing. Useful for documentation, analysis, or sharing with others.

---

## Buttons

| Button | Description |
|--------|-------------|
| **Replace Tool (Batch)** | Replace the selected tool across all icons that use it |
| **Replace Tool (Single)** | Replace the tool for a single selected icon file (enabled when a file is selected in the details panel) |
| **Restore Default Tools Backups...** | Opens window to restore previously backed-up default tool settings |
| **Scan** | Rescan the folder for default tools and rebuild the cache |
| **Close** | Return to main window |

---

## Using the Tool Scanner

### Step 1: Select a Folder

Use the folder path display at the top of the window, or click **Browse...** to select a different folder to scan.

### Step 2: Scan

Click **Scan** to scan all icons in the selected folder and all subfolders. The scan is always recursive to ensure all icons are checked.

### Step 3: Review Results

The tool list shows all default tools found:
- **EXISTS** indicates the tool was found on your system
- **MISSING** indicates the tool cannot be found

Use the **Filter** cycle gadget to show:
- Show All - all tools
- Show Existing - only tools that exist
- Show Missing - only missing tools

Click on a tool to see details in the panel below, including which icon files use that tool.

### Step 4: Fix Missing Tools (Optional)

1. Select a tool with MISSING status from the list
2. Click **Replace Tool (Batch)** to replace it across all icons that use it
3. Or select a specific file in the details panel and click **Replace Tool (Single)**
4. Choose a replacement tool from the suggestions or browse for one
5. Confirm the change

---

## Restore Default Tools Backups

The **Restore Default Tools Backups...** button opens a window to restore previously backed-up default tool settings. Before making changes, iTidy will automatically create a backup of the original default tools, allowing you to revert if needed.  This restore window also allows for deletion of previous backups.

---

## Technical Notes

### PATH Searching

The scanner searches for tools using the Amiga PATH environment variable. This includes:
- `C:` (standard commands directory)
- Any directories added to your PATH in `S:Startup-Sequence` or `S:User-Startup`

### Tool Cache

The scanner builds a cache of tool locations to speed up repeated scans. The cache is:
- Built fresh when you click "Rebuild Cache"
- Retained during the session for quick lookups
- Cleared when you close iTidy

### Backup Integration

When you modify a default tool, iTidy automatically creates a backup of the original setting. These backups are small text files that record the original tool path, so they are always created (this cannot be disabled). This allows you to restore original default tools at any time using the **Restore Default Tools Backups...** button.
