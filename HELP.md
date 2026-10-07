# Line Editor — Help

A tiny command-line line editor. Line numbers start at **1**.
Type `h` inside the editor to see this summary.

## Core commands

### `i <n> <text>` — insert a line
Adds `<text>` as line `n`, pushing existing lines down.
Use `n = (current lines + 1)` to append at the end.

```
> i 1 Hello world
inserted as line 1.
> i 1 First line actually
inserted as line 1.
> p
   1  First line actually
   2  Hello world
```

### `d <n>` — delete a line
Removes line `n`, pulling the lines below it up.

```
> d 2
deleted line 2.
```

### `p` — display the document
Prints every line with its line number.

```
> p
   1  First line actually
```

### `p <n>` — display one line
```
> p 1
   1  First line actually
```

### `s <file>` — save to a file
Writes the document to `<file>` (one line per row).

```
> s notes.txt
saved 1 lines to 'notes.txt'.
```

### `l <file>` — load a file
Reads `<file>`, replacing the current document.

```
> l notes.txt
loaded 1 lines from 'notes.txt'.
```

## Bonus commands

### `f <word>` — search
Lists every line number containing `<word>`.

```
> f line
line 1: First line actually
```

### `r <n> <old> <new>` — find & replace on one line
Replaces every occurrence of `<old>` with `<new>` on line `n`.

```
> r 1 actually really
replaced 1 occurrence(s) on line 1.
```

### `ra <old> <new>` — find & replace everywhere
Replaces every occurrence of `<old>` with `<new>` on all lines.

```
> ra line sentence
replaced 1 occurrence(s).
```

### `c` — line / word / character counts
```
> c
lines: 1   words: 3   characters: 19
```

## Session commands

### `h` — help
Prints the command summary.

### `q` — quit
Exits the editor. If you have unsaved changes, it asks for confirmation first:

```
> q
you have unsaved changes — quit anyway? (y/n) y
bye.
```

Press `Ctrl+D` (EOF) any time to quit immediately.
