newaction {
    trigger     = "clean",
    description = "Removes all generated binaries, intermediate build files, and VS caches",
    execute     = function()
        print("Cleaning project files...")

        local directories = {
            ".vs",
            "bin",
            "bin-int",
            "obj",
            "ipch"
        }

        local file_extensions = {
            "*.sln",
            "*.slnx",
            "*.vcxproj",
            "*.vcxproj.filters",
            "*.vcxproj.user"
        }

        for _, dir in ipairs(directories) do
            if os.isdir(dir) then
                print("Removing directory: " .. dir)
                os.rmdir(dir)
            end
        end

        for _, pattern in ipairs(file_extensions) do
            local files = os.matchfiles("**" .. pattern)
            for _, file in ipairs(files) do
                print("Removing file: " .. file)
                os.remove(file)
            end
        end

        print("Clean complete!")
    end
}
