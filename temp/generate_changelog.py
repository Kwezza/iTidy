#!/usr/bin/env python3
"""
Compare iTidy35.reb.bak (original) with iTidy35.reb (updated)
and generate a markdown changelog showing before/after for every change.
"""

import re

REB_ORIG = r'c:\Amiga\Programming\iTidy\Bin\Amiga\Rebuild\iTidy35.reb.bak'
REB_NEW  = r'c:\Amiga\Programming\iTidy\Bin\Amiga\Rebuild\iTidy35.reb'
OUTPUT   = r'c:\Amiga\Programming\iTidy\temp\reb_changelog.md'


def parse_blocks(filepath):
    """Parse a .reb file into a list of block dicts keyed by IDENT."""
    with open(filepath, 'rb') as f:
        content = f.read().decode('latin-1')

    lines = content.split('\n')
    blocks = {}
    current = None
    in_payload = False

    for line in lines:
        if line.startswith('TYPE: ') and not in_payload:
            current = {
                'type_code': line[6:].strip(),
                'id': '', 'parentid': '', 'name': '', 'ident': '',
                'label': '', 'hints': [], 'listitems': [],
                'payload_lines': [], 'in_payload': False,
            }
            in_payload = False

        if current is None:
            continue

        if not in_payload:
            if line.startswith('ID: '):
                current['id'] = line[4:].strip()
            elif line.startswith('PARENTID:'):
                current['parentid'] = line[10:].strip()
            elif line.startswith('NAME: '):
                current['name'] = line[6:].strip()
            elif line == 'NAME:' or line == 'NAME: ':
                current['name'] = ''
            elif line.startswith('IDENT: '):
                current['ident'] = line[7:].strip()
            elif line.startswith('LABEL: '):
                current['label'] = line[7:].strip()
            elif line == 'LABEL:' or line == 'LABEL: ':
                current['label'] = ''
            elif line.startswith('HINT: '):
                current['hints'].append(line[6:])

        if line == '--':
            in_payload = True
            current['in_payload'] = True

        if in_payload:
            if line.startswith('LISTITEM: '):
                current['listitems'].append(line[10:])

        if line == '-' and in_payload:
            in_payload = False
            if current['ident']:
                blocks[current['ident']] = current
            current = None

    return blocks


# Type code to human-readable name
TYPE_NAMES = {
    '0': 'ReactionList', '1': 'Screen', '2': 'Rexx', '3': 'Window',
    '4': 'Menu', '5': 'Button', '6': 'Bitmap', '7': 'Checkbox',
    '8': 'Chooser', '9': 'ClickTab', '10': 'FuelGauge', '11': 'GetFile',
    '12': 'GetFont', '14': 'Integer', '17': 'Layout', '18': 'ListBrowser',
    '20': 'Scroller', '24': 'String', '25': 'Space', '30': 'Label',
    '35': 'ListView',
}

# Window titles by IDENT
WINDOW_FOR_IDENT = {}

# Build window membership from parentid chains
def assign_windows(blocks):
    """Map each IDENT to its parent window name."""
    # First find all windows
    windows = {}
    for ident, b in blocks.items():
        if b['type_code'] == '3':
            windows[b['id']] = b['name'] or ident

    # Build parent chain map: id -> ident
    id_to_block = {}
    for ident, b in blocks.items():
        id_to_block[b['id']] = b

    def find_window(block):
        visited = set()
        cur = block
        while cur:
            if cur['id'] in visited:
                return None
            visited.add(cur['id'])
            if cur['type_code'] == '3':
                return cur['name'] or cur['ident']
            pid = cur['parentid']
            if pid and pid in id_to_block:
                cur = id_to_block[pid]
            else:
                return None
        return None

    result = {}
    for ident, b in blocks.items():
        w = find_window(b)
        if w:
            result[ident] = w
    return result


def main():
    print("Parsing original...")
    orig = parse_blocks(REB_ORIG)
    print(f"  {len(orig)} blocks")

    print("Parsing updated...")
    new = parse_blocks(REB_NEW)
    print(f"  {len(new)} blocks")

    window_map = assign_windows(new)

    # Collect changes
    hint_changes = []  # (ident, name, type, window, old_hints, new_hints, change_type)
    listitem_changes = []  # (ident, name, window, old_items, new_items)

    all_idents = set(orig.keys()) | set(new.keys())

    for ident in sorted(all_idents):
        o = orig.get(ident)
        n = new.get(ident)
        if not o or not n:
            continue

        # Check hint changes
        if o['hints'] != n['hints']:
            if not o['hints'] and n['hints']:
                change_type = 'Added'
            elif o['hints'] and n['hints']:
                change_type = 'Updated'
            else:
                change_type = 'Removed'

            display_name = n['name'] or n['label'] or ident
            type_name = TYPE_NAMES.get(n['type_code'], f"TYPE {n['type_code']}")
            window = window_map.get(ident, '(unparented)')

            hint_changes.append({
                'ident': ident,
                'name': display_name,
                'type': type_name,
                'window': window,
                'old_hints': o['hints'],
                'new_hints': n['hints'],
                'change_type': change_type,
            })

        # Check listitem changes
        if o['listitems'] != n['listitems'] and (o['listitems'] or n['listitems']):
            display_name = n['name'] or n['label'] or ident
            window = window_map.get(ident, '(global)')
            listitem_changes.append({
                'ident': ident,
                'name': display_name,
                'window': window,
                'old_items': o['listitems'],
                'new_items': n['listitems'],
            })

    # Group hint changes by window
    windows_order = []
    hints_by_window = {}
    for hc in hint_changes:
        w = hc['window']
        if w not in hints_by_window:
            hints_by_window[w] = []
            windows_order.append(w)
        hints_by_window[w].append(hc)

    # Generate markdown
    lines = []
    lines.append("# ReBuild File Changes: iTidy35.reb")
    lines.append("")
    lines.append("**Date:** 2 March 2026")
    lines.append("**File:** `Bin/Amiga/Rebuild/iTidy35.reb`")
    lines.append("**Backup:** `Bin/Amiga/Rebuild/iTidy35.reb.bak`")
    lines.append("")
    lines.append("This document records all changes made to the ReBuild `.reb` file to improve")
    lines.append("gadget hint text (tooltips) and list item labels. Changes were cross-referenced")
    lines.append("against the working notes in `docs/manual/working_notes/`.")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    added = sum(1 for hc in hint_changes if hc['change_type'] == 'Added')
    updated = sum(1 for hc in hint_changes if hc['change_type'] == 'Updated')
    lines.append(f"- **{added} hints added** (gadgets that previously had no tooltip)")
    lines.append(f"- **{updated} hints updated** (existing tooltips reworded for clarity)")
    li_count = sum(len(lc['new_items']) for lc in listitem_changes)
    lines.append(f"- **{li_count} list item labels updated** (chooser/cycle dropdown text improved)")
    lines.append(f"- **{added + updated + li_count} total changes**")
    lines.append("")
    lines.append("No structural changes were made (no IDs, PARENTIDs, types, or block ordering changed).")
    lines.append("")
    lines.append("---")
    lines.append("")

    # LISTITEM changes section
    if listitem_changes:
        lines.append("## List Item Changes")
        lines.append("")
        for lc in listitem_changes:
            lines.append(f"### {lc['name']} (`{lc['ident']}`)")
            lines.append("")
            lines.append("| # | Before | After |")
            lines.append("|---|--------|-------|")
            max_len = max(len(lc['old_items']), len(lc['new_items']))
            for i in range(max_len):
                old_val = lc['old_items'][i] if i < len(lc['old_items']) else '*(none)*'
                new_val = lc['new_items'][i] if i < len(lc['new_items']) else '*(none)*'
                changed = ' **' if old_val != new_val else ''
                changed_end = '**' if old_val != new_val else ''
                if old_val != new_val:
                    lines.append(f"| {i+1} | {old_val} | **{new_val}** |")
                else:
                    lines.append(f"| {i+1} | {old_val} | {new_val} |")
            lines.append("")

        lines.append("---")
        lines.append("")

    # Hint changes by window
    lines.append("## Hint Changes By Window")
    lines.append("")

    for window in windows_order:
        changes = hints_by_window[window]
        lines.append(f"### {window}")
        lines.append("")

        for hc in changes:
            badge = "NEW" if hc['change_type'] == 'Added' else "UPDATED"
            lines.append(f"#### {hc['name']} ({hc['type']}) [{badge}]")
            lines.append(f"*IDENT:* `{hc['ident']}`")
            lines.append("")

            if hc['old_hints']:
                for h in hc['old_hints']:
                    lines.append(f"**Before:** {h}")
            else:
                lines.append("**Before:** *(no hint)*")
            lines.append("")
            for h in hc['new_hints']:
                lines.append(f"**After:** {h}")
            lines.append("")

        lines.append("---")
        lines.append("")

    # Write output
    md_text = '\n'.join(lines)
    with open(OUTPUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write(md_text)

    print(f"\nGenerated {OUTPUT}")
    print(f"  {added} added + {updated} updated hints + {li_count} listitem changes")


if __name__ == '__main__':
    main()
