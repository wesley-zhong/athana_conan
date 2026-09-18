#!/usr/bin/env python3
"""BSON codegen: emits {Name}_bson.gen.cpp (toBson / fromBson) for each DO type."""
import sys
from pathlib import Path
from typing import Dict

from codegen_common import TypeDef, normalize_type, collect_types


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


# =========================================================
# Main
# =========================================================

def main():
    if len(sys.argv) != 3:
        print("usage: python codegen_bson.py <input.hpp | input_dir> <out_dir>")
        sys.exit(1)

    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(exist_ok=True)

    all_types = collect_types(src)

    for t in all_types.values():
        emit(t, all_types, out)

    print(f"[OK] Generated {len(all_types)} BSON cpp files")


if __name__ == "__main__":
    main()
