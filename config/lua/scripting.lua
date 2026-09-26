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

-- All dynamic values are shell arguments, never shell program text.
local function shellQuote(value)
  if value:find("%z") then error("Paths cannot contain NUL bytes", 0) end
  return "'" .. value:gsub("'", "'\"'\"'") .. "'"
end

local function runScript(scriptName, outputFile, background)
  -- Only direct, non-symlink files in the scripts directory may be run.
  if scriptName == "" or scriptName == "." or scriptName == ".."
    or scriptName:find("/", 1, true) or scriptName:find("\\", 1, true)
  then
    error("Expected a script filename inside the scripts directory", 0)
  end

  local scriptPath = getScriptsFolder() .. scriptName
  if scriptPath:sub(1, 1) ~= "/" then scriptPath = "./" .. scriptPath end

  local runner = [=[
set -o pipefail
script=$1
output=$2
background=$3
if [[ ! -f "$script" || ! -r "$script" || -L "$script" ]]; then
  printf 'Script must be a readable, non-symlink file: %s\n' "$script" >&2
  exit 1
fi
if [[ -n "$output" ]]; then
  command -v tee >/dev/null || exit 1
  : >> "$output" || exit 1
fi
run_script() {
  if [[ -n "$output" ]]; then
    bash -- "$script" 2>&1 | tee -a -- "$output"
  else
    bash -- "$script"
  fi
}
if [[ "$background" == yes ]]; then
  run_script </dev/null >/dev/null 2>&1 &
else
  run_script
fi
]=]

  if outputFile == "" then error("Output file path cannot be empty", 0) end
  local success, reason, status = os.execute(
    "bash -c " .. shellQuote(runner) .. " edut-script "
      .. shellQuote(scriptPath) .. " " .. shellQuote(outputFile or "") .. " "
      .. shellQuote(background and "yes" or "no")
  )
  if not success then
    error("Script execution or launch failed (" .. tostring(reason) .. " " .. tostring(status) .. ")", 0)
  end
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

return {
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

          runScript(scriptName, options.outputFilePath, options.runOnBackground)

          print("\n" .. string.rep("`", getUserTerminalWidth()))
          print(options.runOnBackground and "Script launched in background; completion is not tracked"
            or "The script ran successfully")
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
}
