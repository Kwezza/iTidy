# Default Tool Standardization Feature - Design Discussion

## Current State Problem
From the cache, I can see multiple icons all pointing to the same tool (MultiView with 45 hits), but each icon likely has its **own hardcoded path** like:
- Some: `Workbench:Utilities/MultiView`
- Others: `SYS:Utilities/MultiView`
- Others: `Work:Tools/MultiView`
- Maybe even: `RAM:MultiView` (temporary copy)

All pointing to what's essentially the **same executable** (same version).

## Standardization Benefits

**1. System Independence:**
- Setting to just `"MultiView"` lets AmigaDOS PATH resolution find it
- Icons become portable - work on any system regardless of volume names
- No broken paths when users have different partition layouts

**2. Update Resilience:**
- If user upgrades MultiView to newer version in *different location*, icons still work
- Current hardcoded paths break if tool moves

**3. Consistency:**
- All icons use same default tool string = easier bulk operations
- Sorting/grouping by default tool becomes more meaningful

## Validation Strategy

**Version Checking Logic:**
1. Group all icons by **simple tool name** (case-insensitive)
2. For each group, check if **all versions match**
3. If versions match → **safe to standardize** to simple name
4. If versions differ → **flag for review** (user might have MultiView 47.17 *and* 48.0 intentionally)

## Edge Cases to Consider

**When NOT to standardize:**
- **Different versions found** - User might need specific version for compatibility
- **Custom tool locations** - User might have modified version in Work:MyTools/
- **Tool not in PATH** - If Lock() only found it via hardcoded path, removing path breaks it
- **Special cases** - Some tools might legitimately need full paths (network volumes, etc.)

## Proposed Workflow

```
For each tool in cache with hitCount > 1:
  ├─ Collect all icons using this tool
  ├─ Check versions of all references
  ├─ If ALL versions identical:
  │   ├─ Verify tool exists in PATH (not just hardcoded location)
  │   └─ Offer to standardize to simple "ToolName"
  └─ If versions differ:
      └─ Show user the different versions and let them decide
```

## UI Presentation Idea

**"Default Tool Standardization" Window:**
```
Tool: MultiView
Current usage: 45 icons
Versions found: 1 (all using 47.17)
Path variants: 3 different paths

☑ Standardize to "MultiView" (recommended)
  - Icons will use PATH resolution
  - More portable across systems
  
Current paths being used:
  [23] Workbench:Utilities/MultiView
  [18] SYS:Utilities/MultiView
  [ 4] Work:Apps/MultiView

Preview: Tool resolves to: Workbench:Utilities/MultiView ✓
```

## Cache Data We Already Have

Perfect timing - the cache system we just built gives us:
- ✅ Tool names grouped
- ✅ Hit counts (how many icons use it)
- ✅ Version strings
- ✅ Full paths where found

**Missing piece:** We'd need to track **all unique paths** each tool is referenced from in icons (not just where we found it in PATH). Right now cache shows where *we* found it, not where *icons* point to it.

## Implementation Considerations

### Should we:
1. Extend cache to track **path variants per tool** (how many icons use `SYS:` vs `Workbench:` etc.)?
2. Add version comparison logic to identify "safe to standardize" candidates?
3. Build this as a separate analysis phase after processing (using persistent cache)?

### Priority Questions:
- Automatic standardization with safety checks?
- Interactive review window?
- Batch mode with report generation?

## Next Steps

To implement this feature, we would need to:

1. **Enhance ToolCacheEntry structure:**
   - Add array of path variants encountered
   - Track count per variant
   - Store icon references for each variant

2. **Modify ValidateDefaultTool():**
   - Record the original path from icon (before validation)
   - Pass both original path and resolved path to cache

3. **Build Analysis Phase:**
   - After processing, analyze cache for standardization candidates
   - Group by tool name and check version consistency
   - Generate recommendations

4. **Create UI Window:**
   - Display tools eligible for standardization
   - Show path variants and counts
   - Allow user selection/confirmation
   - Preview impact before applying

5. **Apply Changes:**
   - Batch update selected icons
   - Write standardized default tool names
   - Generate backup/report of changes
