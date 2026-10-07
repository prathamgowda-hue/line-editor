# Simple Line Editor in C — Activity 7

A command-line line editor built in C for the Portfolio Building studio
course coding competition (Activity 7). It holds the document in memory as
a **dynamic array of strings** and operates on it one line at a time —
insert, delete, display, save/load, plus search, find-and-replace, and
document statistics.

## Team

- Pratham VG Gowda (B25CS0311) — CSE, 2nd year, REVA University, Bengaluru
- Naman Vijaykumar (R25EJ085)
- Nandeesh.S (R25EJ086)

## Features implemented

Core (4 of 4):
- Insert a line at a given line number
- Delete a line
- Display the document with line numbers
- Save to / load from a `.txt` file

Bonus (3):
- Search — find line numbers containing a word
- Find & replace — on one line (`r`) or the whole document (`ra`)
- Line / word / character counts (`c`)

## How to compile and run

Requires `gcc` (any recent version; tested on Linux).

```sh
gcc -Wall -Wextra -std=c11 -o line_editor line_editor.c
./line_editor
```

The editor is interactive — type `h` once inside it for the command list,
or read [HELP.md](HELP.md) for every command with a usage example.

## Quick demo

```
$ ./line_editor
Simple Line Editor — type 'h' for help, 'q' to quit.
> i 1 Hello world
inserted as line 1.
> i 2 Second line
inserted as line 2.
> f line
line 2: Second line
> ra line LINE
replaced 1 occurrence(s).
> s demo.txt
saved 2 lines to 'demo.txt'.
> q
bye.
```

## Repository structure

| File | What it is |
|---|---|
| `line_editor.c` | The whole editor (~600 lines, single file, no dependencies beyond libc) |
| `DESIGN.md` | Paper-design deliverable: data-structure choice with trade-off table, command set, hand-writable function sketches |
| `HELP.md` | Help file: every command with syntax and a usage example |
| `README.md` | This file |

## Design notes (short version)

Dynamic array of `char *` was chosen over a linked list because every
command is line-number based — indexing is O(1) instead of an O(n) walk
per command — and over a fixed array because there are no hard limits on
line count or line length. Inserts/deletes are O(n) `memmove` shifts,
which is fine for the small documents a line editor holds. Full rationale
in [DESIGN.md](DESIGN.md).
