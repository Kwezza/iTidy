# iTidy Bug List

## Active Bugs

### Bug #1: DefIcons Randomly Returns 'project' Type Instead of 'ascii' for .txt Files

**Status:** FIXED - Extension cache was amplifying DefIcons bugs  
**Date Reported:** 2026-02-11  
**Date Fixed:** 2026-02-11  
**Severity:** Medium - Results in incorrect icon types being created

**Description:**
When running iTidy on a folder containing .txt files (after deleting all existing .info files with `delete #?.info`), DefIcons occasionally returns 'project' type instead of the expected 'ascii' type for .txt files.

**Evidence from `icons_2026-02-11_10-17-19.log`:**

**First run (10:17:26):**
- `sdw-life.txt` → DefIcons correctly returned `'ascii'` ✅
- File received hand-drawn text preview icon as expected

**Second run (10:19:32) - Same session:**
- `xz-aisli.txt` → DefIcons returned `'project'` ❌ (should be 'ascii')
- Multiple other .txt files received default project icons WITHOUT DefIcons being queried:
  - `wpx-rntn.txt`, `wpx-----.txt`, `tls-else.txt`, `tlo3o3o3.txt`
  - `skyline.txt`, `shd-back.txt`, `sdw-life.txt`, `shd-2oo3.txt`, `pa-jsa.txt`

**Log Evidence:**
```
Line 23: [10:17:26] DefIcons query: 'sdw-life.txt' → type 'ascii' ✅
Line 5142: [10:19:32] DefIcons query: 'xz-aisli.txt' → type 'project' ❌
Lines 5146-5153: Multiple .txt files got project icons without DefIcons queries
```

**Reproduction Context:**
- User deletes all .info files with shell command: `delete #?.info`
- Runs iTidy to regenerate icons
- Issue occurs randomly - not reproducible consistently

**Root Cause:**
1. **DefIcons bug**: DefIcons occasionally returns 'project' for .txt files with ASCII art content
2. **Extension cache amplification**: iTidy's extension cache stores the first DefIcons response per extension (`.txt` → `'project'`)
3. **Cascading failure**: All subsequent .txt files hit the cache and get 'project' WITHOUT querying DefIcons
4. **Files without extensions work**: Files like `ReadMeSE`, `Instructions`, `Manual` bypass cache and query DefIcons directly

**Proof:**
- User test: Renamed `KB-idiot.txt` → `KB-idiot` (removed extension)
- Result: File immediately received correct 'ascii' type and text preview icon
- This proves the cache was the issue, not DefIcons queries themselves

**Impact:**
- .txt files that should get hand-drawn text preview icons get generic project icons instead
- Only affects first run after cache is empty - subsequent runs reuse bad cache
- Files without .txt extension always work correctly

**Fix Implemented:**
1. **Response validation**: Added `validate_deficons_response()` function that detects `.txt` + `'project'` pattern
2. **Type override**: When DefIcons returns 'project' for .txt files, iTidy overrides it to 'ascii'
3. **Cache bypass**: Invalid responses from DefIcons are not cached (prevents amplification)
4. **Narrowly scoped**: Only corrects known bug case (.txt files), doesn't interfere with other file types
5. **Enhanced logging**: 
   - Warns: "Suspicious DefIcons response: .txt → 'project' (expected 'ascii')"
   - Confirms: "Corrected type to 'ascii' for: [filepath]"
   
**Code Location:** `src/deficons_identify.c` - `deficons_identify_file()` function

**Testing Results:**
- Multiple test runs confirm the fix works consistently
- When DefIcons returns 'project' for .txt files, iTidy detects and corrects it
- All .txt files now receive proper cyan text preview icons regardless of DefIcons response
- Other file types (.lha, .mod, etc.) unaffected - only .txt override implemented
- No false positives observed

---

## Resolved Bugs

*(No resolved bugs yet)*
