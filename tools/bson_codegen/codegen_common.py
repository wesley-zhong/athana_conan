#!/usr/bin/env python3
"""Shared DO-header parsing for the codegen scripts (bson / json)."""
import re
from pathlib import Path
from dataclasses import dataclass
from typing import Dict, List, Optional


# =========================================================
# Models
# =========================================================

@dataclass
class Field:
    type: str
    name: str
    bson_name: str
    flatten: bool
    skip: bool


@dataclass
class TypeDef:
    name: str
    base: Optional[str]
    fields: List[Field]
    ignore_base: bool


# =========================================================
# Regex
# =========================================================

TYPE_RE = re.compile(
    r"(struct|class)\s+(\w+)(?:\s*:\s*([\w:<>,\s]+?))?\s*\{(.*?)\};",
    re.S
)

FIELD_RE = re.compile(
    r"""
    (?P<anno>(?:@\w+\([^)]*\)\s*)*)
    (?P<type>[\w:<>\s]+?)
    \s+
    (?P<name>\w+)
    \s*;
    """,
    re.X
)


# =========================================================
# Annotation Parsing
# =========================================================

def parse_annotations(anno: str):
    meta = {}
    if "@bson(flatten)" in anno:
        meta["flatten"] = True
    if "@bson(ignore)" in anno:
        meta["skip"] = True
    m = re.search(r'@bson\s*\(\s*rename\s*=\s*"([^"]+)"\s*\)', anno)
    if m:
        meta["rename"] = m.group(1)
    return meta


# =========================================================
# Helpers
# =========================================================

def is_template_type(t: str) -> bool:
    return "<" in t and ">" in t


def extract_public(body: str) -> str:
    m = re.search(r"public\s*:(.*?)(private:|protected:|$)", body, re.S)
    return m.group(1) if m else ""


def normalize_type(t: str) -> str:
    return t.replace(" ", "")


def is_method_decl(text: str, start: int) -> bool:
    # "std::string toString() const override;" -> the match tail "const override;"
    # is preceded by ')', it is a method declaration, not a field
    i = start - 1
    while i >= 0 and text[i].isspace():
        i -= 1
    return i >= 0 and text[i] == ")"


# =========================================================
# Parsing
# =========================================================

def parse_fields(text: str) -> List[Field]:
    fields = []
    for m in FIELD_RE.finditer(text):
        if is_method_decl(text, m.start()):
            continue
        anno = m.group("anno") or ""
        meta = parse_annotations(anno)
        fields.append(Field(
            type=m.group("type").strip(),
            name=m.group("name"),
            bson_name=meta.get("rename", m.group("name")),
            flatten=meta.get("flatten", False),
            skip=meta.get("skip", False),
        ))
    return fields


def parse_first_base(bases: Optional[str]) -> Optional[str]:
    # "public A, public B" -> "A": only the first base participates in codegen
    if not bases:
        return None
    first = bases.split(",")[0].strip()
    first = re.sub(r"^(public|protected|private)\s+", "", first).strip()
    return first or None


def parse_types(text: str) -> Dict[str, TypeDef]:
    types = {}
    for m in TYPE_RE.finditer(text):
        kind, name, bases, body = m.groups()
        ignore_base = "@bson(ignore_base)" in text[:m.start()]
        base = parse_first_base(bases)
        base = None if (not base or is_template_type(base)) else base
        body = extract_public(body) if kind == "class" else body
        types[name] = TypeDef(
            name=name,
            base=base,
            ignore_base=ignore_base,
            fields=parse_fields(body)
        )
    return types


def collect_types(src: Path) -> Dict[str, TypeDef]:
    headers = [src] if src.is_file() else list(src.rglob("*.hpp"))
    all_types: Dict[str, TypeDef] = {}
    for h in headers:
        all_types.update(parse_types(h.read_text(encoding="utf-8")))
    return all_types
