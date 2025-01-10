# Generates header files and main.c for testing

import re
import sys
import subprocess
import os.path

def grab_ids(filepath: str) -> list[str] | None:
    ids = []
    out = subprocess.check_output(["cpp", filepath]).decode()
    if "fatal error" in out:
        return None
    pat = re.compile(r"test_exit_code_t ([A-Za-z0-9_]+)\(void\)")
    index = 0
    while (match := pat.search(out, index)) != None:
        index = match.end()
        ids.append(match.group(1))
    return ids

def main() -> None:
    if len(sys.argv) < 2:
        print(f"usage: python/python3 {sys.argv[0]} [source files...]")
        return
    id_map: dict[str, list[str]] = {}
    for arg in sys.argv[1:]:
        ids = grab_ids(arg)
        if ids == None:
            print(f"{sys.argv[0]}: error: could not find {arg}")
            return
        id_map[arg] = ids
    headernames = []
    for filepath, ids in id_map.items():
        filename = os.path.basename(filepath)
        headername = filename.replace(".c", ".h")
        headerpath = os.path.join(os.path.dirname(filepath), headername)
        headernames.append(headername)
        with open(headerpath, "w") as file:
            guard_macro = headername.upper().replace(".", "_")
            file.write(f"#ifndef {guard_macro}\n")
            file.write(f"#define {guard_macro}\n\n")
            file.write(f"#include \"test.h\"\n\n")

            for id in ids:
                file.write(f"EXPORT_TEST({id});\n")

            file.write(f"\n#endif\n")
    with open(os.path.join(os.path.dirname(sys.argv[1]), "main.c"), "w") as file:
        file.write(f"#include \"test.h\"\n\n")
        for headername in headernames:
            file.write(f"#include \"{headername}\"\n")
        file.write("\n")
        file.write("int main(int argc, char** argv)\n")
        file.write("{\n")
        for _, ids in id_map.items():
            for id in ids:
                file.write(f"    add_test({id});\n")
            file.write("\n")
        file.write(f"    run_tests(argc, argv);\n")
        file.write("}\n")
        
    pass

if __name__ == "__main__":
    main()