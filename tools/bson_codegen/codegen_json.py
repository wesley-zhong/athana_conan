#!/usr/bin/env python3
"""JSON codegen: emits {Name}_redis.gen.cpp (toString / fromString) for each DO type."""
import sys
from pathlib import Path
from typing import List, Optional

from codegen_common import TypeDef, normalize_type, collect_types


# =========================================================
# JSON helpers
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

def emit(t: TypeDef, out: Path):
    path = out / f"{t.name}_redis.gen.cpp"
    path.write_text(f"""#include "{t.name}.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string_view>
#include "log/XLog.h"

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
        print("usage: python codegen_json.py <input.hpp | input_dir> <out_dir>")
        sys.exit(1)

    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(exist_ok=True)

    all_types = collect_types(src)

    for t in all_types.values():
        emit(t, out)

    print(f"[OK] Generated {len(all_types)} REDIS cpp files")


if __name__ == "__main__":
    main()
