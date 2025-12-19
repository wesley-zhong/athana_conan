#!/usr/bin/env python3
import re
import sys
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
    r"(struct|class)\s+(\w+)(?:\s*:\s*public\s+([\w:<>]+))?\s*\{(.*?)\};",
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


# =========================================================
# Parsing
# =========================================================

def parse_fields(text: str) -> List[Field]:
    fields = []
    for m in FIELD_RE.finditer(text):
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


def parse_types(text: str) -> Dict[str, TypeDef]:
    types = {}
    for m in TYPE_RE.finditer(text):
        kind, name, base, body = m.groups()
        ignore_base = "@bson(ignore_base)" in text[:m.start()]
        base = None if (not base or is_template_type(base)) else base
        body = extract_public(body) if kind == "class" else body
        types[name] = TypeDef(
            name=name,
            base=base,
            ignore_base=ignore_base,
            fields=parse_fields(body)
        )
    return types


# =========================================================
# BSON helpers
# =========================================================

def bson_get_expr(cxx_type: str, var: str) -> str:
    t = normalize_type(cxx_type)
    if t in ("std::string", "string"):
        return f'{var}.get_utf8().value.to_string()'
    if t in ("int32_t", "int"):
        return f'{var}.get_int32()'
    if t in ("int64_t", "long", "longlong"):
        return f'{var}.get_int64()'
    if t == "bool":
        return f'{var}.get_bool()'
    if t in ("double", "float"):
        return f'{var}.get_double()'
    return None  # user-defined type


def gen_merge(expr: str) -> str:
    return f"""    {{
        auto __view = {expr}.view();
        for (auto&& e : __view) {{
            doc << e.key() << e.get_value();
        }}
    }}"""


# =========================================================
# Codegen
# =========================================================

def gen_to_bson(t: TypeDef, all_types: Dict[str, TypeDef]) -> str:
    lines = []

    if t.base and not t.ignore_base:
        lines.append(gen_merge(f"{t.base}::toBson()"))

    for f in t.fields:
        if f.skip:
            continue
        if f.flatten:
            lines.append(gen_merge(f"{f.name}.toBson()"))
        elif f.type in all_types:
            lines.append(
                f'    doc << "{f.bson_name}" << {f.name}.toBson().view();'
            )
        else:
            lines.append(
                f'    doc << "{f.bson_name}" << {f.name};'
            )

    return "\n".join(lines)


def gen_from_bson(t: TypeDef, all_types: Dict[str, TypeDef]) -> str:
    lines = []

    if t.base and not t.ignore_base:
        lines.append(f'    {t.base}::fromBson(v);')

    for f in t.fields:
        if f.skip:
            continue

        if f.flatten:
            lines.append(f'    {f.name}.fromBson(v);')
            continue

        if f.type in all_types:
            lines.append(
                f'''    if (auto e = v["{f.bson_name}"]; e && e.type() == bsoncxx::type::k_document)
        {f.name}.fromBson(e.get_document().view());'''
            )
            continue

        expr = bson_get_expr(f.type, "e")
        if expr:
            lines.append(
                f'''    if (auto e = v["{f.bson_name}"])
        {f.name} = {expr};'''
            )
        else:
            lines.append(
                f'''    // unsupported field type: {f.type} {f.name}'''
            )

    return "\n".join(lines)


def emit(t: TypeDef, all_types: Dict[str, TypeDef], out: Path):
    path = out / f"{t.name}_bson.gen.cpp"
    path.write_text(f"""#include "{t.name}.hpp"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>

bsoncxx::document::value {t.name}::toBson() const {{
    bsoncxx::builder::stream::document doc{{}};
{gen_to_bson(t, all_types)}
    return doc << bsoncxx::builder::stream::finalize;
}}

void {t.name}::fromBson(bsoncxx::document::view v) {{
{gen_from_bson(t, all_types)}
}}
""", encoding="utf-8")


# =========================================================
# Main
# =========================================================

def main():
    if len(sys.argv) != 3:
        print("usage: python codegen.py <input.hpp | input_dir> <out_dir>")
        sys.exit(1)

    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(exist_ok=True)

    headers = [src] if src.is_file() else list(src.rglob("*.hpp"))

    all_types: Dict[str, TypeDef] = {}
    for h in headers:
        all_types.update(parse_types(h.read_text(encoding="utf-8")))

    for t in all_types.values():
        emit(t, all_types, out)

    print(f"[OK] Generated {len(all_types)} BSON cpp files")


if __name__ == "__main__":
    main()
