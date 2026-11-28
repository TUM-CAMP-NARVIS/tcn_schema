"""A YAML subset reader, standard library only (the IDL tools take no third-party packages).

Enough YAML for hand-written tables such as tcnart_msgs/msg/joint_sets.yaml:
- `#` comments (at a line start, or after whitespace outside quotes);
- block mappings and sequences (a sequence may sit at its key's indentation), `- key: value` items;
- flow mappings `{a: 1}` and lists `[a, b]`, nested, possibly over several lines;
- plain, 'single'- and "double"-quoted scalars; a plain scalar continues on deeper-indented lines, joined with one
  space;
- `null`/`~`, `true`/`false`, decimal integers, floats with a dot (as YAML 1.1 resolves them).

Everything else raises `YamlSubsetError` with the 1-based line, rather than being read differently than a YAML
library would read it: anchors, aliases, tags, `|`/`>` block scalars, several documents, directives, complex keys,
tabs in indentation, duplicate keys, and the YAML 1.1 words `yes`/`no`/`on`/`off` (quote them).
"""
import re
from typing import Any, List, Optional, Tuple

__all__ = ["YamlSubsetError", "load"]


class YamlSubsetError(ValueError):
    def __init__(self, line: int, message: str):
        super().__init__(f"line {line}: {message}")
        self.line = line


_INT = re.compile(r"[-+]?(0|[1-9][0-9]*)")
_FLOAT = re.compile(r"[-+]?([0-9]+\.[0-9]*|\.[0-9]+)([eE][-+][0-9]+)?")
_FLOAT_SPECIAL = {".inf": float("inf"), "+.inf": float("inf"), "-.inf": float("-inf"), ".nan": float("nan")}
_NULL = {"", "~", "null", "Null", "NULL"}
_BOOL = {"true": True, "True": True, "TRUE": True, "false": False, "False": False, "FALSE": False}
_YAML11_BOOL = {"yes", "Yes", "YES", "no", "No", "NO", "on", "On", "ON", "off", "Off", "OFF"}
_NUMBER_LIKE = re.compile(r"[-+]?(0[0-9]+|[0-9][0-9_]*([eE][-+]?[0-9]+)?|0[xXoObB][0-9a-fA-F_]+|[0-9:]+:[0-9]+)")
# Characters after which a quote opens a quoted scalar (otherwise it is part of a plain scalar, e.g. `it's`).
_QUOTE_OPENERS = ":-[{,"


class _Line:
    __slots__ = ("no", "indent", "text")

    def __init__(self, no: int, indent: int, text: str):
        self.no, self.indent, self.text = no, indent, text


def _strip_comment(raw: str) -> str:
    """The line without its comment and trailing whitespace. Quotes are tracked only where a scalar can start."""
    quote = None
    prev = ""  # the last non-space character outside quotes
    i = 0
    while i < len(raw):
        c = raw[i]
        if quote:
            if quote == "'" and c == "'":
                if i + 1 < len(raw) and raw[i + 1] == "'":
                    i += 2
                    continue
                quote = None
            elif quote == '"' and c == "\\":
                i += 2
                continue
            elif quote == '"' and c == '"':
                quote = None
            i += 1
            continue
        if c == "#" and (i == 0 or raw[i - 1] in " \t"):
            return raw[:i].rstrip()
        if c in "'\"" and (prev == "" or prev in _QUOTE_OPENERS) and (i == 0 or raw[i - 1] in " \t[{,"):
            quote = c
        if not c.isspace():
            prev = c
        i += 1
    return raw.rstrip()


def _lines(text: str) -> List[_Line]:
    out = []
    for no, raw in enumerate(text.splitlines(), 1):
        body = _strip_comment(raw)
        if not body.strip():
            continue
        lead = len(body) - len(body.lstrip(" \t"))
        if "\t" in body[:lead]:
            raise YamlSubsetError(no, "tab in indentation")
        content = body[lead:]
        if lead == 0 and (content in ("---", "...") or content.startswith(("--- ", "%"))):
            raise YamlSubsetError(no, "document markers and directives are not supported (one document only)")
        out.append(_Line(no, lead, content))
    return out


def _check_plain_start(s: str, no: int) -> None:
    if not s:
        return
    if s[0] in "&*!":
        what = {"&": "anchors", "*": "aliases", "!": "tags"}[s[0]]
        raise YamlSubsetError(no, f"{what} are not supported")
    if s[0] in "|>":
        raise YamlSubsetError(no, "block scalars (| and >) are not supported")
    if s[0] in "@`%":
        raise YamlSubsetError(no, f"a plain scalar cannot start with {s[0]!r}")
    if s.startswith("? ") or s == "?":
        raise YamlSubsetError(no, "complex keys are not supported")


def _resolve_plain(s: str, no: int) -> Any:
    if s in _NULL:
        return None
    if s in _BOOL:
        return _BOOL[s]
    if s in _YAML11_BOOL:
        raise YamlSubsetError(no, f"{s!r} is a boolean in YAML 1.1 and a string in YAML 1.2; quote it")
    if _INT.fullmatch(s):
        return int(s)
    if _FLOAT.fullmatch(s):
        return float(s)
    if s in _FLOAT_SPECIAL:
        return _FLOAT_SPECIAL[s]
    if _NUMBER_LIKE.fullmatch(s):
        raise YamlSubsetError(no, f"{s!r} reads differently in YAML 1.1 and 1.2; quote it or write a plain number")
    return s


def _parse_quoted(s: str, i: int, no: int) -> Tuple[str, int]:
    """Parse the quoted scalar starting at s[i]; return (value, index after the closing quote)."""
    q = s[i]
    out = []
    j = i + 1
    while j < len(s):
        c = s[j]
        if q == "'":
            if c == "'":
                if j + 1 < len(s) and s[j + 1] == "'":
                    out.append("'")
                    j += 2
                    continue
                return "".join(out), j + 1
            out.append(c)
        else:
            if c == '"':
                return "".join(out), j + 1
            if c == "\\":
                if j + 1 >= len(s):
                    break
                e = s[j + 1]
                simple = {"\\": "\\", '"': '"', "/": "/", "n": "\n", "t": "\t", "r": "\r", "0": "\0", " ": " "}
                if e in simple:
                    out.append(simple[e])
                    j += 2
                    continue
                if e in "xuU":
                    n = {"x": 2, "u": 4, "U": 8}[e]
                    digits = s[j + 2:j + 2 + n]
                    if len(digits) != n or not re.fullmatch(r"[0-9a-fA-F]+", digits):
                        raise YamlSubsetError(no, f"bad escape \\{e}{digits}")
                    out.append(chr(int(digits, 16)))
                    j += 2 + n
                    continue
                raise YamlSubsetError(no, f"unsupported escape \\{e}")
            out.append(c)
        j += 1
    raise YamlSubsetError(no, "unterminated quoted scalar")


class _Flow:
    """Parser of one flow value (`[...]`, `{...}` or a scalar) in a single string."""

    def __init__(self, s: str, no: int):
        self.s, self.i, self.no = s, 0, no

    def ws(self) -> None:
        while self.i < len(self.s) and self.s[self.i] in " \t":
            self.i += 1

    def peek(self) -> str:
        return self.s[self.i] if self.i < len(self.s) else ""

    def value(self, terminators: str) -> Any:
        self.ws()
        c = self.peek()
        if c == "[":
            return self.seq()
        if c == "{":
            return self.map()
        if c in "'\"":
            v, self.i = _parse_quoted(self.s, self.i, self.no)
            return v
        return self.plain(terminators)

    def plain(self, terminators: str) -> Any:
        start = self.i
        while self.i < len(self.s):
            c = self.s[self.i]
            if c in terminators:
                break
            if c == ":" and ":" in terminators and (self.i + 1 == len(self.s) or self.s[self.i + 1] in " ,]}"):
                break
            self.i += 1
        text = self.s[start:self.i].strip()
        _check_plain_start(text, self.no)
        return _resolve_plain(text, self.no)

    def seq(self) -> list:
        self.i += 1
        out = []
        while True:
            self.ws()
            if self.peek() == "]":
                self.i += 1
                return out
            if not self.peek():
                raise YamlSubsetError(self.no, "unterminated flow sequence")
            out.append(self.value(",]"))
            self.ws()
            if self.peek() == ",":
                self.i += 1
            elif self.peek() != "]":
                raise YamlSubsetError(self.no, f"expected ',' or ']' in a flow sequence, got {self.peek()!r}")

    def map(self) -> dict:
        self.i += 1
        out = {}
        while True:
            self.ws()
            if self.peek() == "}":
                self.i += 1
                return out
            if not self.peek():
                raise YamlSubsetError(self.no, "unterminated flow mapping")
            key = self.value(":,}")
            self.ws()
            if self.peek() == ":":
                self.i += 1
                self.ws()
                val = None if self.peek() in ",}" else self.value(",}")
            else:
                val = None
            if not isinstance(key, (str, int, float, bool)) and key is not None:
                raise YamlSubsetError(self.no, "complex keys are not supported")
            if key in out:
                raise YamlSubsetError(self.no, f"duplicate key {key!r}")
            out[key] = val
            self.ws()
            if self.peek() == ",":
                self.i += 1
            elif self.peek() != "}":
                raise YamlSubsetError(self.no, f"expected ',' or '}}' in a flow mapping, got {self.peek()!r}")

    def done(self) -> None:
        self.ws()
        if self.i != len(self.s):
            raise YamlSubsetError(self.no, f"unexpected text after the value: {self.s[self.i:]!r}")


def _split_key(text: str, no: int) -> Optional[Tuple[Any, str]]:
    """`key: rest` -> (key, rest); None when the text is not a mapping entry."""
    if text[0] in "'\"":
        key, j = _parse_quoted(text, 0, no)
        rest = text[j:].lstrip(" ")
        if rest == ":" or rest.startswith(": "):
            return key, rest[1:].strip()
        return None
    if text[0] in "[{":
        return None
    m = re.search(r":( |$)", text)
    if not m:
        return None
    key = text[:m.start()].rstrip()
    _check_plain_start(key, no)
    return _resolve_plain(key, no), text[m.end():].strip()


class _Block:
    def __init__(self, lines: List[_Line]):
        self.lines = lines
        self.i = 0

    def cur(self) -> Optional[_Line]:
        return self.lines[self.i] if self.i < len(self.lines) else None

    def node(self, indent: int) -> Any:
        """The block node whose lines start at the current line, indented exactly `indent`."""
        line = self.cur()
        if line.text == "-" or line.text.startswith("- "):
            return self.seq(indent)
        if _split_key(line.text, line.no) is not None:
            return self.map(indent)
        return self.inline(line.text, line.no, indent - 1)

    def inline(self, text: str, no: int, parent_indent: int) -> Any:
        """A scalar or flow value starting on line `no`; consumes its continuation lines (indent > parent_indent)."""
        self.i += 1
        if text[0] in "[{":
            while _unbalanced(text, no) and self.cur() is not None and self.cur().indent > parent_indent:
                text += " " + self.cur().text
                self.i += 1
            f = _Flow(text, no)
            v = f.value("")
            f.done()
            return v
        if text[0] in "'\"":
            v, j = _parse_quoted(text, 0, no)
            if text[j:].strip():
                raise YamlSubsetError(no, f"unexpected text after the quoted scalar: {text[j:]!r}")
            if self.cur() is not None and self.cur().indent > parent_indent:
                raise YamlSubsetError(self.cur().no, "multi-line quoted scalars are not supported")
            return v
        _check_plain_start(text, no)
        parts = [text]
        while self.cur() is not None and self.cur().indent > parent_indent:
            parts.append(self.cur().text)
            self.i += 1
        joined = " ".join(parts)
        if len(parts) > 1:
            return joined  # a multi-line plain scalar is always a string
        if ": " in joined or joined.endswith(":"):
            raise YamlSubsetError(no, f"unexpected ':' in a plain scalar: {joined!r}")
        return _resolve_plain(joined, no)

    def value_after(self, rest: str, no: int, indent: int, allow_seq_same_indent: bool) -> Any:
        if rest:
            self.i -= 1  # inline() consumes the current line itself
            return self.inline(rest, no, indent)
        nxt = self.cur()
        if nxt is None or nxt.indent < indent or (nxt.indent == indent and not (
                allow_seq_same_indent and (nxt.text == "-" or nxt.text.startswith("- ")))):
            return None
        return self.node(nxt.indent)

    def map(self, indent: int) -> dict:
        out = {}
        while True:
            line = self.cur()
            if line is None or line.indent < indent:
                return out
            if line.indent > indent:
                raise YamlSubsetError(line.no, "unexpected indentation")
            kv = _split_key(line.text, line.no)
            if kv is None:
                raise YamlSubsetError(line.no, f"expected 'key: value', got {line.text!r}")
            key, rest = kv
            if key in out:
                raise YamlSubsetError(line.no, f"duplicate key {key!r}")
            self.i += 1
            out[key] = self.value_after(rest, line.no, indent, allow_seq_same_indent=True)

    def seq(self, indent: int) -> list:
        out = []
        while True:
            line = self.cur()
            if line is None or line.indent < indent:
                return out
            if line.indent > indent:
                raise YamlSubsetError(line.no, "unexpected indentation")
            if not (line.text == "-" or line.text.startswith("- ")):
                if _split_key(line.text, line.no) is not None:
                    return out  # a sequence at its key's indentation ends at the next key
                raise YamlSubsetError(line.no, f"expected '- item', got {line.text!r}")
            rest = line.text[1:].lstrip(" ")
            if not rest:
                self.i += 1
                out.append(self.value_after("", line.no, indent, allow_seq_same_indent=False))
                continue
            col = indent + (len(line.text) - len(rest))
            if rest == "-" or rest.startswith("- ") or _split_key(rest, line.no) is not None:
                # `- key: value ...` or `- - item`: a block node at the item's column, starting on this line.
                self.lines[self.i] = _Line(line.no, col, rest)
                out.append(self.node(col))
            else:
                out.append(self.inline(rest, line.no, indent))


def _unbalanced(text: str, no: int) -> bool:
    depth, i = 0, 0
    while i < len(text):
        c = text[i]
        if c in "'\"" and (i == 0 or text[i - 1] in " [{,:"):
            try:
                _, i = _parse_quoted(text, i, no)
            except YamlSubsetError:
                return True
            continue
        if c in "[{":
            depth += 1
        elif c in "]}":
            depth -= 1
        i += 1
    return depth > 0


def load(text: str) -> Any:
    """Parse one YAML document of the supported subset."""
    lines = _lines(text)
    if not lines:
        return None
    b = _Block(lines)
    first = lines[0]
    v = b.node(first.indent)
    if b.cur() is not None:
        raise YamlSubsetError(b.cur().no, "unexpected text after the document's top-level node")
    return v
