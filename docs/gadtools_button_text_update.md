# Updating Text Inside a GadTools Button (Workbench 3.0)

On classic Amiga Workbench 3.0 with **GadTools** `BUTTON_KIND` gadgets, changing the text on a button at runtime is more involved than just setting an attribute. This guide shows the practical pattern that works on WB 2.x/3.x.

---

## 1. Why it’s not just `GT_SetGadgetAttrs(GA_Text, …)`

On **Workbench 3.0 / GadTools `BUTTON_KIND`**:

- `BUTTON_KIND` gadgets do **not** have a proper `GA_Text`/label attribute for runtime changes.
- The button label comes from **`NewGadget.ng_GadgetText`**, which is just a **pointer** to your string.
- GadTools **does not copy** the string; it only stores the pointer.

So you don’t change the label via `GT_SetGadgetAttrs()`. Instead, you:

1. Give the label its own mutable buffer.
2. Point `ng_GadgetText` at that buffer when you create the gadget.
3. Later, overwrite the contents of that buffer.
4. Refresh the gadget so Intuition redraws it.

---

## 2. Step 1 – Give each button its own mutable buffer

When you create the gadget, don’t use a string literal or a stack buffer. Use a static or allocated buffer that stays valid for the life of the gadget:

```c
static char cancelLabel[16] = "Cancel";   /* must outlive the gadget */

struct NewGadget ng;
struct Gadget *cancelGad;

/* ... fill in ng position, flags, ng_TextAttr, ng_VisualInfo, etc ... */
ng.ng_GadgetText = cancelLabel;

cancelGad = CreateGadget(BUTTON_KIND, prevGad, &ng, TAG_DONE);
```

Because GadTools only keeps the pointer, when `cancelLabel` changes later, the button will draw whatever text is now stored in that buffer.

> **Note:** Don’t reuse the same `char *` for several buttons unless you want all of them to change label together. Give each button its own buffer.

---

## 3. Step 2 – Change the contents of the buffer

When you want to change the text (for example, `Cancel` → `Stopping…`), overwrite the same buffer you used in `ng_GadgetText`:

```c
void SetCancelButtonText(struct Window *win,
                         struct Gadget *cancelGad,
                         const char *newText)
{
    extern char cancelLabel[16];  /* same buffer used in ng_GadgetText */

    strncpy(cancelLabel, newText, sizeof(cancelLabel) - 1);
    cancelLabel[sizeof(cancelLabel) - 1] = '\0';

    /* See Step 3 for how to refresh the gadget */
}
```

At this point, the underlying text data has changed, but the gadget will not visually update until Intuition redraws it.

---

## 4. Step 3 – Refresh the gadget so the new label appears

After changing the buffer, you normally need to force a redraw so the new label shows immediately.

A pragmatic pattern that works on WB 2.x/3.x is:

```c
void SetCancelButtonText(struct Window *win,
                         struct Gadget *cancelGad,
                         const char *newText)
{
    extern char cancelLabel[16];

    strncpy(cancelLabel, newText, sizeof(cancelLabel) - 1);
    cancelLabel[sizeof(cancelLabel) - 1] = '\0';

    /* Minimal redraw – this gadget only */
    RefreshGList(cancelGad, win, NULL, 1);

    /* Optional: if needed on your setup, you can follow with:
     *
     *   GT_RefreshWindow(win, NULL);
     *
     * but many programs get away with just RefreshGList().
     */
}
```

If you skip the explicit refresh and rely on the window being refreshed later (moved, uncovered, or redrawn by you), the label *will* eventually update, but not immediately.

---

## 5. Summary checklist for your “Cancel” button

For a typical “Cancel”/“Stop”/“Close” button in your program:

1. **Creation time**
   - Put the label in a static/allocated buffer, e.g. `static char cancelLabel[16] = "Cancel";`
   - Set `ng.ng_GadgetText = cancelLabel;` before calling `CreateGadget(BUTTON_KIND, ...)`.

2. **When changing the label**
   - `strncpy()` the new text into `cancelLabel` and terminate with `'\0'`.
   - Call `RefreshGList(cancelGad, win, NULL, 1);`
   - Optionally call `GT_RefreshWindow(win, NULL);` if needed.

That’s the standard “mutable buffer + refresh” pattern used by classic GadTools applications on Workbench 2.x/3.x.
