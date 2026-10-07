/*
 * line_editor.c — a simple command-line line editor in C.
 *
 * The document is held in memory as a dynamic array of strings
 * (char **).  All commands are line-number based:
 *
 *   i <n> <text>    insert <text> as line n (1-based; n = count+1 appends)
 *   d <n>           delete line n
 *   p               print the whole document with line numbers
 *   p <n>           print only line n
 *   s <file>        save the document to <file>
 *   l <file>        load <file>, replacing the current document
 *   f <word>        find: list line numbers containing <word>
 *   r <n> <o> <nw>  replace word <o> with <nw> on line n
 *   ra <o> <nw>     replace word <o> with <nw> on every line
 *   c               show line / word / character counts
 *   h               show this help
 *   q               quit (asks for confirmation if there are unsaved changes)
 *
 * Build:  gcc -Wall -Wextra -std=c11 -o line_editor line_editor.c
 * Run:    ./line_editor
 */

#define _POSIX_C_SOURCE 200809L   /* getline() */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Document: dynamic array of heap-allocated C strings.                */
/* ------------------------------------------------------------------ */

typedef struct {
    char  **lines;     /* array of char* (each a NUL-terminated line) */
    size_t  count;     /* number of lines currently stored           */
    size_t  capacity;  /* allocated slots in lines[]                   */
} Document;

static void doc_init(Document *doc)
{
    doc->lines    = NULL;
    doc->count    = 0;
    doc->capacity = 0;
}

static void doc_free(Document *doc)
{
    size_t i;
    for (i = 0; i < doc->count; i++)
        free(doc->lines[i]);
    free(doc->lines);
    doc_init(doc);
}

/* Insert text as 1-based line number n.  Valid n: 1 .. count+1.
 * Returns 0 on success, -1 on invalid line number or allocation failure. */
static int doc_insert(Document *doc, size_t n, const char *text)
{
    size_t idx;
    char **tmp, *copy;

    if (n < 1 || n > doc->count + 1)
        return -1;

    if (doc->count == doc->capacity) {
        size_t newcap = doc->capacity ? doc->capacity * 2 : 8;
        tmp = realloc(doc->lines, newcap * sizeof *tmp);
        if (!tmp)
            return -1;
        doc->lines    = tmp;
        doc->capacity = newcap;
    }

    copy = strdup(text);
    if (!copy)
        return -1;

    idx = n - 1;
    memmove(&doc->lines[idx + 1], &doc->lines[idx],
            (doc->count - idx) * sizeof *doc->lines);
    doc->lines[idx] = copy;
    doc->count++;
    return 0;
}

/* Delete 1-based line number n.  Returns 0 on success, -1 if invalid. */
static int doc_delete(Document *doc, size_t n)
{
    size_t idx;

    if (n < 1 || n > doc->count)
        return -1;

    idx = n - 1;
    free(doc->lines[idx]);
    memmove(&doc->lines[idx], &doc->lines[idx + 1],
            (doc->count - idx - 1) * sizeof *doc->lines);
    doc->count--;
    return 0;
}

/* Print the whole document with 1-based line numbers. */
static void doc_display(const Document *doc)
{
    size_t i;
    if (doc->count == 0) {
        puts("(empty document)");
        return;
    }
    for (i = 0; i < doc->count; i++)
        printf("%4zu  %s\n", i + 1, doc->lines[i]);
}

/* Save the document to a text file, one line per row. */
static int doc_save(const Document *doc, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    size_t i;

    if (!fp)
        return -1;
    for (i = 0; i < doc->count; i++)
        fprintf(fp, "%s\n", doc->lines[i]);
    fclose(fp);
    return 0;
}

/* Load a text file, replacing the current document.
 * Returns 0 on success, -1 if the file cannot be read.  The old
 * document is left untouched on failure. */
static int doc_load(Document *doc, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    Document tmp;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    if (!fp)
        return -1;

    doc_init(&tmp);
    while ((nread = getline(&line, &len, fp)) != -1) {
        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';
        if (doc_insert(&tmp, tmp.count + 1, line) != 0) {
            doc_free(&tmp);
            free(line);
            fclose(fp);
            return -1;
        }
    }
    free(line);
    fclose(fp);

    doc_free(doc);
    *doc = tmp;
    return 0;
}

/* Print the 1-based line numbers that contain word.  Returns matches. */
static size_t doc_search(const Document *doc, const char *word)
{
    size_t i, found = 0;

    for (i = 0; i < doc->count; i++) {
        if (strstr(doc->lines[i], word)) {
            printf("line %zu: %s\n", i + 1, doc->lines[i]);
            found++;
        }
    }
    if (found == 0)
        printf("'%s' not found.\n", word);
    return found;
}

/* Replace every occurrence of oldw with neww inside *linep (reallocates).
 * Returns the number of replacements made. */
static size_t str_replace_all(char **linep, const char *oldw, const char *neww)
{
    char *line = *linep;
    size_t oldlen = strlen(oldw), newlen = strlen(neww);
    size_t count = 0, cap, used = 0;
    char *out, *pos, *prev;

    if (oldlen == 0)
        return 0;

    for (pos = line; (pos = strstr(pos, oldw)) != NULL; pos += oldlen)
        count++;
    if (count == 0)
        return 0;

    cap = strlen(line) + count * (newlen - oldlen) + 1;
    out = malloc(cap);
    if (!out)
        return 0;

    prev = line;
    while ((pos = strstr(prev, oldw)) != NULL) {
        size_t seg = (size_t)(pos - prev);
        memcpy(out + used, prev, seg);
        used += seg;
        memcpy(out + used, neww, newlen);
        used += newlen;
        prev = pos + oldlen;
    }
    strcpy(out + used, prev);

    free(line);
    *linep = out;
    return count;
}

/* Replace on a single 1-based line.  Returns replacements, or -1 if the
 * line number is invalid. */
static long doc_replace_line(Document *doc, size_t n,
                             const char *oldw, const char *neww)
{
    if (n < 1 || n > doc->count)
        return -1;
    return (long)str_replace_all(&doc->lines[n - 1], oldw, neww);
}

/* Replace across the whole document.  Returns total replacements. */
static size_t doc_replace_all(Document *doc, const char *oldw, const char *neww)
{
    size_t i, total = 0;
    for (i = 0; i < doc->count; i++)
        total += str_replace_all(&doc->lines[i], oldw, neww);
    return total;
}

/* Print line / word / character statistics. */
static void doc_stats(const Document *doc)
{
    size_t i, words = 0, chars = 0;

    for (i = 0; i < doc->count; i++) {
        const char *p = doc->lines[i];
        int inword = 0;
        chars += strlen(p);
        while (*p) {
            if (isspace((unsigned char)*p))
                inword = 0;
            else if (!inword) {
                inword = 1;
                words++;
            }
            p++;
        }
    }
    printf("lines: %zu   words: %zu   characters: %zu\n",
           doc->count, words, chars);
}

/* ------------------------------------------------------------------ */
/* Command parsing helpers.                                            */
/* ------------------------------------------------------------------ */

/* Strip the trailing newline left by getline(). */
static void chomp(char *s)
{
    size_t n = strlen(s);
    if (n > 0 && s[n - 1] == '\n')
        s[n - 1] = '\0';
}

/* Skip leading whitespace; returns pointer to first non-space char. */
static char *skip_spaces(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    return s;
}

/* Parse an unsigned 1-based line number from *pp, advancing *pp past it.
 * Returns 1 on success, 0 on failure. */
static int parse_number(char **pp, size_t *out)
{
    char *end;
    unsigned long v;

    *pp = skip_spaces(*pp);
    if (!isdigit((unsigned char)**pp))
        return 0;
    v = strtoul(*pp, &end, 10);
    if (end == *pp || v == 0 || v > 1000000)
        return 0;
    *out = (size_t)v;
    *pp = end;
    return 1;
}

static void print_help(void)
{
    puts("Commands:");
    puts("  i <n> <text>     insert <text> as line n (n = lines+1 appends)");
    puts("  d <n>            delete line n");
    puts("  p                print the whole document with line numbers");
    puts("  p <n>            print only line n");
    puts("  s <file>         save the document to <file>");
    puts("  l <file>         load <file>, replacing the current document");
    puts("  f <word>         find lines containing <word>");
    puts("  r <n> <o> <nw>   replace word <o> with <nw> on line n");
    puts("  ra <o> <nw>      replace word <o> with <nw> on every line");
    puts("  c                show line / word / character counts");
    puts("  h                show this help");
    puts("  q                quit (asks first if there are unsaved changes)");
}

/* ------------------------------------------------------------------ */
/* Main command loop.                                                  */
/* ------------------------------------------------------------------ */

int main(void)
{
    Document doc;
    char *input = NULL;
    size_t cap = 0;
    ssize_t nread;
    int dirty = 0;   /* set when the document changes since last save */

    doc_init(&doc);

    puts("Simple Line Editor — type 'h' for help, 'q' to quit.");

    while (1) {
        char *p, cmd;

        printf("> ");
        fflush(stdout);
        nread = getline(&input, &cap, stdin);
        if (nread == -1) {          /* EOF (Ctrl+D) behaves like quit */
            putchar('\n');
            break;
        }
        chomp(input);
        p = skip_spaces(input);
        if (*p == '\0')
            continue;

        cmd = *p;
        p++;

        switch (cmd) {

        case 'i': {   /* insert: i <n> <text> */
            size_t n;
            char *text;
            if (!parse_number(&p, &n)) {
                puts("usage: i <line-number> <text>");
                break;
            }
            p = skip_spaces(p);
            text = p;
            if (*text == '\0') {
                puts("usage: i <line-number> <text>");
                break;
            }
            if (n > doc.count + 1) {
                printf("invalid line number (document has %zu lines; "
                       "use 1..%zu to insert, %zu appends)\n",
                       doc.count, doc.count + 1, doc.count + 1);
                break;
            }
            if (doc_insert(&doc, n, text) != 0) {
                puts("insert failed (out of memory).");
                break;
            }
            dirty = 1;
            printf("inserted as line %zu.\n", n);
            break;
        }

        case 'd': {   /* delete: d <n> */
            size_t n;
            if (!parse_number(&p, &n) || *skip_spaces(p) != '\0') {
                puts("usage: d <line-number>");
                break;
            }
            if (doc_delete(&doc, n) != 0) {
                printf("invalid line number (document has %zu lines).\n",
                       doc.count);
                break;
            }
            dirty = 1;
            printf("deleted line %zu.\n", n);
            break;
        }

        case 'p': {   /* print: p  |  p <n> */
            size_t n;
            p = skip_spaces(p);
            if (*p == '\0') {
                doc_display(&doc);
            } else if (parse_number(&p, &n) && *skip_spaces(p) == '\0') {
                if (n < 1 || n > doc.count)
                    printf("invalid line number (document has %zu lines).\n",
                           doc.count);
                else
                    printf("%4zu  %s\n", n, doc.lines[n - 1]);
            } else {
                puts("usage: p [line-number]");
            }
            break;
        }

        case 's': {   /* save: s <file> */
            char *fname = skip_spaces(p);
            if (*fname == '\0') {
                puts("usage: s <filename>");
                break;
            }
            if (doc_save(&doc, fname) != 0) {
                printf("could not write '%s'.\n", fname);
                break;
            }
            dirty = 0;
            printf("saved %zu lines to '%s'.\n", doc.count, fname);
            break;
        }

        case 'l': {   /* load: l <file> */
            char *fname = skip_spaces(p);
            if (*fname == '\0') {
                puts("usage: l <filename>");
                break;
            }
            if (doc_load(&doc, fname) != 0) {
                printf("could not read '%s'.\n", fname);
                break;
            }
            dirty = 1;
            printf("loaded %zu lines from '%s'.\n", doc.count, fname);
            break;
        }

        case 'f': {   /* find: f <word> */
            char *word = skip_spaces(p);
            if (*word == '\0') {
                puts("usage: f <word>");
                break;
            }
            doc_search(&doc, word);
            break;
        }

        case 'r': {   /* replace: r <n> <old> <new>  |  ra <old> <new> */
            if (*p == 'a' || *p == 'A') {          /* replace-all branch */
                char oldw[256], neww[256];
                size_t total;
                p++;
                if (sscanf(p, "%255s %255s", oldw, neww) != 2) {
                    puts("usage: ra <old-word> <new-word>");
                    break;
                }
                total = doc_replace_all(&doc, oldw, neww);
                if (total > 0)
                    dirty = 1;
                printf("replaced %zu occurrence(s).\n", total);
            } else {                              /* single-line branch */
                size_t n;
                char oldw[256], neww[256], extra;
                long made;
                if (!parse_number(&p, &n) ||
                    sscanf(p, "%255s %255s %c", oldw, neww, &extra) != 2) {
                    puts("usage: r <line-number> <old-word> <new-word>");
                    break;
                }
                made = doc_replace_line(&doc, n, oldw, neww);
                if (made < 0) {
                    printf("invalid line number (document has %zu lines).\n",
                           doc.count);
                    break;
                }
                if (made > 0)
                    dirty = 1;
                printf("replaced %ld occurrence(s) on line %zu.\n", made, n);
            }
            break;
        }

        case 'c':   /* counts */
            doc_stats(&doc);
            break;

        case 'h':   /* help */
            print_help();
            break;

        case 'q': { /* quit */
            if (dirty) {
                char ans[16];
                printf("you have unsaved changes — quit anyway? (y/n) ");
                fflush(stdout);
                if (!fgets(ans, sizeof ans, stdin) ||
                    (ans[0] != 'y' && ans[0] != 'Y')) {
                    puts("quit cancelled.");
                    break;
                }
            }
            free(input);
            doc_free(&doc);
            puts("bye.");
            return 0;
        }

        default:
            printf("unknown command '%c' — type 'h' for help.\n", cmd);
            break;
        }
    }

    free(input);
    doc_free(&doc);
    return 0;
}
