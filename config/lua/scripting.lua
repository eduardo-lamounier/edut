-- If you wish to remove this configuration, also remove everything
-- within the 'scripts' folder.

local api = require "edut"

-------------------------------------------------------

local DEFAULT_USER_TERMINAL_WIDTH = 60

local function getScriptsFolder()
  local configFolder = os.getenv "XDG_CONFIG_HOME"

  if not configFolder or configFolder == "" then
    configFolder = os.getenv "HOME" .. "/.config"
  end

  return configFolder .. "/edut/scripts/"
end

local function getUserTerminalWidth()
  local outputFile = io.popen "tput cols 2> /dev/null"

  if not outputFile then return DEFAULT_USER_TERMINAL_WIDTH end

  local result = outputFile:read "*a"
  outputFile:close()

  local width = tonumber(result)
  return width or DEFAULT_USER_TERMINAL_WIDTH
end

-------------------------------------------------------

api.setup {
  commands = {
    {
      "scripts",
      flags = {
        "--help",
        "-h",
      },
      subcommands = {
        {
          "add",
          execute = function(parsedInput) api.err "NOT IMPLEMENTED." end,
        },
        {
          "rm",
          execute = function(parsedInput) api.err "NOT IMPLEMENTED." end,
        },
        {
          "run",
          flags = {
            "--on-background",
            ["--output-file="] = 1,
          },
          execute = function(parsedInput)
            if parsedInput.get_argument(2) then
              api.err "Too many arguments passed: expected script name only"
            end

            local scriptName = parsedInput.get_argument(1)

            local options = {
              runOnBackground = parsedInput.contains_flag "--on-background",
              outputFilePath = parsedInput.contains_flag "--output-file="
                and parsedInput.get_argument("--output-file=", 1),
            }

            if not scriptName then api.err "No script passed to run" end

            print("Running '" .. scriptName .. "'...")
            print(string.rep("`", getUserTerminalWidth()))

            local success = os.execute(
              "bash "
                .. getScriptsFolder()
                .. scriptName
                .. (options.outputFilePath and " 2>&1 | tee -a " .. options.outputFilePath or "")
                .. (options.runOnBackground and " &" or "")
            )

            print("\n" .. string.rep("`", getUserTerminalWidth()))
            print(
              success and "The script ran successfully"
                or "Something went wrong when running (or trying to run) the script"
            )
          end,
        },
        {
          "list",
          flags = {
            ["--grep="] = 1,
          },
          execute = function() api.err "NOT IMPLEMENTED." end,
        },
        {
          "show",
          execute = function(parsedInput) api.err "NOT IMPLEMENTED" end,
        },
        {
          "edit",
          execute = function(parsedInput) api.err "NOT IMPLEMENTED" end,
        },
      },

      execute = function(parsedInput)
        local subcommand = parsedInput.get_subcommand()

        if subcommand ~= nil then
          subcommand.execute(parsedInput.for_subcommand())
          return
        end

        if parsedInput.contains_flag("--help", "-h") then
          print "Used to manage existant scripts or create new custom ones"
        end

        if parsedInput.get_argument(1) ~= nil then api.err "Invalid subcommand." end

        print "No subcommand passed."
      end,
    },
  },
}
