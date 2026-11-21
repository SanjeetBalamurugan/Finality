newoption { trigger="clean-dry-run", description="List generated cleanup targets without deleting them" }
local cleanRoot = path.getabsolute(path.join(_SCRIPT_DIR, ".."))
newaction {
    trigger = "clean",
    description = "Clean generated engine outputs while preserving projects, assets, vendor packages and dependency caches",
    execute = function()
        local function normalized(value)
            value = path.translate(value, "/")
            return os.host() == "windows" and value:lower() or value
        end
        local realRoot = normalized(os.realpath(cleanRoot))
        local function checked(value)
            if os.islink(value) then return false end
            local resolved = os.realpath(value)
            if not resolved then error("Cannot resolve cleanup target: " .. value) end
            resolved = normalized(resolved)
            if resolved:sub(1, #realRoot + 1) ~= realRoot .. "/" then
                error("Cleanup target escaped the workspace: " .. value)
            end
            return true
        end
        local targets, seen = {}, {}
        local function add(file)
            if checked(file) and not seen[file] then targets[#targets+1]=file;seen[file]=true end
        end
        local protected = { projects=true, assets=true, settings=true, [".git"]=true, [".vs"]=true }
        local extensions = { [".obj"]=true,[".pdb"]=true,[".idb"]=true,[".ilk"]=true,[".tlog"]=true,
            [".log"]=true,[".dll"]=true,[".exe"]=true,[".lib"]=true,[".exp"]=true,[".pch"]=true,
            [".o"]=true,[".a"]=true,[".so"]=true,[".dylib"]=true,[".spv"]=true }
        local function collect(directory, everything, shadersOnly)
            if not os.isdir(directory) or not checked(directory) then return end
            if #os.matchfiles(path.join(directory,"*.fproj")) > 0 or os.isfile(path.join(directory,"Player.yaml")) then return end
            for _,file in ipairs(os.matchfiles(path.join(directory,"*"))) do
                local selected = shadersOnly and path.getextension(file):lower()==".spv" and os.isfile(file:sub(1,#file-4))
                if selected or (not shadersOnly and (everything or extensions[path.getextension(file):lower()] or path.getname(file)=="FinalityBuild.json")) then add(file) end
            end
            for _,child in ipairs(os.matchdirs(path.join(directory,"*"))) do
                local name=path.getname(child):lower()
                if not protected[name] and name~="assimp" and name~="llvm" then
                    collect(child,everything or name=="documentation" or name=="clangresource",shadersOnly)
                end
            end
        end
        for _,owner in ipairs({"", "Scripts", "FinalityEngine", "FinalityEditor", "FinalityPlayer", "FIScriptReflect", "FIShaderCompiler", "FIBaker", "FIBuild", "Sandbox", "Documentation", "vendor/stb"}) do
            local directory=path.join(cleanRoot,owner)
            for _,output in ipairs({"bin","bin-int","obj","ipch"}) do collect(path.join(directory,output),false) end
            if owner~="" then
                for _,suffix in ipairs({"*.vcxproj","*.vcxproj.filters","*.vcxproj.user"}) do
                    for _,file in ipairs(os.matchfiles(path.join(directory,suffix))) do add(file) end
                end
            end
        end
        for _,name in ipairs({"Finality.sln","Finality.slnx"}) do
            local file=path.join(cleanRoot,name);if os.isfile(file) then add(file) end
        end
        for _,directory in ipairs({"FinalityEditor/assets","Sandbox/assets"}) do
            collect(path.join(cleanRoot,directory),false,true)
        end
        table.sort(targets)
        for _,file in ipairs(targets) do
            if _OPTIONS["clean-dry-run"] then print("Would remove: " .. file)
            else
                if not checked(file) then error("Cleanup target became a link: " .. file) end
                local ok,message=os.remove(file)
                if not ok then error("Could not clean " .. file .. ": " .. tostring(message)) end
            end
        end
        print((_OPTIONS["clean-dry-run"] and "Planned " or "Cleaned ") .. #targets .. " generated files. Projects, authored assets, vendor packages, IDE state and Assimp dependency caches were preserved.")
    end
}
