premake.override(premake, "generate", function(base, object, extension, callback)
    if extension == ".vcxproj.filters" or extension == ".vcxitems.filters" then
        os.remove(premake.filename(object, extension))
        return false
    end
    return base(object, extension, callback)
end)
