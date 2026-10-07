# Design — Simple Line Editor in C

*Written before any code was typed. This is the paper-design deliverable
for Activity 7 (typed here so it can be committed; the hand-written
function sketches below can be copied onto paper and photographed if the
evaluator wants the literal paper version).*

## 1. Data structure choice

**Chosen: dynamic array of strings (`char **lines` + `count` + `capacity`).**

The document is an ordered sequence of lines, and every command in a line
editor is addressed by **line number**. That makes random access by index
the single most important operation.

| Option | Access line n | Insert/delete at n | Verdict |
|---|---|---|---|
| Dynamic array of `char *` | O(1) | O(n) shift | ✅ Chosen |
| Singly linked list of lines | O(n) walk | O(n) walk + O(1) splice | ❌ every command pays O(n) just to *reach* the line |
| Fixed-size array (`char lines[100][256]`) | O(1) | O(n) shift | ❌ arbitrary limits on line count *and* line length |

Justification:
- **O(1) line access** matches the problem: `d 5`, `p 3`, `r 2 …` all
  index directly. A linked list would turn each of these into a traversal.
- **O(n) insert/delete** is acceptable because a line editor holds small
  documents (tens to hundreds of lines); the shift is a single `memmove`.
- **No hard limits**: the array doubles (`8 → 16 → 32 …`) when full, and
  each line is a separately allocated string, so lines can be any length
  (input is read with `getline()`, which grows the buffer as needed).
- **Memory**: exactly one pointer per line plus the text itself — no
  per-node `next` overhead, contiguous and cache-friendly.

## 2. Command set

Single-letter commands (in the spirit of classic `ed`), all 1-based:

| Command | Meaning |
|---|---|
| `i <n> <text>` | insert `<text>` as line `n`; `n = count+1` appends |
| `d <n>` | delete line `n` |
| `p` / `p <n>` | print all lines (numbered) / print one line |
| `s <file>` | save document to `<file>` |
| `l <file>` | load `<file>`, replacing the document |
| `f <word>` | list line numbers containing `<word>` (bonus) |
| `r <n> <old> <new>` | replace `<old>` → `<new>` on line `n` (bonus) |
| `ra <old> <new>` | replace `<old>` → `<new>` on all lines (bonus) |
| `c` | line / word / character counts (bonus) |
| `h` | help |
| `q` | quit (confirms first if there are unsaved changes) |

## 3. Core function sketches (hand-write these on paper)

**insert(doc, n, text)** — n is 1-based
```
if n < 1 or n > doc.count + 1:  return ERROR
if doc.count == doc.capacity:
    capacity = capacity ? capacity*2 : 8
    lines = realloc(lines, capacity * sizeof(char*))
idx = n - 1
memmove(lines[idx+1 ..], lines[idx ..])   // shift right
lines[idx] = strdup(text)
doc.count += 1
```

**delete(doc, n)** — n is 1-based
```
if n < 1 or n > doc.count:  return ERROR
idx = n - 1
free(lines[idx])
memmove(lines[idx ..], lines[idx+1 ..])   // shift left
doc.count -= 1
```

**display(doc)**
```
if doc.count == 0:  print "(empty document)"
else for i in 0 .. doc.count-1:
    print (i+1) and lines[i]
```

## 4. Edge cases (handled, not crashing)

- Insert/delete/print/replace with line number `0`, negative, or past the
  end → clear error message naming the valid range.
- Commands on an empty document → sensible messages (`(empty document)`).
- `q` with unsaved changes → asks `quit anyway? (y/n)`.
- Save/load with unreadable paths → error message, document untouched.
- Arbitrary line lengths and document sizes → `getline()` + doubling array.
- Unknown command → `unknown command 'x' — type 'h' for help.`

## 5. What was deliberately left out

**Undo** (bonus): reversing insert/delete would need an action stack storing
copies of affected lines. Correct but the most implementation-heavy bonus;
with the other three bonuses (search, replace, counts) already done, undo
was cut to keep the code base small and bug-free.
