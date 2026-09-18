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


# =========================================================
# Parsing
# =========================================================

def is_method_decl(text: str, start: int) -> bool:
    # "std::string toString() const override;" -> the match tail "const override;"
    # is preceded by ')', it is a method declaration, not a field
    i = start - 1
    while i >= 0 and text[i].isspace():
        i -= 1
    return i >= 0 and text[i] == ")"


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


# =========================================================
# BSON helpers
# =========================================================

def bson_get_expr(cxx_type: str, var: str) -> str:
    t = normalize_type(cxx_type)
    if t in ("std::string", "string"):
        # mongocxx 4.x: get_utf8() was replaced by get_string(); stdx::string_view no longer
        # has to_string(), so build std::string explicitly.
        return f'std::string({var}.get_string().value)'
    if t in ("int32_t", "int"):
        return f'{var}.get_int32()'
    if t in ("int64_t", "long", "longlong"):
        return f'{var}.get_int64()'
    if t == "bool":
        return f'{var}.get_bool()'
    if t in ("double", "float"):
        return f'{var}.get_double()'
    return None  # user-defined type


def bson_scalar_supported(cxx_type: str) -> bool:
    return bson_get_expr(cxx_type, "_") is not None


def gen_merge(expr: str) -> str:
    return f"""    {{
        auto __view = {expr}.view();
        for (auto&& e : __view) {{
            doc << e.key() << e.get_value();
        }}
    }}"""


# =========================================================
# JSON (redis) helpers
# =========================================================

def json_set_stmts(cxx_type: str, var: str) -> Optional[List[str]]:
    t = normalize_type(cxx_type)
    if t in ("std::string", "string"):
        return [f"__w.String({var}.c_str(), {var}.size());"]
    if t in ("int32_t", "int"):
        return [f"__w.Int({var});"]
    if t in ("int64_t", "long", "longlong"):
        return [f"__w.Int64({var});"]
    if t == "bool":
        return [f"__w.Bool({var});"]
    if t in ("double", "float"):
        return [f"__w.Double({var});"]
    if t == "std::vector<int64_t>":
        return [
            "__w.StartArray();",
            f"for (const auto& __e : {var}) {{",
            "    __w.Int64(__e);",
            "}",
            "__w.EndArray();",
        ]
    return None  # user-defined / complex type


def json_get_stmts(cxx_type: str, var: str) -> Optional[List[str]]:
    t = normalize_type(cxx_type)
    if t in ("std::string", "string"):
        return [
            f"if (__it != __obj.MemberEnd() && __it->value.IsString())",
            f"    {var} = std::string(__it->value.GetString(), __it->value.GetStringLength());",
        ]
    if t in ("int32_t", "int"):
        return [
            f"if (__it != __obj.MemberEnd() && __it->value.IsInt())",
            f"    {var} = __it->value.GetInt();",
        ]
    if t in ("int64_t", "long", "longlong"):
        return [
            f"if (__it != __obj.MemberEnd() && __it->value.IsInt64())",
            f"    {var} = __it->value.GetInt64();",
        ]
    if t == "bool":
        return [
            f"if (__it != __obj.MemberEnd() && __it->value.IsBool())",
            f"    {var} = __it->value.GetBool();",
        ]
    if t == "double":
        return [
            f"if (__it != __obj.MemberEnd() && __it->value.IsDouble())",
            f"    {var} = __it->value.GetDouble();",
        ]
    if t == "float":
        return [
            f"if (__it != __obj.MemberEnd() && __it->value.IsDouble())",
            f"    {var} = static_cast<float>(__it->value.GetDouble());",
        ]
    if t == "std::vector<int64_t>":
        return [
            f"if (__it != __obj.MemberEnd() && __it->value.IsArray()) {{",
            f"    for (const auto& __ae : __it->value.GetArray()) {{",
            f"        if (__ae.IsInt64())",
            f"            {var}.push_back(__ae.GetInt64());",
            f"    }}",
            f"}}",
        ]
    return None  # user-defined / complex type


def gen_to_json(t: TypeDef) -> str:
    lines = []
    for f in t.fields:
        if f.skip:
            continue
        stmts = json_set_stmts(f.type, f.name)
        if stmts:
            lines.append(f'    __w.Key("{f.bson_name}");')
            lines.extend(f"    {s}" for s in stmts)
        else:
            # no base merge / no nested objects on the redis side
            lines.append(f"    // nested/complex field not serialised: {f.type} {f.name}")
    return "\n".join(lines)


def gen_from_json(t: TypeDef) -> str:
    lines = []
    for f in t.fields:
        if f.skip:
            continue
        stmts = json_get_stmts(f.type, f.name)
        if stmts:
            lines.append(f'    __it = __obj.FindMember("{f.bson_name}");')
            lines.extend(f"    {s}" for s in stmts)
        else:
            lines.append(f"    // nested/complex field not deserialised: {f.type} {f.name}")
    return "\n".join(lines)


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
        elif bson_scalar_supported(f.type):
            lines.append(
                f'    doc << "{f.bson_name}" << {f.name};'
            )
        else:
            lines.append(
                f'    // unsupported field type: {f.type} {f.name}'
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


def emit_redis(t: TypeDef, all_types: Dict[str, TypeDef], out: Path):
    path = out / f"{t.name}_redis.gen.cpp"
    path.write_text(f"""#include "{t.name}.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string_view>
#include "core/log/XLog.h"

std::string {t.name}::toString() const {{
    rapidjson::StringBuffer __sb;
    rapidjson::Writer<rapidjson::StringBuffer> __w(__sb);
    __w.StartObject();
{gen_to_json(t)}
    __w.EndObject();
    return std::string(__sb.GetString(), __sb.GetSize());
}}

void {t.name}::fromString(std::string_view sv) {{
    rapidjson::Document __doc;
    __doc.Parse(sv.data(), sv.size());
    if (__doc.HasParseError()) {{
        ERR_LOG("{{}}::fromString parse failed, code={{}} offset={{}}", "{t.name}",
                static_cast<int>(__doc.GetParseError()), __doc.GetErrorOffset());
        return;
    }}
    const rapidjson::Value& __obj = __doc;
    rapidjson::Value::ConstMemberIterator __it;
{gen_from_json(t)}
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
        emit_redis(t, all_types, out)

    print(f"[OK] Generated {len(all_types)} BSON cpp files")
    print(f"[OK] Generated {len(all_types)} REDIS cpp files")


if __name__ == "__main__":
    main()
