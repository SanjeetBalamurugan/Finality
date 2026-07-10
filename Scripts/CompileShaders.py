import pathlib
import os
import subprocess

cwd = pathlib.Path(os.getcwd())
vertexShaders = cwd.rglob("*.vert")
fragmentShaders = cwd.rglob("*.frag")

fiShaderCompilerPath = list(cwd.rglob("FIShaderCompiler.exe"))
if fiShaderCompilerPath == []:
    print("Compile FIShaderCompiler and run this script!!")
    exit()

compileQueue = list(vertexShaders) + list(fragmentShaders)

for shaderPath in compileQueue:
    p = pathlib.Path(shaderPath)

    outputName = p.name + ".spv"
    outputPath = p.parent / outputName
    print("Compiling", p.name, "to", outputPath)

    output = subprocess.run([fiShaderCompilerPath[0], shaderPath, outputPath, "--vulkan"], capture_output=True, text=True)
    
    print(output.stdout.strip())
    print(output.stderr.strip())