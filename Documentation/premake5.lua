project "Documentation"
    kind "Utility"
    local root = path.getabsolute("..")
    local external = os.getenv("DOXYGEN_EXECUTABLE")
    if not external and os.host() == "windows" then
        local installed = path.join(os.getenv("ProgramFiles") or "", "doxygen/bin/doxygen.exe")
        if os.isfile(installed) then external = installed end
    end
    local command = external and ('"' .. external .. '"') or "doxygen"
    files { "**.md", "**.dox", "**.css", "../Doxyfile" }
    buildmessage "Generating Finality API documentation"
    prebuildcommands { '{MKDIR} "' .. path.join(root, 'bin/Documentation') .. '"' }
    filter "system:windows"
        prebuildcommands { 'cd /d "' .. root .. '" && ' .. command .. ' "' .. path.join(root, 'Doxyfile') .. '"' }
    filter "system:not windows"
        prebuildcommands { 'cd "' .. root .. '" && ' .. command .. ' "' .. path.join(root, 'Doxyfile') .. '"' }
    filter {}
