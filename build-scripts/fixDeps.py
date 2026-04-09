import os
import sys
import subprocess

MPV_INSTALL_PREFIX= os.getcwd() + "/build/out/mpv"

def getDeps(lib):
    try:
        output = subprocess.check_output(["otool", "-L", lib])
        lines = str(output).split('\\n\\t')
    except:
        return []
    deps = []
    for dep in lines:
        if "/opt/" in dep or "/Cellar" in dep or "/Users" in dep or dep.startswith("lib") or not (dep.startswith("/") or dep.startswith("@executable_path/../Frameworks")):
            deps.append(dep.split(' ')[0])
    return deps

def fixDeps(path):
    name = os.path.basename(path)
    deps = getDeps(path)
    print(deps)
    for dep in deps:
        dname = os.path.basename(dep)
        if dname == name:
            print("change {}".format(dname))
            if "mpv" in name: continue
            subprocess.check_output(['install_name_tool', '-change', '@executable_path/../Frameworks/' + name, path, f"{MPV_INSTALL_PREFIX}/bin/mpv"])
            # subprocess.check_output(['install_name_tool', '-id', '@executable_path/../Frameworks/' + name, path, f"{MPV_INSTALL_PREFIX}/bin/mpv"])
            # subprocess.check_output(['install_name_tool', '-id', '@executable_path/../Frameworks/' + name, path])
        else:
            print(f"no issue: {dname}")
        
    # subprocess.check_output(['install_name_tool', '-add_rpath', '@executable_path/../Frameworks/', f"{MPV_INSTALL_PREFIX}/bin/mpv"])

for file in sys.argv[1:]:
    fixDeps(file)
