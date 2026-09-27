-- Modules return a list of commands; init.lua registers them with edut.
local function wants_help(input)
  return input.contains_flag "--help" or input.contains_flag "-h"
end

return {
  {
    "example",
    flags = { "--help", "-h" },
    subcommands = {
      {
        "greet",
        flags = { "--help", "-h", "--upper", ["--greeting"] = 1 },
        execute = function(input)
          if wants_help(input) then
            print "Usage: edut example greet [name] [--greeting text] [--upper]"
            return
          end
          if input.get_argument(2) then error "Expected at most one name" end

          -- Positional arguments are one-based and return nil when absent.
          local name = input.get_argument(1) or "world"
          local greeting = "Hello"
          -- Check presence before reading a flag's value.
          if input.contains_flag "--greeting" then
            greeting = input.get_argument("--greeting", 1)
          end
          local message = greeting .. ", " .. name .. "!"
          if input.contains_flag "--upper" then message = string.upper(message) end
          print(message)
        end,
      },
    },
    execute = function(input)
      if wants_help(input) then
        print "Usage: edut example greet [name] [--greeting text] [--upper]"
        return
      end
      if input.get_argument(1) then error "Unknown example subcommand" end

      -- Parents explicitly dispatch to the selected subcommand using dot syntax.
      local subcommand = input.get_subcommand()
      if subcommand then
        subcommand.execute(input.for_subcommand())
      else
        print "Try: edut example greet world"
      end
    end,
  },
}
